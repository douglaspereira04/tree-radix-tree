#include "ll_trie/ll_trie.hpp"
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <string>

namespace {

using SteadyClock = std::chrono::steady_clock;

// Match test_random workload scale (adjust for faster smoke runs if needed).
constexpr int FACTOR = 1000;
constexpr int MAX_KEY = 100 * FACTOR;
constexpr int SCAN_MAX_LENGTH = 1 * FACTOR;
constexpr int ITERATIONS = 1000 * FACTOR;

std::string key_from_int(int n) {
    const int width = static_cast<int>(std::to_string(MAX_KEY).size());
    std::string s = std::to_string(n);
    if (static_cast<int>(s.size()) >= width)
        return s;
    return std::string(static_cast<size_t>(width - s.size()), '0') + s;
}

/** Random mix of find, insert_or_assign, and lower_bound scans (same ops as
 * test_random). */
template <typename Map> double benchmark_map_workload() {
    Map map;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> op_dist(0, 2);
    std::uniform_int_distribution<int> key_dist(0, MAX_KEY);
    std::uniform_int_distribution<int> scan_len_dist(1, SCAN_MAX_LENGTH);

    volatile std::uint64_t sink = 0;

    const auto t0 = SteadyClock::now();
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        const int op = op_dist(rng);
        const int k = key_dist(rng);
        const std::string key = key_from_int(k);

        if (op == 0) {
            auto it = map.find(key);
            sink += (it != map.end()) ? 1u : 0u;
        } else if (op == 1) {
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
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

} // namespace

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    std::cout << "map benchmark: " << ITERATIONS
              << " iterations (find / insert_or_assign / lower_bound scan)\n";
    std::cout << "MAX_KEY=" << MAX_KEY << " SCAN_MAX_LENGTH=" << SCAN_MAX_LENGTH
              << "\n\n";

    const double ms_std_map =
        benchmark_map_workload<std::map<std::string, std::string>>();
    std::cout << "std::map<std::string,std::string>:              "
              << ms_std_map << " ms\n";

    const double ms_flat_map = benchmark_map_workload<
        boost::container::flat_map<std::string, std::string>>();
    std::cout << "boost::container::flat_map<string,string>:       "
              << ms_flat_map << " ms\n";

    const double ms_ll_trie = benchmark_map_workload<
        ll_trie::LLTrie<std::string, boost::container::flat_map>>();
    std::cout << "ll_trie::LLTrie<string,boost::container::flat_map>: "
              << ms_ll_trie << " ms\n";

    return 0;
}
