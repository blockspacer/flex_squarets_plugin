#include <flex_squarets_plugin/Tooling.hpp> // IWYU pragma: associated

#include <squarets/core/squarets.hpp>
#include <squarets/codegen/cpp/cpp_codegen.hpp>
#include <squarets/core/defaults/defaults.hpp>
#include <squarets/core/tags.hpp>
#include <squarets/core/errors/errors.hpp>

#include <flexlib/reflect/ReflTypes.hpp>
#include <flexlib/reflect/ReflectAST.hpp>
#include <flexlib/reflect/ReflectionCache.hpp>
#include <flexlib/ToolPlugin.hpp>
#include <flexlib/core/errors/errors.hpp>
#include <flexlib/utils.hpp>
#include <flexlib/funcParser.hpp>
#include <flexlib/inputThread.hpp>
#include <flexlib/clangUtils.hpp>
#include <flexlib/clangPipeline.hpp>
#include <flexlib/annotation_parser.hpp>
#include <flexlib/annotation_match_handler.hpp>
#include <flexlib/matchers/annotation_matcher.hpp>
#include <flexlib/options/ctp/options.hpp>
#if defined(CLING_IS_ON)
#include "flexlib/ClingInterpreterModule.hpp"
#endif // CLING_IS_ON

#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Lexer.h>

#include <base/cpu.h>
#include <base/bind.h>
#include <base/command_line.h>
#include <base/debug/alias.h>
#include <base/debug/stack_trace.h>
#include <base/memory/ptr_util.h>
#include <base/sequenced_task_runner.h>
#include <base/trace_event/trace_event.h>
#include <base/logging.h>
#include <base/strings/string_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>
#include <base/strings/utf_string_conversions.h>
#include <base/stl_util.h>
#include <base/files/file_util.h>

#include <any>
#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>

