#pragma once

#include <flex_squarets_plugin/Tooling.hpp>

#include <flexlib/ToolPlugin.hpp>
#if defined(CLING_IS_ON)
#include "flexlib/ClingInterpreterModule.hpp"
#endif // CLING_IS_ON

#include <base/logging.h>
#include <base/sequenced_task_runner.h>

namespace plugin {

/// \note class name must not collide with
/// class names from other loaded plugins
class FlexSquaretsEventHandler {
public:
  FlexSquaretsEventHandler();

  ~FlexSquaretsEventHandler();

  void StringCommand(
    const ::plugin::ToolPlugin::Events::StringCommand& event);

#if defined(CLING_IS_ON)
  void RegisterClingInterpreter(
    const ::plugin::ToolPlugin::Events::RegisterClingInterpreter& event);
#endif // CLING_IS_ON

  void RegisterAnnotationMethods(
    const ::plugin::ToolPlugin::Events::RegisterAnnotationMethods& event);

private:
  std::unique_ptr<Tooling> tooling_;

#if defined(CLING_IS_ON)
  ::cling_utils::ClingInterpreter* clingInterpreter_;
#endif // CLING_IS_ON

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(FlexSquaretsEventHandler);
};

} // namespace plugin
