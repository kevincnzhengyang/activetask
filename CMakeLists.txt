# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# speech_recognition should be the first
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/components/activetask
    ${CMAKE_CURRENT_SOURCE_DIR}/components/blackboard
    ${CMAKE_CURRENT_SOURCE_DIR}/components/network
    )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(activetask)