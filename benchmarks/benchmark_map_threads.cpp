#include "concurrent_ll_trie/concurrent_ll_trie.hpp"
#include "ll_radix_tree/ll_radix_tree.hpp"
#include "ll_trie/ll_trie.hpp"
#include <absl/container/btree_map.h>
#include <atomic>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

using SteadyClock = std::chrono::steady_clock;

// Same globals as benchmark_map.cpp
constexpr int FACTOR = 1000;
constexpr int MAX_KEY = 100 * FACTOR;
constexpr int SCAN_MAX_LENGTH = FACTOR / 10;
constexpr int ITERATIONS = 1000 * FACTOR;

constexpr int NUM_THREADS = 16;

/** Mean delay (µs) for Poisson-distributed “thinking” while still holding
 *  map / iterator locks (sampled per operation). */
int g_mean_think_before_release_us = 10;

/** Mean delay (µs) for Poisson-distributed “thinking” between operations
 *  (after locks are released). */
int g_mean_think_between_ops_us = 10;

std::atomic<int> ITER = 0;

void spin_microseconds(int us) {
    if (us <= 0)
        return;
    const auto start = SteadyClock::now();
    const auto deadline = start + std::chrono::microseconds(us);
    while (SteadyClock::now() < deadline) {
    }
}

void spin_poisson_microseconds(std::mt19937 &rng,
                               std::poisson_distribution<int> &dist) {
    spin_microseconds(dist(rng));
}

void random_iteration_monitor() {
    int last_iter = ITER.load(std::memory_order_relaxed);
    while (ITER.load(std::memory_order_relaxed) < ITERATIONS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        const int cur = ITER.load(std::memory_order_relaxed);
        std::cout << cur - last_iter << " iterations per second\n";
        last_iter = cur;
    }
}

std::string key_from_int(int n) { return std::to_string(n); }

/** Same workload as benchmark_map.cpp; N threads share one map and \c ITER.
 * Non-thread-safe maps use \c mtx: exclusive for insert/erase, shared for find
 * and scans. */
template <typename Map>
double benchmark_map_workload_threaded(int num_threads, std::shared_mutex &mtx,
                                       Map &map) {
    ITER.store(0, std::memory_order_relaxed);
    std::thread monitor_thread(random_iteration_monitor);
    std::vector<std::thread> workers;

    const auto t0 = SteadyClock::now();
    workers.reserve(static_cast<size_t>(num_threads));
    for (int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&, t]() {
            std::mt19937 rng(42u + static_cast<unsigned>(t) * 7919u);
            std::uniform_int_distribution<int> op_dist(0, 99);
            std::uniform_int_distribution<int> key_dist(0, MAX_KEY);
            std::uniform_int_distribution<int> scan_len_dist(1,
                                                             SCAN_MAX_LENGTH);
            std::poisson_distribution<int> think_before_release_dist(
                static_cast<double>(g_mean_think_before_release_us));
            std::poisson_distribution<int> think_between_ops_dist(
                static_cast<double>(g_mean_think_between_ops_us));
            std::uint64_t sink = 0;

            while (true) {
                const int i = ITER.fetch_add(1, std::memory_order_relaxed);
                if (i >= ITERATIONS)
                    break;

                const int op = op_dist(rng);
                const int k = key_dist(rng);
                const std::string key = key_from_int(k);

                if (op < 10) {
                    std::unique_lock<std::shared_mutex> lk(mtx);
                    sink += static_cast<std::uint64_t>(map.erase(key));
                    spin_poisson_microseconds(rng, think_before_release_dist);
                } else if (op < 40) {
                    std::shared_lock<std::shared_mutex> slk(mtx);
                    auto it = map.find(key);
                    sink += (it != map.end()) ? 1u : 0u;
                    spin_poisson_microseconds(rng, think_before_release_dist);
                } else if (op < 70) {
                    std::unique_lock<std::shared_mutex> lk(mtx);
                    map.insert_or_assign(key, key);
                    spin_poisson_microseconds(rng, think_before_release_dist);
                } else {
                    const int scan_start = key_dist(rng);
                    const int scan_count = scan_len_dist(rng);
                    const std::string start_key = key_from_int(scan_start);
                    std::shared_lock<std::shared_mutex> slk(mtx);
                    int scanned = 0;
                    for (auto it = map.lower_bound(start_key);
                         it != map.end() && scanned < scan_count; ++it) {
                        sink += static_cast<std::uint64_t>((*it).first.size());
                        ++scanned;
                    }
                    spin_poisson_microseconds(rng, think_before_release_dist);
                }

                spin_poisson_microseconds(rng, think_between_ops_dist);
            }
            (void)sink;
        });
    }

    for (auto &th : workers)
        th.join();
    const auto t1 = SteadyClock::now();

    monitor_thread.join();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

template <typename Map> double run_mutex_map_benchmark(int num_threads) {
    std::shared_mutex mtx;
    Map map;
    return benchmark_map_workload_threaded(num_threads, mtx, map);
}

