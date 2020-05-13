include_guard( DIRECTORY )

list(APPEND flex_squarets_plugin_SOURCES
  ${flex_squarets_plugin_src_DIR}/plugin_main.cc
  ${flex_squarets_plugin_include_DIR}/EventHandler.hpp
  ${flex_squarets_plugin_src_DIR}/EventHandler.cc
  ${flex_squarets_plugin_include_DIR}/Tooling.hpp
  ${flex_squarets_plugin_src_DIR}/Tooling.cc
)
