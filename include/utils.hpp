#pragma once

#include <string_view>

namespace {

inline size_t common_prefix_length_scalar(const std::string_view key1,
                                          const std::string_view key2) {
    size_t min_length = std::min(key1.size(), key2.size());
    for (size_t i = 0; i < min_length; i++) {
        if (key1[i] != key2[i]) {
            return i;
        }
    }
    return min_length;
}

} // namespace

inline size_t common_prefix_length(const std::string_view key1,
                                   const std::string_view key2) {
    return common_prefix_length_scalar(key1, key2);
}