#include "../utils/test_assertions.h"
#include "concurrent_ll_trie/concurrent_ll_trie.hpp"
#include <boost/container/flat_map.hpp>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <vector>

int tests_passed = 0;
int tests_failed = 0;

namespace {

const int FACTOR = 100;
const int MAX_KEY = 100 * FACTOR;
const int SCAN_MAX_LENGTH = 1 * FACTOR;
const int ITERATIONS = 1000 * FACTOR;
std::atomic<int> ITER = 0;

void random_iteration_monitor() {
    int last_iter = ITER;
    while (ITER < ITERATIONS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (ITER == last_iter) {
            std::cerr
                << "test_random: iteration count unchanged for 1s (stuck at "
                << ITER << "), aborting\n";
            std::abort();
        }
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

void test_random() {
    TEST("test_random")

    std::thread monitor_thread(random_iteration_monitor);

    std::map<std::string, std::string> ref;
    concurrent_ll_trie::ConcurrentLLTrie<std::string,
                                         boost::container::flat_map>
        trie;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> op_dist(0, 2);
    std::uniform_int_distribution<int> key_dist(0, MAX_KEY);
    std::uniform_int_distribution<int> scan_len_dist(1, SCAN_MAX_LENGTH);

    for (ITER = 0; ITER < ITERATIONS; ++ITER) {
        try {
            const int op = op_dist(rng);
            const int k = key_dist(rng);
            const std::string key = key_from_int(k);

            if (op == 0) {
                auto rit = ref.find(key);
                auto tit = trie.find(key);
                const bool r_found = (rit != ref.end());
                const bool t_found = (tit != trie.shared_end());
                ASSERT_TRUE(r_found == t_found);
                if (r_found) {
                    ASSERT_STR_EQ(rit->second, tit->second);
                }
                tit.unlock_shared();
            } else if (op == 1) {
                auto rp = ref.insert({key, key});
                auto tp = trie.insert({key, key});
                ASSERT_TRUE(rp.second == tp.second);
                ASSERT_STR_EQ(rp.first->second, tp.first->second);
                tp.first.unlock();
            } else {
                const int scan_start = key_dist(rng);
                const int scan_count = scan_len_dist(rng);
                const std::string start_key = key_from_int(scan_start);

                std::vector<std::pair<std::string, std::string>> rv;
                std::vector<std::pair<std::string, std::string>> tv;
                for (auto it = ref.lower_bound(start_key);
                     it != ref.end() &&
                     static_cast<int>(rv.size()) < scan_count;
                     ++it) {
                    rv.emplace_back(it->first, it->second);
                }
                auto tit = trie.lower_bound(start_key);
                for (; tit != trie.shared_end() &&
                       static_cast<int>(tv.size()) < scan_count;
                     ++tit) {
                    tv.emplace_back(tit->first, tit->second);
                }
                tit.unlock_shared();
                ASSERT_EQ(static_cast<int>(rv.size()),
                          static_cast<int>(tv.size()));
                for (size_t i = 0; i < rv.size(); ++i) {
                    ASSERT_STR_EQ(rv[i].first, tv[i].first);
                    ASSERT_STR_EQ(rv[i].second, tv[i].second);
                }
            }
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("iteration ") +
                                     std::to_string(ITER) + ": " + e.what());
        }
    }
    monitor_thread.join();
    END_TEST("test_random")
}

} // namespace

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  test_random: ConcurrentLLTrie<flat_map> vs std::map"
              << std::endl;
    std::cout << "========================================" << std::endl;

    std::vector<std::pair<std::string, TestFunction>> tests = {
        {"test_random", test_random},
    };
    run_test_suite("test_random", tests);

    std::cout << "========================================" << std::endl;
    std::cout << "  Overall Test Results" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;
    std::cout << "Total tests:  " << (tests_passed + tests_failed) << std::endl;
    std::cout << std::endl;

    if (tests_failed == 0) {
        std::cout << "✓ All tests passed!" << std::endl;
        return 0;
    }
    std::cout << "✗ Some tests failed!" << std::endl;
    return 1;
}
