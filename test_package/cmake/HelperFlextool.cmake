find_package(flextool MODULE REQUIRED)

find_program(flextool flextool NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
message(STATUS "find_program for flextool ${flextool}")

if(${flextool} STREQUAL "")
    message(FATAL_ERROR "flextool not found ${flextool}")
endif()
