cmake_minimum_required(VERSION 3.16)
project(ShaderSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Library target
# ---------------------------------------------------------------------------
add_library(shader_system STATIC
    ShaderTypes.cpp
    ShaderCompiler.cpp
    ShaderManager.cpp
)

target_include_directories(shader_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# std::filesystem support
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(shader_system PUBLIC stdc++fs)
endif()

# ---------------------------------------------------------------------------
# Example executable
# ---------------------------------------------------------------------------
add_executable(ShaderExample Example.cpp)

target_link_libraries(ShaderExample PRIVATE shader_system)

# Copy example shaders next to the binary so the example can find them.
add_custom_command(TARGET ShaderExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/example_shaders
        $<TARGET_FILE_DIR:ShaderExample>/example_shaders
    COMMENT "Copying example shaders to output directory"
)

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------
if (MSVC)
    target_compile_options(shader_system  PRIVATE /W4)
    target_compile_options(ShaderExample  PRIVATE /W4)
else()
    target_compile_options(shader_system  PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ShaderExample  PRIVATE -Wall -Wextra -Wpedantic)
endif()
