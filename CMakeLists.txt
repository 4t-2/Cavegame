cmake_minimum_required(VERSION 3.10)

project(CaveGame)

function(compile_shader stage input_path output_path)
	message("Compiling: ${input_path}")
	execute_process(
		COMMAND glslc -fshader-stage=${stage} -c ${input_path} -o ${output_path}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
	)
endfunction()

file(GLOB VERT_SHADERS "./bin/shader/*Vert.glsl")
file(GLOB FRAG_SHADERS "./bin/shader/*Frag.glsl")

foreach(SHADER ${VERT_SHADERS})
	cmake_path(GET SHADER STEM SHADER_STEM)
	compile_shader("vert" "./shader/${SHADER_STEM}.glsl" "./shader/${SHADER_STEM}.spv")
endforeach()

foreach(SHADER ${FRAG_SHADERS})
	cmake_path(GET SHADER STEM SHADER_STEM)
	compile_shader("frag" "./shader/${SHADER_STEM}.glsl" "./shader/${SHADER_STEM}.spv")
endforeach()

file(GLOB_RECURSE SRC ./src/*.cpp)

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

add_executable(CaveGame src/main.cpp src/World.cpp src/other.cpp src/Mesh.cpp src/installing.cpp)

find_package(jsoncpp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(CaveGame PRIVATE Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)
target_link_libraries(CaveGame PRIVATE glfw)
target_link_libraries(CaveGame PRIVATE imgui::imgui)
target_link_libraries(CaveGame PRIVATE JsonCpp::JsonCpp)
target_link_libraries(CaveGame PRIVATE imgui::imgui)
target_include_directories(CaveGame PRIVATE "lib")
target_include_directories(CaveGame PRIVATE ${Stb_INCLUDE_DIR})

include_directories("./lib")
