cmake_minimum_required(VERSION 3.18.0)
message("VulkanExample: using cmake version: '" ${CMAKE_VERSION} "'")

project(VulkanExample LANGUAGES CXX VERSION 0.1.0)

# create "VulkanExample" target library
add_library(${PROJECT_NAME}) 

# Specify "VulkanExample" library
target_sources(${PROJECT_NAME} PRIVATE 
source/VulkanExample.cpp
include/VulkanExample.h
) 

# 3rd party libraries: find and execute cmake modules. (create cmake-libraries and variables)
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED) 
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader REQUIRED)

# directories for included files (1st/3rd party libraries)
target_include_directories(${PROJECT_NAME}
PUBLIC ${PROJECT_SOURCE_DIR}/include	
PUBLIC ${VULKAN_INCLUDE_DIRS}
PRIVATE ${Stb_INCLUDE_DIR}
PRIVATE ${tinyobjloader_INCLUDE_DIR}	
)

# link static libraries (3rd party)
target_link_libraries(${PROJECT_NAME} 
Vulkan::Vulkan
glfw 
tinyobjloader::tinyobjloader
glm::glm
)