namespace plugin {

namespace {

static const char kAnnotationCXTPL[] = "CXTPL;";

static const size_t kMB = 1024 * 1024;

static const size_t kGB = 1024 * kMB;

/// \note |base::ReadFileToStringWithMaxSize| implementation
/// uses size_t
/// The standard says that SIZE_MAX for size_t must be at least 65535.
/// It specifies no upper bound for size_t.
/// \note usually size_t limited to (32bit) 4294967295U
/// or (64bit) 18446744073709551615UL
static const size_t kMaxFileSizeInBytes = 1024 * kGB;

// example before:
// __attribute__((annotate("{gen};{squarets};CXTPL;" #__VA_ARGS__ )))
// contentsUTF16 == "CXTPL;" #__VA_ARGS__
// example after:
// result == "" #__VA_ARGS__
static bool removeSyntaxPrefix(
  const clang::SourceLocation& initStartLoc
  , const char prefix[]
  , const size_t prefix_size
  , clang::SourceManager &SM
  , base::StringPiece16& result)
{
  const bool isSyntaxCXTPL
    = base::StartsWith(
        result
        , base::StringPiece16{
            base::ASCIIToUTF16(prefix)}
        , base::CompareCase::INSENSITIVE_ASCII);
  if(isSyntaxCXTPL) {
    DCHECK(prefix_size);
    result.remove_prefix(
      prefix_size - 1);
  } else {
    DCHECK(initStartLoc.isValid());
    LOG(ERROR)
      << "(squarets) invalid annotation syntax."
         " For now squarets supports only `CXTPL;`"
         " template engine: "
      << initStartLoc.printToString(SM);
    CHECK(false);
    return false;
  }

  return true;
}

static std::string runTemplateParser(
  // name of output variable in generated code
  const std::string& nodeName
  // template to parse
  , const base::StringPiece16& clean_contents
  // initial annotation code, for logging
  , const std::string& processedAnnotation
){
  squarets::core::Generator template_engine(
    // output variable name
    nodeName
  );

  const outcome::result<
      std::string
      , squarets::core::errors::GeneratorErrorExtraInfo
    >
    genResult
      = template_engine.generate_from_UTF16(
        clean_contents);

  if(genResult.has_error()) {
    {
      const std::error_code& ec
        = make_error_code(genResult.error().ec);
      LOG(ERROR)
        << "(squarets) ERROR:"
        << " message: "
        << ec.message()
        << " category: "
        << ec.category().name()
        << " info: "
        << genResult.error().extra_info
        << " input data: "
        /// \note limit to first N symbols
        << processedAnnotation.substr(0, 1000)
        << "...";
    }
    CHECK(false);
    return "";
  }

  if(!genResult.has_value() || genResult.value().empty()) {
    LOG(WARNING) << "WARNING: empty output from squarets ";
    return "";
  }

  return genResult.value();
}

static void insertCodeAfterPos(
  const std::string& processedAnnotation
  , clang::AnnotateAttr* annotateAttr
  , const clang_utils::MatchResult& matchResult
  , clang::Rewriter& rewriter
  , const clang::Decl* nodeDecl
  , clang::SourceLocation& nodeStartLoc
  , clang::SourceLocation& nodeEndLoc
  , const std::string& codeToInsert
){
  clang::SourceManager &SM
    = rewriter.getSourceMgr();

  const clang::LangOptions& langOptions
    = rewriter.getLangOpts();

  clang_utils::expandLocations(
    nodeStartLoc, nodeEndLoc, rewriter);

  clang::CharSourceRange charSourceRange(
    clang::SourceRange{nodeStartLoc, nodeEndLoc},
    true // IsTokenRange
  );

#if defined(DEBUG_VERBOSE_PLUGIN)
  if(charSourceRange.isValid()) {
    StringRef sourceText
      = clang::Lexer::getSourceText(
          charSourceRange
          , SM, langOptions, 0);
    DCHECK(nodeStartLoc.isValid());
    VLOG(9)
      << "(squarets) original code: "
      << sourceText.str()
      << " at "
      << nodeStartLoc.printToString(SM);
  } else {
    DCHECK(nodeStartLoc.isValid());
    LOG(ERROR)
      << "variable declaration with"
         " annotation of type squarets"
         " must have initializer"
         " that is valid: "
      << nodeStartLoc.printToString(SM);
    return;
  }
#endif // DEBUG_VERBOSE_PLUGIN

  // MeasureTokenLength gets us past the last token,
  // and adding 1 gets us past the ';'
  int offset = clang::Lexer::MeasureTokenLength(
    nodeEndLoc,
    SM,
    langOptions) + 1;

  clang::SourceLocation realEnd
    = nodeEndLoc.getLocWithOffset(offset);

  rewriter.InsertText(realEnd, codeToInsert,
    /*InsertAfter=*/true, /*IndentNewLines*/ false);
}

static void replaceCodeAfterPos(
  const std::string& processedAnnotation
  , clang::AnnotateAttr* annotateAttr
  , const clang_utils::MatchResult& matchResult
  , clang::Rewriter& rewriter
  , const clang::Decl* nodeDecl
  , clang::SourceLocation& nodeStartLoc
  , clang::SourceLocation& nodeEndLoc
  , const std::string& codeToInsert
){
  clang::SourceManager &SM
    = rewriter.getSourceMgr();

  const clang::LangOptions& langOptions
    = rewriter.getLangOpts();

  clang_utils::expandLocations(
    nodeStartLoc, nodeEndLoc, rewriter);

  clang::CharSourceRange charSourceRange(
    clang::SourceRange{nodeStartLoc, nodeEndLoc},
    true // IsTokenRange
  );

#if defined(DEBUG_VERBOSE_PLUGIN)
  if(charSourceRange.isValid()) {
    StringRef sourceText
      = clang::Lexer::getSourceText(
          charSourceRange
          , SM, langOptions, 0);
    DCHECK(nodeStartLoc.isValid());
    VLOG(9)
      << "(squarets) original code: "
      << sourceText.str()
      << " at "
      << nodeStartLoc.printToString(SM);
  } else {
    DCHECK(nodeStartLoc.isValid());
    LOG(ERROR)
      << "variable declaration with"
         " annotation of type squarets"
         " must have initializer"
         " that is valid: "
      << nodeStartLoc.printToString(SM);
    return;
  }
#endif // DEBUG_VERBOSE_PLUGIN

  // MeasureTokenLength gets us past the last token,
  // and adding 1 gets us past the ';'
  int offset = clang::Lexer::MeasureTokenLength(
    nodeEndLoc,
    SM,
    langOptions) + 1;

  clang::SourceLocation realEnd
    = nodeEndLoc.getLocWithOffset(offset);

  rewriter.ReplaceText(
    clang::SourceRange{nodeStartLoc, realEnd}, codeToInsert);
}

static void executeCodeInInterpreter(
  ::cling_utils::ClingInterpreter* clingInterpreter_
  // for debug
  , const std::string& processedAnnotation
  , clang::AnnotateAttr* annotateAttr
  , const clang_utils::MatchResult& matchResult
  , clang::Rewriter& rewriter
  , const clang::Decl* nodeDecl
  , const base::StringPiece16& codeToExecute
  , cling::Value& result
  , const std::string& extraVariables = ""
){
  std::ostringstream sstr;
  // populate variables that can be used by interpreted code:
  //   clangMatchResult, clangRewriter, clangDecl
  ///
  /// \todo convert multiple variables to single struct or tuple
  {
    // scope begin
    sstr << "[](){";

    sstr << "clang::AnnotateAttr*"
            " clangAnnotateAttr = ";
    sstr << cling_utils::passCppPointerIntoInterpreter(
      reinterpret_cast<void*>(annotateAttr)
      , "(clang::AnnotateAttr*)");
    sstr << ";";

    sstr << "const clang::ast_matchers::MatchFinder::MatchResult&"
            " clangMatchResult = ";
    sstr << cling_utils::passCppPointerIntoInterpreter(
      reinterpret_cast<void*>(
        &const_cast<clang_utils::MatchResult&>(matchResult))
      , "*(const clang::ast_matchers::MatchFinder::MatchResult*)");
    sstr << ";";

    sstr << "clang::Rewriter&"
            " clangRewriter = ";
    sstr << cling_utils::passCppPointerIntoInterpreter(
      reinterpret_cast<void*>(&rewriter)
      , "*(clang::Rewriter*)");
    sstr << ";";

    sstr << "const clang::Decl*"
            " clangDecl = ";
    sstr << cling_utils::passCppPointerIntoInterpreter(
      reinterpret_cast<void*>(const_cast<clang::Decl*>(nodeDecl))
      , "(const clang::Decl*)");
    sstr << ";";

    sstr << extraVariables;

    // vars end
    sstr << "return ";
    sstr << codeToExecute << ";";

    // scope end
    sstr << "}();";
  }

  VLOG(9)
    << "(squarets) executing code: "
    << sstr.str();

  {
    cling::Interpreter::CompilationResult compilationResult
      = clingInterpreter_->processCodeWithResult(
          sstr.str(), result);
    if(compilationResult
       != cling::Interpreter::Interpreter::kSuccess)
    {
      LOG(ERROR)
        << "ERROR while running cling code:"
        << codeToExecute.substr(0, 10000)
        << " from annotation:"
        << processedAnnotation.substr(0, 10000)
        << "...";
    }
  }
}

} // namespace

SquaretsTooling::SquaretsTooling(
  const ::plugin::ToolPlugin::Events::RegisterAnnotationMethods& event
#if defined(CLING_IS_ON)
  , ::cling_utils::ClingInterpreter* clingInterpreter
#endif // CLING_IS_ON
) : clingInterpreter_(clingInterpreter)
{
  DCHECK(clingInterpreter_);

  DETACH_FROM_SEQUENCE(sequence_checker_);

  DCHECK(event.sourceTransformPipeline);
  ::clang_utils::SourceTransformPipeline& sourceTransformPipeline
    = *event.sourceTransformPipeline;

  sourceTransformRules_
    = &sourceTransformPipeline.sourceTransformRules;
}

SquaretsTooling::~SquaretsTooling()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void SquaretsTooling::interpretSquarets(
  const std::string& processedAnnotation
  , clang::AnnotateAttr* annotateAttr
  , const clang_utils::MatchResult& matchResult
  , clang::Rewriter& rewriter
  , const clang::Decl* nodeDecl)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT0("toplevel",
               "plugin::FlexSquarets::interpretSquarets");

#if defined(CLING_IS_ON)
  DCHECK(clingInterpreter_);
#endif // CLING_IS_ON

