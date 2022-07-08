cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(06Duck "Samples")

target_link_libraries(06Duck glad glfw assimp Core)