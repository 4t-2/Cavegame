cmake_minimum_required(VERSION 3.10)

project(CaveGame)

file(GLOB_RECURSE SRC ./src/*.cpp)
file(GLOB_RECURSE AX ./lib/AXIS/src/*.cpp)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER "g++")

# set(CMAKE_BUILD_TYPE Debug) # default build

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE MATCHES Debug)
	set (CMAKE_CXX_FLAGS "-g -fsanitize=address")
	message(STATUS "Building with debug flags")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
	set (CMAKE_CXX_FLAGS "-Ofast -flto=auto")
	message(STATUS "Building with release flags")
endif()

add_executable(CaveGame ${SRC} ${AX})

if(LINUX)
	target_link_libraries(CaveGame -lX11 -lGL -lGLEW -lfreetype -ljsoncpp)
	target_include_directories(CaveGame PUBLIC "/usr/include/freetype2")
endif()

if(WIN32)
	find_package(GLEW REQUIRED)
	find_package(glfw3 REQUIRED)
	find_package(Freetype REQUIRED)
	find_package(jsoncpp REQUIRED)
	target_link_libraries(CaveGame GLEW::GLEW glfw Freetype::Freetype JsonCpp::JsonCpp opengl32)
endif()

include_directories("./lib")