template <typename ValueType,
          template <typename K, typename V> class BackendMap>
double benchmark_concurrent_ll_trie_workload_threaded(int num_threads) {
    ITER.store(0, std::memory_order_relaxed);
    std::thread monitor_thread(random_iteration_monitor);
    concurrent_ll_trie::ConcurrentLLTrie<ValueType, BackendMap> map;
    std::vector<std::thread> workers;

    const auto t0 = SteadyClock::now();
    workers.reserve(static_cast<size_t>(num_threads));
    for (int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&, t]() {
            std::mt19937 rng(42u + static_cast<unsigned>(t) * 7919u);
            std::uniform_int_distribution<int> op_dist(0, 99);
            std::uniform_int_distribution<int> key_dist(0, MAX_KEY);
            std::uniform_int_distribution<int> scan_len_dist(1,
                                                             SCAN_MAX_LENGTH);
            std::poisson_distribution<int> think_before_release_dist(
                static_cast<double>(g_mean_think_before_release_us));
            std::poisson_distribution<int> think_between_ops_dist(
                static_cast<double>(g_mean_think_between_ops_us));
            std::uint64_t sink = 0;

            while (true) {
                const int i = ITER.fetch_add(1, std::memory_order_relaxed);
                if (i >= ITERATIONS)
                    break;

                const int op = op_dist(rng);
                const int k = key_dist(rng);
                const std::string key = key_from_int(k);

                if (op < 10) {
                    sink += static_cast<std::uint64_t>(map.erase(key));
                    /* Locks are released inside erase(); spin matches other
                     * ops. */
                    spin_poisson_microseconds(rng, think_before_release_dist);
                } else if (op < 40) {
                    auto it = map.find(key);
                    sink += (it != map.shared_end()) ? 1u : 0u;
                    spin_poisson_microseconds(rng, think_before_release_dist);
                    it.unlock_shared();
                } else if (op < 70) {
                    auto p = map.insert_or_assign(key, key);
                    spin_poisson_microseconds(rng, think_before_release_dist);
                    p.first.unlock();
                } else {
                    const int scan_start = key_dist(rng);
                    const int scan_count = scan_len_dist(rng);
                    const std::string start_key = key_from_int(scan_start);
                    int scanned = 0;
                    auto it = map.lower_bound(start_key);
                    for (; it != map.shared_end() && scanned < scan_count;
                         ++it) {
                        sink += static_cast<std::uint64_t>((*it).first.size());
                        ++scanned;
                    }
                    spin_poisson_microseconds(rng, think_before_release_dist);
                    it.unlock_shared();
                }

                spin_poisson_microseconds(rng, think_between_ops_dist);
            }
            (void)sink;
        });
    }

    for (auto &th : workers)
        th.join();
    const auto t1 = SteadyClock::now();

    monitor_thread.join();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

} // namespace

int main(int argc, char **argv) {
    int num_threads = NUM_THREADS;
    if (argc >= 2) {
        const int n = std::atoi(argv[1]);
        if (n > 0)
            num_threads = n;
    }

    std::cout << "map benchmark (threaded): " << ITERATIONS << " iterations, "
              << num_threads
              << " threads (10% erase; 30% find; 30% insert_or_assign; 30% "
                 "lower_bound scan)\n";
    std::cout << "MAX_KEY=" << MAX_KEY << " SCAN_MAX_LENGTH=" << SCAN_MAX_LENGTH
              << "\n";
    std::cout << "think Poisson means (µs): before_release="
              << g_mean_think_before_release_us
              << " between_ops=" << g_mean_think_between_ops_us << "\n\n";

    const double ms_std_map =
        run_mutex_map_benchmark<std::map<std::string, std::string>>(
            num_threads);
    std::cout << "std::map<std::string,std::string> (+mutex):        "
              << ms_std_map << " ms\n";

    const double ms_btree_map =
        run_mutex_map_benchmark<absl::btree_map<std::string, std::string>>(
            num_threads);
    std::cout << "absl::btree_map<string,string> (+mutex):           "
              << ms_btree_map << " ms\n";

    const double ms_ll_trie = run_mutex_map_benchmark<
        ll_trie::LLTrie<std::string, boost::container::flat_map>>(num_threads);
    std::cout << "ll_trie::LLTrie<string,flat_map> (+mutex):          "
              << ms_ll_trie << " ms\n";

    const double ms_ll_radix = run_mutex_map_benchmark<
        ll_radix_tree::LLRadixTree<std::string, boost::container::flat_map>>(
        num_threads);
    std::cout << "ll_radix_tree::LLRadixTree<string,flat_map> (+mutex): "
              << ms_ll_radix << " ms\n";

    const double ms_concurrent_ll_trie =
        benchmark_concurrent_ll_trie_workload_threaded<
            std::string, boost::container::flat_map>(num_threads);
    std::cout << "concurrent_ll_trie::ConcurrentLLTrie<string,flat_map>: "
              << ms_concurrent_ll_trie << " ms\n";

    return 0;
}
