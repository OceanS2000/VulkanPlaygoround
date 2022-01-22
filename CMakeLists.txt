cmake_minimum_required(VERSION 3.21)
project(VulkanPlayground)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g ${CMAKE_CXX_FLAGS_DEBUG}")

add_compile_options(-Wall -Wextra)
add_compile_options(-fvisibility=hidden)

find_package(PkgConfig MODULE REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Vulkan MODULE REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_subdirectory(src)
add_subdirectory(assets)
add_subdirectory(external)

add_executable(VulkanPlayground main.cpp)
target_link_libraries(VulkanPlayground PRIVATE BaseEngine)
add_dependencies(VulkanPlayground shaders)