  DCHECK(nodeDecl);

  VLOG(9)
    << "squarets called...";

  clang::SourceManager &SM
    = rewriter.getSourceMgr();

  const clang::LangOptions& langOptions
    = rewriter.getLangOpts();

  std::string nodeName;

  const clang::CXXRecordDecl* nodeRecordDecl =
    matchResult.Nodes.getNodeAs<
      clang::CXXRecordDecl>("bind_gen");

  if(nodeRecordDecl) {
    nodeName = nodeRecordDecl->getNameAsString();
  } else {
    CHECK(false)
      << "interpretSquarets error: "
         "node must be CXXRecordDecl";
  }

  DCHECK(!nodeName.empty());

  VLOG(9)
    << "(squarets) nodeDecl name: "
    << nodeName;

  VLOG(9)
    << "(squarets) nodeDecl processedAnnotation: "
    << processedAnnotation;

  clang::SourceLocation nodeStartLoc
    = nodeDecl->getLocStart();
  // Note Stmt::getLocEnd() returns the source location prior to the
  // token at the end of the line.  For instance, for:
  // var = 123;
  //      ^---- getLocEnd() points here.
  clang::SourceLocation nodeEndLoc
    = nodeDecl->getLocEnd();
  DCHECK(nodeStartLoc != nodeEndLoc);

