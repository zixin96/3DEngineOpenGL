cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(02HelloTriangle "Samples")

target_link_libraries(02HelloTriangle glad glfw)