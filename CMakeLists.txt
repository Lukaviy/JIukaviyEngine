# CMakeList.txt : CMake project for JIukaviyEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.30)
include(FetchContent)
project(JIukaviyEngine)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(ENGINE_SOURCES PUBLIC FILE_SET engine_modules TYPE CXX_MODULES FILES
	"src/tools/key_id.ixx"
	"src/tools/pointers.ixx"
	"src/tools/variant.ixx"
	"src/tools/glm_formatter.ixx"
	"src/tools/resource_system.ixx"
	"src/display/window.ixx"
	"src/display/display.ixx"
	"src/engine/application.ixx"
)

set (PLATFROM_SOURCES PRIVATE FILE_SET platform_modules TYPE CXX_MODULES FILES
	"src/platforms/glfw_vulkan/application_glfw_vulkan.ixx"
)

add_library(JIukaviyEngine)
target_sources(JIukaviyEngine ${ENGINE_SOURCES} ${PLATFROM_SOURCES})
set_target_properties(JIukaviyEngine PROPERTIES COMPILE_WARNING_AS_ERROR TRUE)

FetchContent_Declare(
	glfwpp
	GIT_REPOSITORY git@github.com:Lukaviy/glfwpp.git
	GIT_TAG        264b76abdd8ea3836ace92013809db5154fc51be # dev
)

FetchContent_Declare(
	glm
	GIT_REPOSITORY git@github.com:icaven/glm.git
	GIT_TAG        dca38025fba63bb9284023e6de55f756b9e37cec # master
)

FetchContent_Declare(
	fmt
	GIT_REPOSITORY git@github.com:fmtlib/fmt.git
	GIT_TAG        40626af88bd7df9a5fb80be7b25ac85b122d6c21 # 11.2.0
)

FetchContent_Declare(
	spdlog
	GIT_REPOSITORY git@github.com:gabime/spdlog.git
	GIT_TAG        b18a234ed6af38638678a3338c0d7ed90210ae6c # 1.15.3
)

FetchContent_Declare(
	taskflow
	GIT_REPOSITORY git@github.com:taskflow/taskflow.git
	GIT_TAG        c6d0d6c14c04867911519c04bdc4636c09f81e9b
)

set(GLFWPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfwpp glm fmt spdlog taskflow)

find_package(Vulkan REQUIRED)

target_include_directories(JIukaviyEngine PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(JIukaviyEngine PRIVATE Vulkan::Vulkan)
target_link_libraries(JIukaviyEngine PRIVATE GLFWPP)
target_link_libraries(JIukaviyEngine PRIVATE glm)
target_link_libraries(JIukaviyEngine PRIVATE fmt::fmt)
target_link_libraries(JIukaviyEngine PUBLIC spdlog::spdlog)
target_link_libraries(JIukaviyEngine PUBLIC Taskflow)

# EDITOR

add_executable(Editor)

set (EDITOR_SOURCES PRIVATE FILE_SET editor_modules TYPE CXX_MODULES FILES
	"src/editor/main.ixx"
)

target_sources(Editor ${EDITOR_SOURCES})
target_link_libraries(Editor JIukaviyEngine)

set_property(TARGET Editor 
	PROPERTY WIN32_EXECUTABLE TRUE
)

# Shaders

add_subdirectory(res/shaders)

add_dependencies(Editor Shaders)
