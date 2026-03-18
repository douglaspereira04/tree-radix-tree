cmake_minimum_required(VERSION 3.20)

project(trie-kv VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Core library target (header-only template, no .cpp needed)
add_library(trie-kv-core INTERFACE)

target_compile_features(trie-kv-core INTERFACE cxx_std_20)
target_compile_options(trie-kv-core INTERFACE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
)

target_include_directories(trie-kv-core INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Minimal executable to verify build
add_executable(trie-kv-main main.cpp)
target_link_libraries(trie-kv-main PRIVATE trie-kv-core)
target_compile_features(trie-kv-main PRIVATE cxx_std_20)
target_compile_options(trie-kv-main PRIVATE -Wall -Wextra -Wpedantic)

# Interactive radix tree shell
add_executable(trie-interactive interactive.cpp)
target_link_libraries(trie-interactive PRIVATE trie-kv-core)
target_compile_features(trie-interactive PRIVATE cxx_std_20)
target_compile_options(trie-interactive PRIVATE -Wall -Wextra -Wpedantic)

# Radix tree tests
add_executable(test_radix_tree test/test_radix_tree.cpp)
target_link_libraries(test_radix_tree PRIVATE trie-kv-core)
target_compile_features(test_radix_tree PRIVATE cxx_std_20)
target_compile_options(test_radix_tree PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(test_radix_tree PRIVATE ${CMAKE_SOURCE_DIR})
