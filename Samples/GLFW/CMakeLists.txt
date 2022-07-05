cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(GLFW "Samples")

target_link_libraries(GLFW glad glfw)