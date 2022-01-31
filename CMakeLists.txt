cmake_minimum_required(VERSION 3.17)
message("Vulkan subpasses example: using cmake version: '" ${CMAKE_VERSION} "'")
set (CMAKE_CXX_STANDARD 17)

project(VulkanSubpasses LANGUAGES CXX VERSION 0.1.0)

add_subdirectory(vulkan_example)
add_subdirectory(app)

message("cmake_study: processing 'CMakeLists.txt' and configuring 'CMakeCache.txt'")
message("cmake_study: generating native solution at '" ${CMAKE_BINARY_DIR} "'")