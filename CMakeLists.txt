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
    include/pch.cpp
    src/Triangle.cpp
        src/input.cpp
        src/input.cpp
        include/input.h
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

# Shader compiler
find_program(GLSLC glslc REQUIRED)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/resources/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)

set(SHADERS
        ${SHADER_DIR}/shader1/shader1.frag
        ${SHADER_DIR}/shader1/shader1.vert

        ${SHADER_DIR}/shader2/shader2.frag
        ${SHADER_DIR}/shader2/shader2.vert

        ${SHADER_DIR}/shader3/shader3.frag
        ${SHADER_DIR}/shader3/shader3.vert

        ${SHADER_DIR}/shader4/shader4.frag
        ${SHADER_DIR}/shader4/shader4.vert
)

set(SPIRV_SHADERS)

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)

    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
            COMMAND ${GLSLC} ${SHADER} -o ${SPIRV}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME}"
    )

    list(APPEND SPIRV_SHADERS ${SPIRV})
endforeach()

add_custom_target(Shaders DEPENDS ${SPIRV_SHADERS})

add_dependencies(VulkanLearning Shaders)