#pragma once

#include <string>
#include <string_view>

#if defined(DEBUG_LOG)
#include <iostream>
/** Prints msg to stdout. When DEBUG_LOG is off, expands to nothing and does not
 * evaluate msg. */
#define UTILS_LOG(msg)                                                         \
    do {                                                                       \
        std::cout << (msg) << '\n';                                            \
    } while (0)
#else
#define UTILS_LOG(msg) ((void)0)
#endif

namespace {

inline size_t common_prefix_length_scalar(const std::string &key1,
                                          const std::string &key2) {
    size_t min_length = std::min(key1.size(), key2.size());
    for (size_t i = 0; i < min_length; i++) {
        if (key1[i] != key2[i]) {
            return i;
        }
    }
    return min_length;
}

inline size_t common_prefix_length_scalar(const std::string &key1,
                                          const std::string &key2,
                                          size_t key_2_begin) {
    size_t min_length = std::min(key1.size(), key2.size() - key_2_begin);
    for (size_t i = 0; i < min_length; i++) {
        if (key1[i] != key2[i + key_2_begin]) {
            return i;
        }
    }
    return min_length;
}

} // namespace

inline size_t common_prefix_length(const std::string &key1,
                                   const std::string &key2) {
    return common_prefix_length_scalar(key1, key2);
}

inline size_t common_prefix_length(const std::string &key1,
                                   const std::string &key2,
                                   size_t key_2_begin) {
    return common_prefix_length_scalar(key1, key2, key_2_begin);
}