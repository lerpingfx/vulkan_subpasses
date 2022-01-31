cmake_minimum_required(VERSION 3.17)
message("app: using cmake version: '" ${CMAKE_VERSION} "'")
project(app LANGUAGES CXX VERSION 0.1.0)

message("app: C++ Compiler: '" ${CMAKE_CXX_COMPILER} "'")
message("app: build version '" ${PROJECT_VERSION} "'")
message("app: major version '" ${PROJECT_VERSION_MAJOR} "' minor version: '" ${PROJECT_VERSION_MINOR} "' patch version: '" ${PROJECT_VERSION_PATCH})

add_executable(VulkanExampleApp main.cpp)

# directories for included files for VulkanExampleApp
target_include_directories(VulkanExampleApp
PUBLIC ${CMAKE_SOURCE_DIR}/vulkan_example/include
)

# link static libraries for VulkanExampleApp (1st party)
target_link_libraries(VulkanExampleApp
VulkanExample
)

# copy asset folder to build folder
add_custom_command(TARGET VulkanExampleApp PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${PROJECT_SOURCE_DIR}/assets/ ${PROJECT_BINARY_DIR}/assets)