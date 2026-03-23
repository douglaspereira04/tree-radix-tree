#include "../utils/test_assertions.h"
#include "ll_trie/ll_trie.hpp"
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

int tests_passed = 0;
int tests_failed = 0;

namespace {

constexpr int MAX_KEY = 1000;

std::string key_from_int(int n) {
    const int width = static_cast<int>(std::to_string(MAX_KEY).size());
    std::string s = std::to_string(n);
    if (static_cast<int>(s.size()) >= width)
        return s;
    return std::string(static_cast<size_t>(width - s.size()), '0') + s;
}

void test_random() {
    TEST("test_random")
    std::map<std::string, std::string> ref;
    ll_trie::LLTrie<std::string> trie;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> op_dist(0, 2);
    std::uniform_int_distribution<int> key_dist(0, MAX_KEY);
    std::uniform_int_distribution<int> scan_len_dist(1, 1000);

    constexpr int iterations = 50000;

    for (int iter = 0; iter < iterations; ++iter) {
        try {
            const int op = op_dist(rng);
            const int k = key_dist(rng);
            const std::string key = key_from_int(k);

            if (op == 0) {
                auto rit = ref.find(key);
                auto tit = trie.find(key);
                const bool r_found = (rit != ref.end());
                const bool t_found = (tit != trie.end());
                ASSERT_TRUE(r_found == t_found);
                if (r_found) {
                    ASSERT_STR_EQ(rit->second, tit->second);
                }
            } else if (op == 1) {
                auto rp = ref.insert({key, key});
                auto tp = trie.insert({key, key});
                ASSERT_TRUE(rp.second == tp.second);
                ASSERT_STR_EQ(rp.first->second, tp.first->second);
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
                for (auto it = trie.lower_bound(start_key);
                     it != trie.end() &&
                     static_cast<int>(tv.size()) < scan_count;
                     ++it) {
                    tv.emplace_back(it->first, it->second);
                }
                ASSERT_EQ(static_cast<int>(rv.size()),
                          static_cast<int>(tv.size()));
                for (size_t i = 0; i < rv.size(); ++i) {
                    ASSERT_STR_EQ(rv[i].first, tv[i].first);
                    ASSERT_STR_EQ(rv[i].second, tv[i].second);
                }
            }
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("iteration ") +
                                     std::to_string(iter) + ": " + e.what());
        }
    }
    END_TEST("test_random")
}

} // namespace

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  test_random: LLTrie vs std::map" << std::endl;
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
