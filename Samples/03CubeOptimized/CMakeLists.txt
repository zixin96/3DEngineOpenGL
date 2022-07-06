cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(03CubeOptimized "Samples")

target_link_libraries(03CubeOptimized glad glfw Core)