  base::string16 contentsUTF16
    = base::UTF8ToUTF16(processedAnnotation);

  base::StringPiece16 clean_contents = contentsUTF16;

  bool isCleaned
    = removeSyntaxPrefix(
        nodeStartLoc
        , kAnnotationCXTPL
        , base::size(kAnnotationCXTPL)
        , SM
        , clean_contents);
  DCHECK(isCleaned);

  VLOG(9)
    << "(squarets) nodeVarDecl clean_contents: "
    << clean_contents;

  DCHECK(!nodeName.empty());
  std::string squaretsProcessedAnnotation
    = runTemplateParser(
        // name of output variable in generated code
        nodeName
        // template to parse
        , clean_contents
        // initial annotation code, for logging
        , processedAnnotation);

  if(squaretsProcessedAnnotation.empty()) {
    DCHECK(nodeStartLoc.isValid());
    LOG(ERROR)
      << "variable declaration with"
         " annotation of type squarets"
         " must be valid: "
      << nodeStartLoc.printToString(SM);
  }

#if defined(CLING_IS_ON)
  // execute code stored in annotation
  cling::Value result;

  std::string codeToExecute;

  {
    /// \note lambda will be returned
    codeToExecute += "[&](){";

    codeToExecute += "std::string ";

    // name of output variable in generated code
    codeToExecute += nodeName;

    codeToExecute += ";";

    DCHECK(!squaretsProcessedAnnotation.empty());
    codeToExecute += squaretsProcessedAnnotation;

    codeToExecute += "return new llvm::Optional<std::string>{"
                     "std::move(";
    codeToExecute += nodeName;
    codeToExecute += ")"
                     "}; ";

    codeToExecute += "}();";
  }

  std::ostringstream sstr;

  /// \todo support custom namespaces
  reflection::NamespacesTree m_namespaces;

  reflection::AstReflector reflector(
    matchResult.Context);

  /// \note scope prolongs lifetime
  reflection::ClassInfoPtr classInfoPtr;

  if(nodeRecordDecl)
  {
    classInfoPtr
      = reflector.ReflectClass(
          nodeRecordDecl
          , &m_namespaces
          , false // recursive
        );
    DCHECK(classInfoPtr);

    sstr << "const reflection::ClassInfo*"
            " classInfoPtr = ";
    sstr << cling_utils::passCppPointerIntoInterpreter(
      reinterpret_cast<void*>(const_cast<reflection::ClassInfo*>(classInfoPtr.get()))
      , "(const reflection::ClassInfo*)");
    sstr << ";";
  } // if nodeRecordDecl

  const std::string extraVarables
    = sstr.str();

  base::string16 codeToExecuteUTF16
    = base::UTF8ToUTF16(codeToExecute);

  base::StringPiece16 codeToExecute16 = codeToExecuteUTF16;

  executeCodeInInterpreter(
    clingInterpreter_
    , processedAnnotation // for debug
    , annotateAttr
    , matchResult
    , rewriter
    , nodeDecl
    , codeToExecute16
    , result
    , extraVarables
  );

