#include "concurrent_ll_trie/concurrent_ll_trie.hpp"
#include "ll_trie/ll_trie.hpp"
#include <absl/container/btree_map.h>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <atomic>
#include <thread>

namespace {

using SteadyClock = std::chrono::steady_clock;

// Match test_random workload scale (adjust for faster smoke runs if needed).
constexpr int FACTOR = 1000;
constexpr int MAX_KEY = FACTOR / 10;
constexpr int SCAN_MAX_LENGTH = FACTOR / 100;
constexpr int ITERATIONS = 1000 * FACTOR;

std::atomic<int> ITER = 0;

void random_iteration_monitor() {
    int last_iter = ITER;
    while (ITER < ITERATIONS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << ITER - last_iter << " iterations per second\n";
        last_iter = ITER;
    }
}

std::string key_from_int(int n) {
    const int width = static_cast<int>(std::to_string(MAX_KEY).size());
    std::string s = std::to_string(n);
    if (static_cast<int>(s.size()) >= width)
        return s;
    return std::string(static_cast<size_t>(width - s.size()), '0') + s;
}

/** Random mix: 10% erase; remaining 90% split evenly among find,
 * insert_or_assign, and lower_bound scans. */
template <typename Map> double benchmark_map_workload() {
    ITER = 0;
    std::thread monitor_thread(random_iteration_monitor);
    Map map;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> op_dist(0, 99);
    std::uniform_int_distribution<int> key_dist(0, MAX_KEY);
    std::uniform_int_distribution<int> scan_len_dist(1, SCAN_MAX_LENGTH);

    volatile std::uint64_t sink = 0;

    const auto t0 = SteadyClock::now();
    for (ITER = 0; ITER < ITERATIONS; ++ITER) {
        const int op = op_dist(rng);
        const int k = key_dist(rng);
        const std::string key = key_from_int(k);

        if (op < 10) {
            sink += static_cast<std::uint64_t>(map.erase(key));
        } else if (op < 40) {
            auto it = map.find(key);
            sink += (it != map.end()) ? 1u : 0u;
        } else if (op < 70) {
            map.insert_or_assign(key, key);
        } else {
            const int scan_start = key_dist(rng);
            const int scan_count = scan_len_dist(rng);
            const std::string start_key = key_from_int(scan_start);
            int scanned = 0;
            for (auto it = map.lower_bound(start_key);
                 it != map.end() && scanned < scan_count; ++it) {
                sink += (*it).first.size();
                ++scanned;
            }
        }
    }
    const auto t1 = SteadyClock::now();
    (void)sink;

    monitor_thread.join();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

/** Same ops as benchmark_map_workload, using ConcurrentLLTrie locking API. */
template <typename ValueType,
          template <typename K, typename V> class BackendMap>
double benchmark_concurrent_ll_trie_workload() {
    ITER = 0;
    std::thread monitor_thread(random_iteration_monitor);
    concurrent_ll_trie::ConcurrentLLTrie<ValueType, BackendMap> map;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> op_dist(0, 99);
    std::uniform_int_distribution<int> key_dist(0, MAX_KEY);
    std::uniform_int_distribution<int> scan_len_dist(1, SCAN_MAX_LENGTH);

    volatile std::uint64_t sink = 0;

    const auto t0 = SteadyClock::now();
    for (ITER = 0; ITER < ITERATIONS; ++ITER) {
        const int op = op_dist(rng);
        const int k = key_dist(rng);
        const std::string key = key_from_int(k);

        if (op < 10) {
            sink += static_cast<std::uint64_t>(map.erase(key));
        } else if (op < 40) {
            auto it = map.find(key);
            sink += (it != map.shared_end()) ? 1u : 0u;
            it.unlock_shared();
        } else if (op < 70) {
            auto p = map.insert_or_assign(key, key);
            p.first.unlock();
        } else {
            const int scan_start = key_dist(rng);
            const int scan_count = scan_len_dist(rng);
            const std::string start_key = key_from_int(scan_start);
            int scanned = 0;
            auto it = map.lower_bound(start_key);
            for (; it != map.shared_end() && scanned < scan_count; ++it) {
                sink += (*it).first.size();
                ++scanned;
            }
            it.unlock_shared();
        }
    }
    const auto t1 = SteadyClock::now();
    (void)sink;

    monitor_thread.join();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

} // namespace

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    std::cout << "map benchmark: " << ITERATIONS
              << " iterations (10% erase; 30% find; 30% insert_or_assign; 30% "
                 "lower_bound scan)\n";
    std::cout << "MAX_KEY=" << MAX_KEY << " SCAN_MAX_LENGTH=" << SCAN_MAX_LENGTH
              << "\n\n";

    const double ms_std_map =
        benchmark_map_workload<std::map<std::string, std::string>>();
    std::cout << "std::map<std::string,std::string>:              "
              << ms_std_map << " ms\n";

    const double ms_btree_map =
        benchmark_map_workload<absl::btree_map<std::string, std::string>>();
    std::cout << "absl::btree_map<string,string>:                  "
              << ms_btree_map << " ms\n";

    const double ms_ll_trie = benchmark_map_workload<
        ll_trie::LLTrie<std::string, boost::container::flat_map>>();
    std::cout << "ll_trie::LLTrie<string,boost::container::flat_map>: "
              << ms_ll_trie << " ms\n";

    const double ms_concurrent_ll_trie =
        benchmark_concurrent_ll_trie_workload<std::string,
                                              boost::container::flat_map>();
    std::cout << "concurrent_ll_trie::ConcurrentLLTrie<string,flat_map>: "
              << ms_concurrent_ll_trie << " ms\n";

    return 0;
}
