cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(07VertexPulling "Samples")

target_link_libraries(07VertexPulling glad glfw assimp Core)