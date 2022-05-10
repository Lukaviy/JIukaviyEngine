# CMakeList.txt : CMake project for JIukaviyEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.12)

set (SOURCE
	"Tools/KeyId.ixx"
	"Display/Display.ixx"
	"Display/DisplayManager.ixx"
	"Platforms/Windows/DisplayManagerWindows.ixx" 
	"Platforms/GlfwWrapper/GlfwWrapper.ixx"
)
add_library (JIukaviyEngine ${SOURCE})

set_property(TARGET JIukaviyEngine PROPERTY CXX_STANDARD 20)

add_subdirectory("${CMAKE_SOURCE_DIR}/libs/glfw")
