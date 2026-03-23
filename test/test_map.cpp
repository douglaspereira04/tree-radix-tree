#include "../utils/test_assertions.h"
#include "ll_trie/ll_trie.hpp"
#include <iostream>
#include <string>
#include <vector>

// Test result tracking
int tests_passed = 0;
int tests_failed = 0;

template <typename Map> void test_basic_put_get() {
    TEST("basic_put_get")
    Map t;

    t.insert({"key1", "val1"});
    t.insert({"key2", "val2"});
    t.insert({"key3", "val3"});

    auto it = t.find("key1");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("val1", it->second);
    it = t.find("key2");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("val2", it->second);
    it = t.find("key3");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("val3", it->second);
    END_TEST("basic_put_get")
}

template <typename Map> void test_get_nonexistent_key() {
    TEST("get_nonexistent_key")
    Map t;

    auto it = t.find("nonexistent");
    ASSERT_TRUE(it == t.end());
    END_TEST("get_nonexistent_key")
}

template <typename Map> void test_overwrite_value() {
    TEST("overwrite_value")
    Map t;

    t.insert({"key", "val1"});
    auto it = t.find("key");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("val1", it->second);

    t.insert_or_assign("key", "val2");
    it = t.find("key");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("val2", it->second);
    END_TEST("overwrite_value")
}

