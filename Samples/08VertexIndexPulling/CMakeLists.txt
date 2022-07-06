cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(08VertexIndexPulling "Samples")

target_link_libraries(08VertexIndexPulling glad glfw assimp Core)