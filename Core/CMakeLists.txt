cmake_minimum_required(VERSION 3.12)

project(Core CXX C)

include(../CommonMacros.txt)

include_directories(../vendor/src/imgui)

file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)

SETUP_GROUPS("${SRC_FILES}")
SETUP_GROUPS("${HEADER_FILES}")

add_library(Core ${SRC_FILES} ${HEADER_FILES})

set_property(TARGET Core PROPERTY CXX_STANDARD 20)
set_property(TARGET Core PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(Core PUBLIC glad glfw assimp)
