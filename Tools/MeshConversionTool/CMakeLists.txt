cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(MeshConversionTool "Tools")

target_link_libraries(MeshConversionTool assimp meshoptimizer argh Core)