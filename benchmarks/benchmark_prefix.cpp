#include "utils.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

int main(int argc, char **argv) {
    const size_t num_pairs = 10'000'000;
    const size_t max_len = 8;
    const size_t min_len = 6;
    std::mt19937 rng(777);
    std::uniform_int_distribution<int> char_dist('a', 'z');

    // Generate string pairs with varying common prefix lengths
    std::vector<std::string> a(num_pairs), b(num_pairs);
    for (size_t i = 0; i < num_pairs; i++) {
        size_t len = min_len + (rng() % (max_len - min_len));
        a[i].resize(len);
        b[i].resize(len);
        size_t common = rng() % (len + 1); // 0 to len
        for (size_t j = 0; j < common; j++) {
            char c = static_cast<char>(char_dist(rng));
            a[i][j] = b[i][j] = c;
        }
        for (size_t j = common; j < len; j++) {
            a[i][j] = static_cast<char>(char_dist(rng));
            b[i][j] = static_cast<char>(char_dist(rng));
        }
    }

    volatile size_t sink = 0;
    const int iterations = 5;

    auto run = [&]() {
        auto start = std::chrono::high_resolution_clock::now();
        for (int iter = 0; iter < iterations; iter++) {
            for (size_t i = 0; i < num_pairs; i++) {
                sink += common_prefix_length(a[i], b[i]);
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    };

    double ms = run();
    size_t total_comparisons = num_pairs * iterations;
    double ns_per_call = (ms * 1e6) / total_comparisons;

    std::cout << ms << " ms for " << total_comparisons << " calls ("
              << ns_per_call << " ns/call)\n";

    (void)sink;
    (void)argc;
    (void)argv;
    return 0;
}