  if(result.hasValue() && result.isValid()
        && !result.isVoid())
  {
    void* resOptionVoid = result.getAs<void*>();
    auto resOption =
    static_cast<llvm::Optional<std::string>*>(resOptionVoid);
    if(resOption && resOption->hasValue()) {
      DCHECK(!resOption->getValue().empty());
      replaceCodeAfterPos(
        processedAnnotation
        , annotateAttr
        , matchResult
        , rewriter
        , nodeDecl
        , nodeStartLoc
        , nodeEndLoc
        , resOption->getValue()
      );
    } else {
      LOG(ERROR)
        << "interpretSquarets: ."
        << " Nothing provided";
      DCHECK(false);
    }
    delete resOption; /// \note frees resOptionVoid memory
  } else {
    DLOG(INFO)
      << "ignored invalid "
         "Cling result "
         "for processedAnnotation: "
      << processedAnnotation;
  }
#else
  LOG(WARNING)
    << "Unable to execute C++ code at runtime: "
    << "Cling is disabled.";
#endif // CLING_IS_ON
}

void SquaretsTooling::squaretsCodeAndReplace(
  const std::string& processedAnnotation
  , clang::AnnotateAttr* annotateAttr
  , const clang_utils::MatchResult& matchResult
  , clang::Rewriter& rewriter
  , const clang::Decl* nodeDecl)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT0("toplevel",
               "plugin::FlexSquarets::process_squaretsCodeAndReplace");

  DCHECK(nodeDecl);

#if defined(CLING_IS_ON)
  DCHECK(clingInterpreter_);
#endif // CLING_IS_ON

  DLOG(INFO)
    << "started processing of annotation: "
    << processedAnnotation;

  clang::SourceManager &SM
    = rewriter.getSourceMgr();

  const clang::LangOptions& langOptions
    = rewriter.getLangOpts();

  clang::SourceLocation nodeStartLoc
    = nodeDecl->getLocStart();
  // Note Stmt::getLocEnd() returns the source location prior to the
  // token at the end of the line.  For instance, for:
  // var = 123;
  //      ^---- getLocEnd() points here.
  clang::SourceLocation nodeEndLoc
    = nodeDecl->getLocEnd();
  DCHECK(nodeStartLoc != nodeEndLoc);

  base::string16 contentsUTF16
    = base::UTF8ToUTF16(processedAnnotation);

  base::StringPiece16 clean_contents = contentsUTF16;

  bool isCleaned
    = removeSyntaxPrefix(
        nodeStartLoc
        , kAnnotationCXTPL
        , base::size(kAnnotationCXTPL)
        , SM
        , clean_contents);
  DCHECK(isCleaned);

  VLOG(9)
    << "(squarets) nodeVarDecl clean_contents: "
    << clean_contents;

  /// VarDecl - An instance of this class is created
  /// to represent a variable
  /// declaration or definition.
  const clang::VarDecl* nodeVarDecl =
    matchResult.Nodes.getNodeAs<
      clang::VarDecl>("bind_gen");

  if(!nodeVarDecl) {
    /// CXXRecordDecl Represents a C++ struct/union/class.
    clang::CXXRecordDecl const *record =
        matchResult.Nodes
        .getNodeAs<clang::CXXRecordDecl>("bind_gen");
    LOG(WARNING)
      << "squarets annotation must be"
         " placed near to clang::VarDecl "
      << (record ? record->getNameAsString() : "");
    return;
  }

  const std::string nodeName
    = nodeVarDecl->getNameAsString();

  DCHECK(!nodeName.empty());

  VLOG(9)
    << "(squarets) nodeVarDecl name: "
    << nodeName;

#if defined(CLING_IS_ON)
  // execute code stored in annotation
  cling::Value result;

  executeCodeInInterpreter(
    clingInterpreter_
    , processedAnnotation // for debug
    , annotateAttr
    , matchResult
    , rewriter
    , nodeDecl
    , clean_contents // codeToExecute
    , result
  );

  if(result.hasValue() && result.isValid()
        && !result.isVoid())
  {
    void* resOptionVoid = result.getAs<void*>();
    auto resOption =
    static_cast<llvm::Optional<std::string>*>(resOptionVoid);
    if(resOption && resOption->hasValue()) {
      std::string squaretsProcessedAnnotation
        = runTemplateParser(
            // name of output variable in generated code
            nodeName
            // template to parse
            , base::UTF8ToUTF16(resOption->getValue())
            // initial annotation code, for logging
            , processedAnnotation);

      if(squaretsProcessedAnnotation.empty()) {
        DCHECK(nodeStartLoc.isValid());
        LOG(ERROR)
          << "variable declaration with"
             " annotation of type squarets"
             " must be valid: "
          << nodeStartLoc.printToString(SM);
      }

      insertCodeAfterPos(
        processedAnnotation
        , annotateAttr
        , matchResult
        , rewriter
        , nodeDecl
        , nodeStartLoc
        , nodeEndLoc
        , squaretsProcessedAnnotation
      );
    } else {
      LOG(ERROR)
        << "squaretsCodeAndReplace: ."
        << " Nothing provided";
      DCHECK(false);
    }
    delete resOption; /// \note frees resOptionVoid memory
  } else {
    DLOG(INFO)
      << "ignored invalid "
         "Cling result "
         "for processedAnnotation: "
      << processedAnnotation;
  }
