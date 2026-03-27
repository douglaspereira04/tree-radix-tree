# tree-radix-tree — header-only radix trie library (CMake INTERFACE target).
#
# Include from another project with FetchContent, for example:
#
#   include(FetchContent)
#   FetchContent_Declare(
#     tree-radix-tree
#     GIT_REPOSITORY https://github.com/<user>/tree-radix-tree.git
#     GIT_TAG        main
#   )
#   FetchContent_MakeAvailable(tree-radix-tree)
#   target_link_libraries(my_target PRIVATE tree_radix_tree::trie-kv-core)
#
# Optional: set -DTRIE_KV_BUILD_TESTS=ON when building this repo as the top-level
# project to build tests and benchmarks (default ON when top-level, OFF when a
# dependency).

cmake_minimum_required(VERSION 3.20)

project(tree-radix-tree VERSION 1.0.0 LANGUAGES CXX)

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

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GRAPHVIZ REQUIRED IMPORTED_TARGET libgvc)

# Header-only interface library (works as top-level or via FetchContent/add_subdirectory)
add_library(trie-kv-core INTERFACE)
add_library(tree_radix_tree::trie-kv-core ALIAS trie-kv-core)

target_compile_features(trie-kv-core INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(trie-kv-core INTERFACE
        -Wall -Wextra -Wpedantic
        ${AVX2_FLAG}
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O2>
    )
endif()

target_include_directories(trie-kv-core INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(trie-kv-core INTERFACE PkgConfig::GRAPHVIZ)

option(DEBUG_LOG "Enable LLTrie debug logging (UTILS_LOG macro)" OFF)
if(DEBUG_LOG)
    target_compile_definitions(trie-kv-core INTERFACE DEBUG_LOG=1)
endif()

# Default: build tests/examples when this is the top-level project; skip when used as a dependency.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(TRIE_KV_BUILD_TESTS "Build tests, benchmarks, and examples" ON)
else()
    option(TRIE_KV_BUILD_TESTS "Build tests, benchmarks, and examples" OFF)
endif()

if(TRIE_KV_BUILD_TESTS)
    find_package(Boost CONFIG REQUIRED COMPONENTS container)
    find_package(absl CONFIG REQUIRED)

    # Minimal executable to verify build
    add_executable(trie-kv-main main.cpp)
    target_link_libraries(trie-kv-main PRIVATE trie-kv-core)
    target_compile_features(trie-kv-main PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(trie-kv-main PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(interactive test/interactive.cpp)
    target_link_libraries(interactive PRIVATE trie-kv-core)
    target_compile_features(interactive PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(interactive PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_include_directories(interactive PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # LLTrie map interface tests
    add_executable(test_map test/test_map.cpp)
    target_link_libraries(test_map PRIVATE trie-kv-core)
    target_compile_features(test_map PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_map PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_include_directories(test_map PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(test_random test/test_random.cpp)
    target_link_libraries(test_random PRIVATE trie-kv-core Boost::container)
    target_compile_features(test_random PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_random PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_include_directories(test_random PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(test_random_concurrent test/test_random_concurrent.cpp)
    target_link_libraries(test_random_concurrent PRIVATE trie-kv-core Boost::container)
    target_compile_features(test_random_concurrent PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_random_concurrent PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_include_directories(test_random_concurrent PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(test_concurrent_ll_trie test/test_concurrent_ll_trie.cpp)
    target_link_libraries(test_concurrent_ll_trie PRIVATE trie-kv-core)
    target_compile_features(test_concurrent_ll_trie PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_concurrent_ll_trie PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_include_directories(test_concurrent_ll_trie PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Benchmarks
    add_executable(benchmark_prefix benchmarks/benchmark_prefix.cpp)
    target_link_libraries(benchmark_prefix PRIVATE trie-kv-core)
    target_include_directories(benchmark_prefix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(benchmark_map benchmarks/benchmark_map.cpp)
    target_link_libraries(benchmark_map PRIVATE trie-kv-core Boost::container absl::btree)
    target_compile_features(benchmark_map PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(benchmark_map PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_include_directories(benchmark_map PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(benchmark_map_threads benchmarks/benchmark_map_threads.cpp)
    target_link_libraries(benchmark_map_threads PRIVATE trie-kv-core Boost::container absl::btree)
    target_compile_features(benchmark_map_threads PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(benchmark_map_threads PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    target_include_directories(benchmark_map_threads PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Install layout (optional: find_package after install, or vendoring with CMake path)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS trie-kv-core
    EXPORT TreeRadixTreeTargets
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TreeRadixTreeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/TreeRadixTreeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TreeRadixTree
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/TreeRadixTreeConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT TreeRadixTreeTargets
    FILE TreeRadixTreeTargets.cmake
    NAMESPACE tree_radix_tree::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TreeRadixTree
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/TreeRadixTreeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/TreeRadixTreeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TreeRadixTree
)
