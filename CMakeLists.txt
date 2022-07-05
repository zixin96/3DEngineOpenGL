cmake_minimum_required(VERSION 3.12)

project(3DOpenGLEngine CXX C)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32)
	set(PYTHON_EXECUTABLE "python")
else()
	set(PYTHON_EXECUTABLE "python3")
endif()

if((NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/.bootstrap.json") OR ("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt" IS_NEWER_THAN "${CMAKE_CURRENT_SOURCE_DIR}/vendor/.bootstrap.json"))
	execute_process(
		COMMAND ${PYTHON_EXECUTABLE} "bootstrap.py"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		RESULT_VARIABLE bootstrap_result
	)
	if (NOT (${bootstrap_result} EQUAL 0))
		message(FATAL_ERROR "Cannot run bootstrap.py")
	endif()
endif()

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories(.)
include_directories(vendor/src)
include_directories(vendor/src/glfw/include)
include_directories(vendor/glad/include)
include_directories(vendor/src/glm)
include_directories(vendor/src/stb)
# Enable this for vulkan engine. 
# Comment this out for OpenGL engine
# include_directories(vendor/src/vulkan/include)

# add vendors
add_subdirectory(vendor/src/glfw)

# add samples
add_subdirectory(Samples/01HelloWorld)
add_subdirectory(Samples/02HelloTriangle)
add_subdirectory(Samples/03Cube)
add_subdirectory(Samples/03CubeOptimized)
add_subdirectory(Samples/04TextureMapping)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")

# package glad as static lib
set(GLAD_SOURCES vendor/glad/src/gl.c)
if(WIN32)
	set(GLAD_SOURCES ${GLAD_SOURCES} vendor/glad/src/wgl.c)
endif()
add_library(glad ${GLAD_SOURCES})

set_property(TARGET glfw          PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET glad          PROPERTY FOLDER "ThirdPartyLibraries") 