#else
  LOG(WARNING)
    << "Unable to execute C++ code at runtime: "
    << "Cling is disabled.";
#endif // CLING_IS_ON
}

void SquaretsTooling::squaretsFile(
  const std::string& processedAnnotation
  , clang::AnnotateAttr* annotateAttr
  , const clang_utils::MatchResult& matchResult
  , clang::Rewriter& rewriter
  , const clang::Decl* nodeDecl)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT0("toplevel",
               "plugin::FlexSquarets::squaretsFile");

#if defined(CLING_IS_ON)
  DCHECK(clingInterpreter_);
#endif // CLING_IS_ON

  DCHECK(nodeDecl);

  VLOG(9)
    << "squaretsFile called...";

  clang::SourceManager &SM
    = rewriter.getSourceMgr();

  const clang::LangOptions& langOptions
    = rewriter.getLangOpts();

  /// VarDecl - An instance of this class is created to represent a variable
  /// declaration or definition.
  const clang::VarDecl* nodeVarDecl =
    matchResult.Nodes.getNodeAs<
      clang::VarDecl>("bind_gen");

  if(!nodeVarDecl) {
    /// CXXRecordDecl Represents a C++ struct/union/class.
    clang::CXXRecordDecl const *record =
        matchResult.Nodes
        .getNodeAs<clang::CXXRecordDecl>("bind_gen");
    LOG(WARNING)
      << "squarets annotation must be"
         " placed near to clang::VarDecl "
      << (record ? record->getNameAsString() : "");
    return;
  }

  const std::string nodeName = nodeVarDecl->getNameAsString();

  DCHECK(!nodeName.empty());

  VLOG(9)
    << "(squaretsFile) nodeVarDecl name: "
    << nodeName;

  VLOG(9)
    << "(squaretsFile) nodeVarDecl processedAnnotation: "
    << processedAnnotation;

  clang::SourceLocation nodeStartLoc
    = nodeVarDecl->getLocStart();
  // Note Stmt::getLocEnd() returns the source location prior to the
  // token at the end of the line.  For instance, for:
  // var = 123;
  //      ^---- getLocEnd() points here.
  clang::SourceLocation nodeEndLoc
    = nodeVarDecl->getLocEnd();
  DCHECK(nodeStartLoc != nodeEndLoc);

  base::string16 contentsUTF16
    = base::UTF8ToUTF16(processedAnnotation);

  base::StringPiece16 clean_contents = contentsUTF16;

  bool isCleaned
    = removeSyntaxPrefix(
        nodeStartLoc
        , kAnnotationCXTPL
        , base::size(kAnnotationCXTPL)
        , SM
        , clean_contents);
  DCHECK(isCleaned);

  VLOG(9)
    << "(squaretsFile) nodeVarDecl clean_contents: "
    << clean_contents;

  base::FilePath filePath{
    base::UTF16ToUTF8(clean_contents)};
  if(!base::PathExists(filePath)) {
    LOG(ERROR)
      << "unable to find file: "
      << filePath
      << " see "
      << nodeStartLoc.printToString(SM);
  }
  else if(base::DirectoryExists(filePath)) {
    LOG(ERROR)
      << "expected file, not directory: "
      << filePath
      << " see "
      << nodeStartLoc.printToString(SM);
  }

  std::string file_contents;

  // When the file size exceeds |max_size|, the
  // function returns false with |contents|
  // holding the file truncated to |max_size|.
  const bool file_ok
    = base::ReadFileToStringWithMaxSize(
        filePath
        , &file_contents
        // |max_size| in bytes
        , kMaxFileSizeInBytes
      );

  if(!file_ok)
  {
    LOG(WARNING)
      << "(squarets) Unable to read file"
      << filePath;
  }

  if(file_contents.empty()) {
    LOG(WARNING)
      << "(squarets) Empty file "
      << filePath;
  }

  base::string16 fileContentsUTF16
    = base::UTF8ToUTF16(file_contents);

  std::string squaretsProcessedAnnotation
    = runTemplateParser(
        // name of output variable in generated code
        nodeName
        // template to parse
        , fileContentsUTF16
        // initial annotation code, for logging
        , processedAnnotation);

  if(squaretsProcessedAnnotation.empty()) {
    DCHECK(nodeStartLoc.isValid());
    LOG(ERROR)
      << "variable declaration with"
         " annotation of type squarets"
         " must be valid: "
      << nodeStartLoc.printToString(SM);
  }

  insertCodeAfterPos(
    processedAnnotation
    , annotateAttr
    , matchResult
    , rewriter
    , nodeDecl
    , nodeStartLoc
    , nodeEndLoc
    , squaretsProcessedAnnotation
  );
}

