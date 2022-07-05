cmake_minimum_required(VERSION 3.12)

include(../../CommonMacros.txt)

SETUP_APP(04TextureMapping "Samples")

target_link_libraries(04TextureMapping glad glfw)