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

# AVX2 for x86/x64 (Intel/AMD)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|AMD64|i[3-6]86)")
    set(AVX2_FLAG "-mavx2")
else()
    set(AVX2_FLAG "")
endif()

# Core library target (header-only template, no .cpp needed)
add_library(trie-kv-core INTERFACE)

target_compile_features(trie-kv-core INTERFACE cxx_std_20)
target_compile_options(trie-kv-core INTERFACE
    -Wall -Wextra -Wpedantic
    ${AVX2_FLAG}
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

add_executable(interactive test/interactive.cpp)
target_link_libraries(interactive PRIVATE trie-kv-core)
target_compile_features(interactive PRIVATE cxx_std_20)
target_compile_options(interactive PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(interactive PRIVATE ${CMAKE_SOURCE_DIR})

# LLTrie map interface tests
add_executable(test_map test/test_map.cpp)
target_link_libraries(test_map PRIVATE trie-kv-core)
target_compile_features(test_map PRIVATE cxx_std_20)
target_compile_options(test_map PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(test_map PRIVATE ${CMAKE_SOURCE_DIR})

add_executable(test_random test/test_random.cpp)
target_link_libraries(test_random PRIVATE trie-kv-core)
target_compile_features(test_random PRIVATE cxx_std_20)
target_compile_options(test_random PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(test_random PRIVATE ${CMAKE_SOURCE_DIR})

# Benchmarks
add_executable(benchmark_prefix benchmarks/benchmark_prefix.cpp)
target_link_libraries(benchmark_prefix PRIVATE trie-kv-core)
target_include_directories(benchmark_prefix PRIVATE ${CMAKE_SOURCE_DIR})