void SquaretsTooling::squarets(
  const std::string& processedAnnotation
  , clang::AnnotateAttr* annotateAttr
  , const clang_utils::MatchResult& matchResult
  , clang::Rewriter& rewriter
  , const clang::Decl* nodeDecl)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT0("toplevel",
               "plugin::FlexSquarets::callFuncBySignature");

#if defined(CLING_IS_ON)
  DCHECK(clingInterpreter_);
#endif // CLING_IS_ON

  DCHECK(nodeDecl);

  VLOG(9)
    << "squarets called...";

  clang::SourceManager &SM
    = rewriter.getSourceMgr();

  const clang::LangOptions& langOptions
    = rewriter.getLangOpts();

  /// VarDecl - An instance of this class is created to represent a variable
  /// declaration or definition.
  const clang::VarDecl* nodeVarDecl =
    matchResult.Nodes.getNodeAs<
      clang::VarDecl>("bind_gen");

  if(!nodeVarDecl) {
    /// CXXRecordDecl Represents a C++ struct/union/class.
    clang::CXXRecordDecl const *record =
        matchResult.Nodes
        .getNodeAs<clang::CXXRecordDecl>("bind_gen");
    LOG(WARNING)
      << "squarets annotation must be"
         " placed near to clang::VarDecl "
      << (record ? record->getNameAsString() : "");
    return;
  }

  const std::string nodeName = nodeVarDecl->getNameAsString();

  DCHECK(!nodeName.empty());

  VLOG(9)
    << "(squarets) nodeVarDecl name: "
    << nodeName;

  VLOG(9)
    << "(squarets) nodeVarDecl processedAnnotation: "
    << processedAnnotation;

  clang::SourceLocation nodeStartLoc
    = nodeVarDecl->getLocStart();
  // Note Stmt::getLocEnd() returns the source location prior to the
  // token at the end of the line.  For instance, for:
  // var = 123;
  //      ^---- getLocEnd() points here.
  clang::SourceLocation nodeEndLoc
    = nodeVarDecl->getLocEnd();
  DCHECK(nodeStartLoc != nodeEndLoc);

  base::string16 contentsUTF16
    = base::UTF8ToUTF16(processedAnnotation);

  base::StringPiece16 clean_contents = contentsUTF16;

  bool isCleaned
    = removeSyntaxPrefix(
        nodeStartLoc
        , kAnnotationCXTPL
        , base::size(kAnnotationCXTPL)
        , SM
        , clean_contents);
  DCHECK(isCleaned);

  VLOG(9)
    << "(squarets) nodeVarDecl clean_contents: "
    << clean_contents;

  std::string squaretsProcessedAnnotation
    = runTemplateParser(
        // name of output variable in generated code
        nodeName
        // template to parse
        , clean_contents
        // initial annotation code, for logging
        , processedAnnotation);

  if(squaretsProcessedAnnotation.empty()) {
    DCHECK(nodeStartLoc.isValid());
    LOG(ERROR)
      << "variable declaration with"
         " annotation of type squarets"
         " must be valid: "
      << nodeStartLoc.printToString(SM);
  }

  insertCodeAfterPos(
    processedAnnotation
    , annotateAttr
    , matchResult
    , rewriter
    , nodeDecl
    , nodeStartLoc
    , nodeEndLoc
    , squaretsProcessedAnnotation
  );
}

} // namespace plugin
