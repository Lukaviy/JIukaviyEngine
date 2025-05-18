# CMakeList.txt : CMake project for JIukaviyEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.30)
include(FetchContent)
project(JIukaviyEngine)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set (ENGINE_SOURCES PUBLIC FILE_SET engine_modules TYPE CXX_MODULES FILES
	"Tools/KeyId.ixx"
	"Display/Display.ixx"
	"Tools/Pointers.ixx"
	"Tools/Variant.ixx"
	"Tools/GlmFormatter.ixx"
	"Display/Window.ixx"
	"Platforms/Windows/WindowWindows.ixx"
	"Engine/Application.ixx"
)

set (PLATFROM_SOURCES PRIVATE FILE_SET platform_modules TYPE CXX_MODULES FILES
	"Platforms/Windows/ApplicationWindows.ixx"
)

add_library (JIukaviyEngine)
target_sources(JIukaviyEngine ${ENGINE_SOURCES} ${PLATFROM_SOURCES})
set_target_properties(JIukaviyEngine PROPERTIES COMPILE_WARNING_AS_ERROR TRUE)

FetchContent_Declare(
	glfwpp
	GIT_REPOSITORY git@github.com:Lukaviy/glfwpp.git
	GIT_TAG        5fd5a6af24df6660c86ac844a29d145907b462dc # master
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

set(GLFWPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfwpp glm fmt)

target_link_libraries(JIukaviyEngine PRIVATE GLFWPP)
target_link_libraries(JIukaviyEngine PRIVATE fmt)
target_link_libraries(JIukaviyEngine PRIVATE glm)

# EDITOR

add_executable(Editor)

set (EDITOR_SOURCES PRIVATE FILE_SET editor_modules TYPE CXX_MODULES FILES
	"Editor/Main.ixx"
)

target_sources(Editor ${EDITOR_SOURCES})
target_link_libraries(Editor JIukaviyEngine)

#set_property(TARGET Editor 
#	PROPERTY WIN32_EXECUTABLE TRUE
#)
