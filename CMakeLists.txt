cmake_minimum_required(VERSION 3.20)

project(VulkanLearning)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan
find_package(Vulkan REQUIRED)

# GLFW
add_subdirectory(lib/glfw)

# GLM
add_subdirectory(lib/glm)

# Executable
add_executable(VulkanLearning
    # executables
    src/main.cpp

    # headers
    include/pch.h
        src/Triangle.cpp
)

# Precompiled header
target_precompile_headers(VulkanLearning PRIVATE
    include/pch.h
)

# Include directories
target_include_directories(VulkanLearning PRIVATE
    include
)

# Libraries
target_link_libraries(VulkanLearning PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)