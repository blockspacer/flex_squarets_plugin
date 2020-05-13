#include <flex_squarets_plugin/Tooling.hpp> // IWYU pragma: associated

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
#include <base/strings/string_util.h>
#include <base/trace_event/trace_event.h>
#include <base/logging.h>

#include <any>
#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <fstream>

namespace plugin {

Tooling::Tooling(
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

Tooling::~Tooling()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

clang_utils::SourceTransformResult
  Tooling::squarets(
    const clang_utils::SourceTransformOptions& sourceTransformOptions)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  VLOG(9)
    << "squarets called...";

  flexlib::args typeclassBaseNames =
    sourceTransformOptions.func_with_args.parsed_func_.args_;

  clang::Rewriter& rewriter
    = sourceTransformOptions.rewriter;

  clang::SourceManager &SM
    = rewriter.getSourceMgr();

  const clang::LangOptions& langOptions
    = rewriter.getLangOpts();

  /// VarDecl - An instance of this class is created to represent a variable
  /// declaration or definition.
  const clang::VarDecl* nodeVarDecl =
    sourceTransformOptions.matchResult.Nodes.getNodeAs<
      clang::VarDecl>("bind_gen");

  if(!nodeVarDecl) {
    /// CXXRecordDecl Represents a C++ struct/union/class.
    clang::CXXRecordDecl const *record =
        sourceTransformOptions.matchResult.Nodes
        .getNodeAs<clang::CXXRecordDecl>("bind_gen");
    LOG(WARNING)
      << "squarets annotation must be placed near clang::VarDecl "
      << (record ? record->getNameAsString() : "");
    return clang_utils::SourceTransformResult{nullptr};
  }

  const std::string nodeName = nodeVarDecl->getNameAsString();

  DCHECK(!nodeName.empty());

  LOG(INFO)
    << "(squarets) nodeVarDecl name: "
    << nodeName;

  // source range of initializer
  clang::SourceLocation startLoc;
  clang::SourceLocation endLoc;

  // source text of initializer
  std::string sourceCode;

  // get source text of RValue variable initializer
  if(nodeVarDecl->hasInit()) {
    const clang::Expr* varInit = nodeVarDecl->getInit();
    if(varInit->isRValue()) {
      clang::SourceRange varSourceRange
        = varInit->getSourceRange();
      clang::CharSourceRange charSourceRange(
        varSourceRange, true);
      startLoc = charSourceRange.getBegin();
      endLoc = charSourceRange.getEnd();
      if(varSourceRange.isValid()) {
        StringRef sourceText
          = clang::Lexer::getSourceText(
              charSourceRange
              , SM, langOptions, 0);
        sourceCode = sourceText.str();
        LOG(INFO)
          << "(squarets) nodeVarDecl RValue: "
          << sourceCode;
      } else {
        LOG(ERROR)
          << "variable declaration with"
             " annotation of type squarets"
             " must have initializer"
             " that is RValue"
             " and valid";
        return clang_utils::SourceTransformResult{nullptr};
      }
    } else {
      LOG(ERROR)
        << "variable declaration with"
           " annotation of type squarets"
           " must have initializer"
           " that is RValue";
      return clang_utils::SourceTransformResult{nullptr};
    }
  } else {
    LOG(ERROR)
      << "variable declaration with"
         " annotation of type squarets"
         " must have initializer";
    return clang_utils::SourceTransformResult{nullptr};
  }

  if(sourceCode.empty()) {
    LOG(ERROR)
      << "variable declaration with"
         " annotation of type squarets"
         " must have not empty initializer";
    return clang_utils::SourceTransformResult{nullptr};
  }

  // remove annotation from source file
  // replacing it with callback result
  {
    clang_utils::expandLocations(startLoc, endLoc, rewriter);

    rewriter.ReplaceText(
      clang::SourceRange(startLoc, endLoc)
      , "R\"rawdata(1233434)rawdata\"");
  }

  return clang_utils::SourceTransformResult{nullptr};
}

} // namespace plugin
