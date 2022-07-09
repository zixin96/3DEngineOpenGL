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

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")

set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")

set(MESHOPT_BUILD_DEMO OFF CACHE BOOL "")
set(MESHOPT_BUILD_TOOLS OFF CACHE BOOL "")
set(MESHOPT_BUILD_SHARED_LIBS OFF CACHE BOOL "")

# for Argh
set(BUILD_TESTS OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")

# Enable this for vulkan engine. 
# Comment this out for OpenGL engine
# include_directories(vendor/src/vulkan/include)

# add vendors
add_subdirectory(vendor/src/glfw)
add_subdirectory(vendor/src/assimp)
add_subdirectory(vendor/src/meshoptimizer)
add_subdirectory(vendor/src/argh)

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories(.)
include_directories(vendor/src)
include_directories(vendor/src/glfw/include)
include_directories(vendor/glad/include)
include_directories(vendor/src/glm)
include_directories(vendor/src/stb)
include_directories(vendor/src/imgui)
include_directories(vendor/src/assimp/include)
include_directories(vendor/src/gli)
include_directories(vendor/src/rapidjson/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/vendor/src/assimp/include)
include_directories(Core)

# package glad as static lib
set(GLAD_SOURCES vendor/glad/src/gl.c)
if(WIN32)
	set(GLAD_SOURCES ${GLAD_SOURCES} vendor/glad/src/wgl.c)
endif()
add_library(glad ${GLAD_SOURCES})

set_property(TARGET glfw          PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET update_mappings          PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET glad          PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET assimp        PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET meshoptimizer PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET argh PROPERTY FOLDER "ThirdPartyLibraries")
if(WIN32)
set_property(TARGET zlibstatic    PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET uninstall PROPERTY FOLDER "ThirdPartyLibraries")
endif()

add_subdirectory(Core)

# add samples
add_subdirectory(Samples/01HelloWorld)
add_subdirectory(Samples/02HelloTriangle)
add_subdirectory(Samples/03Cube)
add_subdirectory(Samples/03CubeOptimized)
add_subdirectory(Samples/04TextureMapping)
add_subdirectory(Samples/05ImGui)
add_subdirectory(Samples/06Duck)
add_subdirectory(Samples/07VertexPulling)
add_subdirectory(Samples/08VertexIndexPulling)
add_subdirectory(Samples/09CubeMap)
add_subdirectory(Samples/10MeshOpt)
add_subdirectory(Samples/11DebugGrid)
add_subdirectory(Samples/12MeshRenderer)
add_subdirectory(Samples/13Tessellation)
add_subdirectory(Samples/14PBR)
add_subdirectory(Samples/15LargeScene)

add_subdirectory(Tools/MeshConversionTool)
add_subdirectory(Tools/SceneConversionTool)