template <typename Map> void test_scan_basic() {
    TEST("scan_basic")
    Map t;

    t.insert({"user:1001", "v100"});
    t.insert({"user:1002", "v200"});
    t.insert({"user:1003", "v300"});
    t.insert({"product:2001", "v400"});

    std::vector<std::pair<std::string, std::string>> results;
    for (auto it = t.lower_bound("user:"); it != t.end(); ++it) {
        std::cout << it->first << " " << it->second << std::endl;
        if (it->first.compare(0, 5, "user:") != 0)
            break;
        std::cout << "Adding to results: " << it->first << " " << it->second
                  << std::endl;
        results.push_back({it->first, it->second});
    }
    ASSERT_EQ(3, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1001", results[0].first);
    ASSERT_STR_EQ("v100", results[0].second);
    ASSERT_STR_EQ("user:1002", results[1].first);
    ASSERT_STR_EQ("v200", results[1].second);
    ASSERT_STR_EQ("user:1003", results[2].first);
    ASSERT_STR_EQ("v300", results[2].second);
    END_TEST("scan_basic")
}

template <typename Map> void test_scan_no_matches() {
    TEST("scan_no_matches")
    Map t;

    t.insert({"apple", "v10"});
    t.insert({"banana", "v20"});

    int count = 0;
    for (auto it = t.lower_bound("orange"); it != t.end(); ++it) {
        if (it->first.compare(0, 6, "orange") != 0)
            break;
        ++count;
    }
    ASSERT_EQ(0, count);
    END_TEST("scan_no_matches")
}

template <typename Map> void test_scan_empty_prefix() {
    TEST("scan_empty_prefix")
    Map t;

    t.insert({"a", "v1"});
    t.insert({"b", "v2"});
    t.insert({"c", "v3"});

    int count = 0;
    for (auto it = t.begin(); it != t.end(); ++it)
        ++count;
    ASSERT_EQ(3, count);
    END_TEST("scan_empty_prefix")
}

template <typename Map> void test_scan_sorted_order() {
    TEST("scan_sorted_order")
    Map t;

    t.insert({"z", "zval"});
    t.insert({"a", "aval"});
    t.insert({"m", "mval"});

    std::vector<std::pair<std::string, std::string>> results;
    for (auto it = t.begin(); it != t.end(); ++it)
        results.push_back({it->first, it->second});
    ASSERT_EQ(3, static_cast<int>(results.size()));
    ASSERT_STR_EQ("a", results[0].first);
    ASSERT_STR_EQ("aval", results[0].second);
    ASSERT_STR_EQ("m", results[1].first);
    ASSERT_STR_EQ("mval", results[1].second);
    ASSERT_STR_EQ("z", results[2].first);
    ASSERT_STR_EQ("zval", results[2].second);
    END_TEST("scan_sorted_order")
}

template <typename Map> void test_scan_partial_prefix() {
    TEST("scan_partial_prefix")
    Map t;

    t.insert({"user:1001", "v100"});
    t.insert({"user:1002", "v200"});
    t.insert({"user:1003", "v300"});

    std::vector<std::pair<std::string, std::string>> results;
    for (auto it = t.lower_bound("user:1002"); it != t.end(); ++it) {
        if (it->first >= "user:1004")
            break;
        if (it->first >= "user:1002")
            results.push_back({it->first, it->second});
    }
    ASSERT_EQ(2, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1002", results[0].first);
    ASSERT_STR_EQ("v200", results[0].second);
    ASSERT_STR_EQ("user:1003", results[1].first);
    ASSERT_STR_EQ("v300", results[1].second);
    END_TEST("scan_partial_prefix")
}

template <typename Map> void test_scan_range() {
    TEST("scan_range")
    Map t;

    t.insert({"user:1001", "v100"});
    t.insert({"user:1002", "v200"});
    t.insert({"user:1003", "v300"});

    std::vector<std::pair<std::string, std::string>> results;
    for (auto it = t.lower_bound("user:1002");
         it != t.end() && it->first < "user:1004"; ++it)
        results.push_back({it->first, it->second});
    ASSERT_EQ(2, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1002", results[0].first);
    ASSERT_STR_EQ("user:1003", results[1].first);
    END_TEST("scan_range")
}

template <typename Map> void test_large_dataset() {
    TEST("large_dataset")
    Map t;

    for (int i = 0; i < 1000; ++i) {
        std::string key = "key:" + std::to_string(i);
        t.insert({key, std::to_string(i)});
    }

    auto it = t.find("key:0");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("0", it->second);
    it = t.find("key:500");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("500", it->second);
    it = t.find("key:999");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("999", it->second);

    int count = 0;
    for (auto i = t.lower_bound("key:"); i != t.end(); ++i) {
        if (i->first.compare(0, 4, "key:") != 0)
            break;
        ++count;
    }
    ASSERT_EQ(1000, count);
    END_TEST("large_dataset")
}

template <typename Map> void test_special_characters() {
    TEST("special_characters")
    Map t;

    t.insert({"key:with:colons", "v1"});
    t.insert({"key/with/slashes", "v2"});
    t.insert({"key-with-dashes", "v3"});
    t.insert({"key_with_underscores", "v4"});
    t.insert({"key.with.dots", "v5"});

    auto it = t.find("key:with:colons");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("v1", it->second);
    it = t.find("key/with/slashes");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("v2", it->second);
    it = t.find("key-with-dashes");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("v3", it->second);
    it = t.find("key_with_underscores");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("v4", it->second);
    it = t.find("key.with.dots");
    ASSERT_TRUE(it != t.end());
    ASSERT_STR_EQ("v5", it->second);
    END_TEST("special_characters")
}

template <typename Map> void test_tree_string() {
    TEST("tree_string")
    Map t;
    t.insert({"a", "va"});
    t.insert({"bc", "vbc"});
    std::string s = t.tree_string();
    ASSERT_TRUE(s.find("\"a\"") != std::string::npos);
    ASSERT_TRUE(s.find("\"bc\"") != std::string::npos);
    END_TEST("tree_string")
}

template <typename Map> void run_map_test_suite(const std::string &map_name) {
    std::vector<std::pair<std::string, TestFunction>> tests = {
        {"basic_put_get", []() { test_basic_put_get<Map>(); }},
        {"get_nonexistent_key", []() { test_get_nonexistent_key<Map>(); }},
        {"overwrite_value", []() { test_overwrite_value<Map>(); }},
        {"scan_basic", []() { test_scan_basic<Map>(); }},
        {"scan_no_matches", []() { test_scan_no_matches<Map>(); }},
        {"scan_empty_prefix", []() { test_scan_empty_prefix<Map>(); }},
        {"scan_sorted_order", []() { test_scan_sorted_order<Map>(); }},
        {"scan_partial_prefix", []() { test_scan_partial_prefix<Map>(); }},
        {"scan_range", []() { test_scan_range<Map>(); }},
        {"large_dataset", []() { test_large_dataset<Map>(); }},
        {"special_characters", []() { test_special_characters<Map>(); }},
        {"tree_string", []() { test_tree_string<Map>(); }},
    };

    run_test_suite(map_name, tests);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  LLTrie Test Suite" << std::endl;
    std::cout << "  (insert, find, lower_bound, begin, end)" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n=== Testing LLTrie<std::string> ===" << std::endl;
    run_map_test_suite<ll_trie::LLTrie<std::string>>("LLTrie<std::string>");

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
    } else {
        std::cout << "✗ Some tests failed!" << std::endl;
        return 1;
    }
}
