cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(09CubeMap "Samples")

target_link_libraries(09CubeMap glad glfw assimp Core)