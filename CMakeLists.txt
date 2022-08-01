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
	"Tools/RangeHelpers.ixx" 
	"Tools/Result.ixx"
	"Tools/Pointers.ixx"
	"Tools/Variant.ixx"
	"Tools/GlmFormatter.ixx"
 "Display/Window.ixx" "Platforms/Windows/WindowWindows.ixx")

add_library (JIukaviyEngine ${SOURCE} )

set_property(TARGET JIukaviyEngine PROPERTY CXX_STANDARD 20)

add_subdirectory("libs/glfw")
add_subdirectory("libs/glm")
add_subdirectory("libs/expected")
add_subdirectory("libs/fmt")

target_link_libraries(JIukaviyEngine PRIVATE glfw)
target_link_libraries(JIukaviyEngine PRIVATE fmt)
target_include_directories(JIukaviyEngine PRIVATE "libs/glfw/include")
target_include_directories(JIukaviyEngine PRIVATE "libs/glm")
target_include_directories(JIukaviyEngine PRIVATE "libs/expected/include")
target_include_directories(JIukaviyEngine PRIVATE "libs/fmt/include")
