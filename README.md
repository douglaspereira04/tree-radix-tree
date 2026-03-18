# TreeRadixTree

TreeRadixTree is a C++20 header-only radix tree implementation for key-value storage. It provides `put`, `get`, and prefix `scan` operations with a configurable map backend (default: `std::map`).

## Build

### Using the build script

```bash
./build.sh
```

This configures CMake, builds the main executable, interactive shell, and tests.

### Manual build

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

## Targets

- **trie-kv-main**: minimal executable to verify the radix tree build
- **trie-interactive**: interactive shell for manual `put`/`get`/`scan` operations
- **test_radix_tree**: unit tests for the radix tree

## Run

```bash
cd build
./trie-kv-main
./trie-interactive
```

## Tests

```bash
./build.sh
```

Or run tests manually:

```bash
cd build
./test_radix_tree
```

## Usage

The radix tree is header-only. Include and use:

```cpp
#include "radix_tree/radix_tree.hpp"

radix_tree::RadixTree<std::string> tree;
tree.put("user:1001", "value1");
tree.put("user:1002", "value2");

std::string value;
if (tree.get("user:1001", value)) { /* ... */ }

std::vector<std::pair<std::string, std::string>> results;
tree.scan("user:", results);
```

You can customize the internal map type via the second template parameter (default: `std::map`).

## Repository layout

```text
TreeRadixTree/
  include/radix_tree/   Radix tree headers (radix_tree.hpp, node.hpp)
  test/                 Unit tests
  main.cpp              Minimal demo
  interactive.cpp       Interactive shell
  build.sh              Configure/build script
  run_tests.sh          Test runner
  CMakeLists.txt
```
