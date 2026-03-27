# tree-radix-tree

C++20 **header-only** libraries for ordered string-key maps backed by trie-style structures. Keys are `std::string`; the value type is a template parameter. Child edges use a configurable associative container (default: `std::map<char, …>`), so iteration follows lexicographic key order.

CMake exposes a single interface target: **`tree_radix_tree::trie-kv-core`** (`trie-kv-core`).

## What is included

| Namespace / type | Role |
|------------------|------|
| `ll_trie::LLTrie` | Linked-list **trie**: one character step per edge; full key stored at the value node. |
| `ll_radix_tree::LLRadixTree` | **Radix tree** (compressed paths): common prefixes share one edge label; splits nodes when keys diverge. Same map-like API as `LLTrie`. |
| `concurrent_ll_trie::ConcurrentLLTrie` | Thread-safe trie with **per-node mutexes**; shared (`find`, `count`, `lower_bound`, …) vs exclusive (`insert`, `erase`, `begin`) access patterns. |

**Radix-only extras:** `LLRadixTree` can dump a Graphviz layout and write a **PNG** to the system temp directory via `display_tree()` (requires Graphviz at runtime for layout).

**Utilities:** `include/utils.hpp` provides helpers such as common-prefix length; on x86/x64 the build may pass **`-mavx2`** for SIMD where used.

## Requirements

- **C++20** compiler (GCC or Clang recommended).
- **CMake** ≥ 3.20.
- **pkg-config** and **Graphviz development libraries** (`libgvc` / `libcgraph`): the core target links them so consumers resolve Graphviz when linking.

When this repository is built as the **top-level** project with tests (`TRIE_KV_BUILD_TESTS` defaults **ON**), you also need:

- **Boost** (Container component),
- **Abseil** (e.g. for benchmarks comparing with `absl::btree`).

## Build (this repository)

```bash
mkdir -p build && cd build
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release
cmake --build . -j"$(nproc)"
```

Or use `./build.sh` from the repo root (configures, builds, runs `clang-format`, and executes `./run_tests.sh`).

### CMake options

| Option | Meaning |
|--------|---------|
| `TRIE_KV_BUILD_TESTS` | Build demos, tests, and benchmarks. **ON** when this tree is the top-level project; **OFF** when used as a subdirectory / dependency. |
| `DEBUG_LOG` | Defines `DEBUG_LOG=1` so `UTILS_LOG` macros emit debug output. |

## Use as a dependency

```cmake
include(FetchContent)
FetchContent_Declare(
  tree-radix-tree
  GIT_REPOSITORY https://github.com/<user>/tree-radix-tree.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(tree-radix-tree)
target_link_libraries(my_target PRIVATE tree_radix_tree::trie-kv-core)
```

Install rules also generate `TreeRadixTreeConfig.cmake` for `find_package` after `cmake --install`.

## API overview

The trie and radix types follow **associative-container** patterns close to `std::map<std::string, Value>`:

- **Insertion:** `insert`, `insert_or_assign`, `emplace` — return `std::pair<Iterator, bool>` like `map::insert`.
- **Lookup:** `find`, `count`, `at` (throws if missing).
- **Removal:** `erase(key)`, `erase(iterator)`.
- **Order:** `lower_bound`, `upper_bound`, `begin` / `end` for forward iteration over keys in sorted order.

Iterators dereference to a small **`pair_ref`** type (`.first` is the key string, `.second` is the value) so syntax stays close to `std::pair`.

**Prefix-style scans:** because keys are sorted, a range starting at `lower_bound(prefix)` and stopping when keys no longer begin with `prefix` is the usual pattern (there is no separate `scan` member).

### Minimal example (`LLTrie`)

```cpp
#include "ll_trie/ll_trie.hpp"
#include <string>

int main() {
    ll_trie::LLTrie<std::string> t;
    t.insert({"user:1001", "alice"});
    t.insert_or_assign("user:1001", "bob");

    if (auto it = t.find("user:1001"); it != t.end()) {
        (void)it->second;
    }
    return 0;
}
```

### Radix tree and Graphviz dump

```cpp
#include "ll_radix_tree/ll_radix_tree.hpp"

void demo() {
    ll_radix_tree::LLRadixTree<int> tree;
    tree.insert_or_assign("user:1001", 1);
    tree.display_tree();  // writes PNG under the temp directory (Graphviz)
}
```

### Concurrent trie

Use `find`, `count`, `lower_bound`, `upper_bound`, `shared_begin` / `shared_end` for **reader** paths (shared locks). Use `insert`, `erase`, and exclusive `begin` where the implementation takes **exclusive** locks. Iterator types differ (`SharedIterator` vs `Iterator`); unlock helpers exist where the API holds locks across calls — see `concurrent_ll_trie.hpp` for details.

## Programs built with tests enabled

| Executable | Purpose |
|------------|---------|
| `trie-kv-main` | Smoke test include/link. |
| `interactive` | Interactive shell for manual testing. |
| `test_map` | Map API tests for `LLTrie` and `LLRadixTree`. |
| `test_random` / `test_random_concurrent` | Randomized stress tests (Boost). |
| `test_concurrent_ll_trie` | Concurrent trie tests. |
| `benchmark_*` | Prefix and map benchmarks (Boost / Abseil where linked). |

Run all built `test_*` binaries with `./run_tests.sh` from the repository root (expects a `build/` directory).

## Repository layout

```text
include/
  ll_trie/              LLTrie + node headers
  ll_radix_tree/        LLRadixTree + node headers
  concurrent_ll_trie/   ConcurrentLLTrie + node headers
  utils.hpp             Shared helpers
test/                   Tests and interactive tool source
benchmarks/             Benchmark sources
main.cpp                Minimal demo
cmake/                  Package config template
CMakeLists.txt
```
