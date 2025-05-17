# CMakeList.txt : CMake project for JIukaviyEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.30)
include(FetchContent)
project(JIukaviyEngine)

set (SOURCE
	"Tools/KeyId.ixx"
	"Display/Display.ixx"
	"Display/DisplayManager.ixx"
	"Platforms/Windows/DisplayManagerWindows.ixx" 
	"Platforms/GlfwWrapper/GlfwWrapper.ixx"
	"Tools/RangeHelpers.ixx"
	"Tools/Pointers.ixx"
	"Tools/Variant.ixx"
	"Tools/GlmFormatter.ixx"
	"Display/Window.ixx"
	"Platforms/Windows/WindowWindows.ixx"
)

add_library (JIukaviyEngine ${SOURCE} )

set_property(TARGET JIukaviyEngine PROPERTY CXX_STANDARD 23)

FetchContent_Declare(
	glfwpp
	GIT_REPOSITORY git@github.com:janekb04/glfwpp.git
	GIT_TAG        17b0ca06799fec64f63f5006fe45209ce33aace0
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

target_link_libraries(JIukaviyEngine PRIVATE glfwpp)
target_link_libraries(JIukaviyEngine PRIVATE fmt)
target_link_libraries(JIukaviyEngine PRIVATE glm)
