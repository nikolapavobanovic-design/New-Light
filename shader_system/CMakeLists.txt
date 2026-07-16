cmake_minimum_required(VERSION 3.16)
project(ShaderSystem VERSION 1.0.0 LANGUAGES CXX)

# Require C++17 for std::filesystem (used in future extensions) and
# structured bindings in the manager implementation.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Core library
# ---------------------------------------------------------------------------
add_library(ShaderSystemLib STATIC
    ShaderTypes.cpp
    ShaderCompiler.cpp
    ShaderManager.cpp
)

target_include_directories(ShaderSystemLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable strict warnings
if(MSVC)
    target_compile_options(ShaderSystemLib PRIVATE /W4 /WX)
else()
    target_compile_options(ShaderSystemLib PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter
    )
endif()

# ---------------------------------------------------------------------------
# Example executable
# ---------------------------------------------------------------------------
add_executable(ShaderExample Example.cpp)
target_link_libraries(ShaderExample PRIVATE ShaderSystemLib)

# On POSIX systems the mock filesystem code requires no extra libraries;
# add platform-specific link targets here if needed.
if(WIN32)
    # D3DCompiler.lib would be linked here for a real DirectX backend:
    # target_link_libraries(ShaderSystemLib PRIVATE d3dcompiler dxgi d3d11)
endif()

# ---------------------------------------------------------------------------
# Installation (optional)
# ---------------------------------------------------------------------------
install(TARGETS ShaderSystemLib ShaderExample
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES
    ShaderTypes.h
    ShaderCompiler.h
    ShaderManager.h
    DESTINATION include/ShaderSystem
)
install(DIRECTORY example_shaders DESTINATION share/ShaderSystem)
