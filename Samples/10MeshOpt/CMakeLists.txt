cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(10MeshOpt "Samples")

target_link_libraries(10MeshOpt glad glfw assimp meshoptimizer Core)