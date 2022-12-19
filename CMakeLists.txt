# CMakeList.txt : CMake project for RepairingBridge, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("RepairingBridge")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY /usr/bin)

# Add source to this project's executable.
include_directories(include/)
include_directories(include/Executables/)
add_executable (RepairingBridge "src/RepairingBridge.cpp" "src/Interface.cpp" "src/Log.cpp" "src/Processes.cpp" "include/Executables/DynamicExec.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RepairingBridge PROPERTY CXX_STANDARD 20)
endif()



# TODO: Add tests and install targets if needed.