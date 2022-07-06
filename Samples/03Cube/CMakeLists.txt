cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(03Cube "Samples")

target_link_libraries(03Cube glad glfw Core)