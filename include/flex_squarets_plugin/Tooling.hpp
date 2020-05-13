#pragma once

#include <flexlib/clangUtils.hpp>
#include <flexlib/ToolPlugin.hpp>
#if defined(CLING_IS_ON)
#include "flexlib/ClingInterpreterModule.hpp"
#endif // CLING_IS_ON

#include <base/logging.h>
#include <base/sequenced_task_runner.h>

namespace plugin {

class Tooling {
public:
  Tooling(
    const ::plugin::ToolPlugin::Events::RegisterAnnotationMethods& event
#if defined(CLING_IS_ON)
    , ::cling_utils::ClingInterpreter* clingInterpreter
#endif // CLING_IS_ON
  );

  ~Tooling();

  clang_utils::SourceTransformResult
    squarets(
      const clang_utils::SourceTransformOptions& sourceTransformOptions);

private:
  ::clang_utils::SourceTransformRules* sourceTransformRules_;

#if defined(CLING_IS_ON)
  ::cling_utils::ClingInterpreter* clingInterpreter_;
#endif // CLING_IS_ON

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(Tooling);
};

} // namespace plugin
