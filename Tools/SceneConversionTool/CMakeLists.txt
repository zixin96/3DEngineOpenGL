cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(SceneConversionTool "Tools")

target_link_libraries(SceneConversionTool assimp meshoptimizer argh Core)