#pragma once

#include <flexlib/clangUtils.hpp>
#include <flexlib/ToolPlugin.hpp>
#if defined(CLING_IS_ON)
#include "flexlib/ClingInterpreterModule.hpp"
#endif // CLING_IS_ON

#include <base/logging.h>
#include <base/sequenced_task_runner.h>

namespace plugin {

/// \note class name must not collide with
/// class names from other loaded plugins
class SquaretsTooling {
public:
  SquaretsTooling(
    const ::plugin::ToolPlugin::Events::RegisterAnnotationMethods& event
#if defined(CLING_IS_ON)
    , ::cling_utils::ClingInterpreter* clingInterpreter
#endif // CLING_IS_ON
  );

  ~SquaretsTooling();

  // extracts template code from annotated varible
  void squarets(
    const std::string& processedAnnotaion
    , clang::AnnotateAttr* annotateAttr
    , const clang_utils::MatchResult& matchResult
    , clang::Rewriter& rewriter
    , const clang::Decl* nodeDecl);

  // extracts template code from file path
  void squaretsFile(
    const std::string& processedAnnotaion
    , clang::AnnotateAttr* annotateAttr
    , const clang_utils::MatchResult& matchResult
    , clang::Rewriter& rewriter
    , const clang::Decl* nodeDecl);

  // executes arbitrary C++ code in Cling interpreter to
  // get template code
  // squarets will generate code from template
  // and append it after annotated variable
  /// \note template will be NOT interpreted by Cling,
  /// but we assume that it will be returned from Cling
  void squaretsCodeAndReplace(
    const std::string& processedAnnotaion
    , clang::AnnotateAttr* annotateAttr
    , const clang_utils::MatchResult& matchResult
    , clang::Rewriter& rewriter
    , const clang::Decl* nodeDecl);

  // executes arbitrary template code in Cling interpreter and
  // replaces original code with template execution result
  // (execution result is arbitrary string, it may be not C++ code)
  /// \note template WILL be interpreted by Cling
  void interpretSquarets(
    const std::string& processedAnnotaion
    , clang::AnnotateAttr* annotateAttr
    , const clang_utils::MatchResult& matchResult
    , clang::Rewriter& rewriter
    , const clang::Decl* nodeDecl);

private:
  ::clang_utils::SourceTransformRules* sourceTransformRules_;

#if defined(CLING_IS_ON)
  ::cling_utils::ClingInterpreter* clingInterpreter_;
#endif // CLING_IS_ON

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SquaretsTooling);
};

} // namespace plugin
