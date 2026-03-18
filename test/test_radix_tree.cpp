#include "../utils/test_assertions.h"
#include "radix_tree/radix_tree.hpp"
#include <iostream>
#include <string>
#include <vector>

// Test result tracking
int tests_passed = 0;
int tests_failed = 0;

// Generic test functions that work with RadixTree<Value>
template <typename Value> void test_basic_put_get() {
    TEST("basic_put_get")
    radix_tree::RadixTree<Value> t;

    t.put("key1", static_cast<Value>(100));
    t.put("key2", static_cast<Value>(200));
    t.put("key3", static_cast<Value>(300));

    Value v1;
    ASSERT_TRUE(t.get("key1", v1));
    ASSERT_EQ(static_cast<Value>(100), v1);

    Value v2;
    ASSERT_TRUE(t.get("key2", v2));
    ASSERT_EQ(static_cast<Value>(200), v2);

    Value v3;
    ASSERT_TRUE(t.get("key3", v3));
    ASSERT_EQ(static_cast<Value>(300), v3);
    END_TEST("basic_put_get")
}

template <typename Value> void test_get_nonexistent_key() {
    TEST("get_nonexistent_key")
    radix_tree::RadixTree<Value> t;

    Value v{};
    ASSERT_FALSE(t.get("nonexistent", v));
    END_TEST("get_nonexistent_key")
}

template <typename Value> void test_overwrite_value() {
    TEST("overwrite_value")
    radix_tree::RadixTree<Value> t;

    t.put("key", static_cast<Value>(100));
    Value v;
    ASSERT_TRUE(t.get("key", v));
    ASSERT_EQ(static_cast<Value>(100), v);

    t.put("key", static_cast<Value>(200));
    ASSERT_TRUE(t.get("key", v));
    ASSERT_EQ(static_cast<Value>(200), v);
    END_TEST("overwrite_value")
}

template <typename Value> void test_scan_basic() {
    TEST("scan_basic")
    radix_tree::RadixTree<Value> t;

    t.put("user:1001", static_cast<Value>(100));
    t.put("user:1002", static_cast<Value>(200));
    t.put("user:1003", static_cast<Value>(300));
    t.put("product:2001", static_cast<Value>(400));

    std::vector<std::pair<std::string, Value>> results;
    ASSERT_TRUE(t.scan("user:", results));
    ASSERT_EQ(3, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1001", results[0].first);
    ASSERT_EQ(static_cast<Value>(100), results[0].second);
    ASSERT_STR_EQ("user:1002", results[1].first);
    ASSERT_EQ(static_cast<Value>(200), results[1].second);
    ASSERT_STR_EQ("user:1003", results[2].first);
    ASSERT_EQ(static_cast<Value>(300), results[2].second);
    END_TEST("scan_basic")
}

template <typename Value> void test_scan_no_matches() {
    TEST("scan_no_matches")
    radix_tree::RadixTree<Value> t;

    t.put("apple", static_cast<Value>(10));
    t.put("banana", static_cast<Value>(20));

    std::vector<std::pair<std::string, Value>> results;
    ASSERT_TRUE(t.scan("orange", results));
    ASSERT_EQ(0, static_cast<int>(results.size()));
    END_TEST("scan_no_matches")
}

template <> void test_scan_no_matches<std::string>() {
    TEST("scan_no_matches")
    radix_tree::RadixTree<std::string> t;

    t.put("apple", "v10");
    t.put("banana", "v20");

    std::vector<std::pair<std::string, std::string>> results;
    ASSERT_TRUE(t.scan("orange", results));
    ASSERT_EQ(0, static_cast<int>(results.size()));
    END_TEST("scan_no_matches")
}

template <typename Value> void test_scan_empty_prefix() {
    TEST("scan_empty_prefix")
    radix_tree::RadixTree<Value> t;

    t.put("a", static_cast<Value>(1));
    t.put("b", static_cast<Value>(2));
    t.put("c", static_cast<Value>(3));

    std::vector<std::pair<std::string, Value>> results;
    ASSERT_TRUE(t.scan("", results));
    ASSERT_EQ(3, static_cast<int>(results.size()));
    END_TEST("scan_empty_prefix")
}

template <> void test_scan_empty_prefix<std::string>() {
    TEST("scan_empty_prefix")
    radix_tree::RadixTree<std::string> t;

    t.put("a", "v1");
    t.put("b", "v2");
    t.put("c", "v3");

    std::vector<std::pair<std::string, std::string>> results;
    ASSERT_TRUE(t.scan("", results));
    ASSERT_EQ(3, static_cast<int>(results.size()));
    END_TEST("scan_empty_prefix")
}

template <typename Value> void test_scan_sorted_order() {
    TEST("scan_sorted_order")
    radix_tree::RadixTree<Value> t;

    t.put("z", static_cast<Value>(26));
    t.put("a", static_cast<Value>(1));
    t.put("m", static_cast<Value>(13));

    std::vector<std::pair<std::string, Value>> results;
    ASSERT_TRUE(t.scan("", results));
    ASSERT_EQ(3, static_cast<int>(results.size()));
    ASSERT_STR_EQ("a", results[0].first);
    ASSERT_EQ(static_cast<Value>(1), results[0].second);
    ASSERT_STR_EQ("m", results[1].first);
    ASSERT_EQ(static_cast<Value>(13), results[1].second);
    ASSERT_STR_EQ("z", results[2].first);
    ASSERT_EQ(static_cast<Value>(26), results[2].second);
    END_TEST("scan_sorted_order")
}

template <typename Value> void test_large_dataset() {
    TEST("large_dataset")
    radix_tree::RadixTree<Value> t;

    for (int i = 0; i < 1000; ++i) {
        std::string key = "key:" + std::to_string(i);
        t.put(key, static_cast<Value>(i));
    }

    Value v0;
    ASSERT_TRUE(t.get("key:0", v0));
    ASSERT_EQ(static_cast<Value>(0), v0);

    Value v500;
    ASSERT_TRUE(t.get("key:500", v500));
    ASSERT_EQ(static_cast<Value>(500), v500);

    Value v999;
    ASSERT_TRUE(t.get("key:999", v999));
    ASSERT_EQ(static_cast<Value>(999), v999);

    std::vector<std::pair<std::string, Value>> results;
    ASSERT_TRUE(t.scan("key:", results));
    ASSERT_EQ(1000, static_cast<int>(results.size()));
    END_TEST("large_dataset")
}

template <> void test_large_dataset<std::string>() {
    TEST("large_dataset")
    radix_tree::RadixTree<std::string> t;

    for (int i = 0; i < 1000; ++i) {
        std::string key = "key:" + std::to_string(i);
        t.put(key, std::to_string(i));
    }

    std::string v0;
    ASSERT_TRUE(t.get("key:0", v0));
    ASSERT_STR_EQ("0", v0);

    std::string v500;
    ASSERT_TRUE(t.get("key:500", v500));
    ASSERT_STR_EQ("500", v500);

    std::string v999;
    ASSERT_TRUE(t.get("key:999", v999));
    ASSERT_STR_EQ("999", v999);

    std::vector<std::pair<std::string, std::string>> results;
    ASSERT_TRUE(t.scan("key:", results));
    ASSERT_EQ(1000, static_cast<int>(results.size()));
    END_TEST("large_dataset")
}

template <typename Value> void test_special_characters() {
    TEST("special_characters")
    radix_tree::RadixTree<Value> t;

    t.put("key:with:colons", static_cast<Value>(1));
    t.put("key/with/slashes", static_cast<Value>(2));
    t.put("key-with-dashes", static_cast<Value>(3));
    t.put("key_with_underscores", static_cast<Value>(4));
    t.put("key.with.dots", static_cast<Value>(5));

    Value v;
    ASSERT_TRUE(t.get("key:with:colons", v));
    ASSERT_EQ(static_cast<Value>(1), v);
    ASSERT_TRUE(t.get("key/with/slashes", v));
    ASSERT_EQ(static_cast<Value>(2), v);
    ASSERT_TRUE(t.get("key-with-dashes", v));
    ASSERT_EQ(static_cast<Value>(3), v);
    ASSERT_TRUE(t.get("key_with_underscores", v));
    ASSERT_EQ(static_cast<Value>(4), v);
    ASSERT_TRUE(t.get("key.with.dots", v));
    ASSERT_EQ(static_cast<Value>(5), v);
    END_TEST("special_characters")
}

template <typename Value> void test_scan_partial_prefix() {
    TEST("scan_partial_prefix")
    radix_tree::RadixTree<Value> t;

    t.put("user:1001", static_cast<Value>(100));
    t.put("user:1002", static_cast<Value>(200));
    t.put("user:1003", static_cast<Value>(300));

    std::vector<std::pair<std::string, Value>> results;
    ASSERT_TRUE(t.scan("user:1002", results));
    ASSERT_EQ(2, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1002", results[0].first);
    ASSERT_EQ(static_cast<Value>(200), results[0].second);
    ASSERT_STR_EQ("user:1003", results[1].first);
    ASSERT_EQ(static_cast<Value>(300), results[1].second);
    END_TEST("scan_partial_prefix")
}

// Specialization for std::string (ASSERT_EQ uses std::to_string which doesn't
// work for string)
template <> void test_basic_put_get<std::string>() {
    TEST("basic_put_get")
    radix_tree::RadixTree<std::string> t;

    t.put("key1", "val1");
    t.put("key2", "val2");
    t.put("key3", "val3");

    std::string v1;
    ASSERT_TRUE(t.get("key1", v1));
    ASSERT_STR_EQ("val1", v1);

    std::string v2;
    ASSERT_TRUE(t.get("key2", v2));
    ASSERT_STR_EQ("val2", v2);

    std::string v3;
    ASSERT_TRUE(t.get("key3", v3));
    ASSERT_STR_EQ("val3", v3);
    END_TEST("basic_put_get")
}

template <> void test_overwrite_value<std::string>() {
    TEST("overwrite_value")
    radix_tree::RadixTree<std::string> t;

    t.put("key", "val1");
    std::string v;
    ASSERT_TRUE(t.get("key", v));
    ASSERT_STR_EQ("val1", v);

    t.put("key", "val2");
    ASSERT_TRUE(t.get("key", v));
    ASSERT_STR_EQ("val2", v);
    END_TEST("overwrite_value")
}

template <> void test_scan_basic<std::string>() {
    TEST("scan_basic")
    radix_tree::RadixTree<std::string> t;

    t.put("user:1001", "v100");
    t.put("user:1002", "v200");
    t.put("user:1003", "v300");
    t.put("product:2001", "v400");

    std::vector<std::pair<std::string, std::string>> results;
    ASSERT_TRUE(t.scan("user:", results));
    ASSERT_EQ(3, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1001", results[0].first);
    ASSERT_STR_EQ("v100", results[0].second);
    ASSERT_STR_EQ("user:1002", results[1].first);
    ASSERT_STR_EQ("v200", results[1].second);
    ASSERT_STR_EQ("user:1003", results[2].first);
    ASSERT_STR_EQ("v300", results[2].second);
    END_TEST("scan_basic")
}

template <> void test_scan_sorted_order<std::string>() {
    TEST("scan_sorted_order")
    radix_tree::RadixTree<std::string> t;

    t.put("z", "zval");
    t.put("a", "aval");
    t.put("m", "mval");

    std::vector<std::pair<std::string, std::string>> results;
    ASSERT_TRUE(t.scan("", results));
    ASSERT_EQ(3, static_cast<int>(results.size()));
    ASSERT_STR_EQ("a", results[0].first);
    ASSERT_STR_EQ("aval", results[0].second);
    ASSERT_STR_EQ("m", results[1].first);
    ASSERT_STR_EQ("mval", results[1].second);
    ASSERT_STR_EQ("z", results[2].first);
    ASSERT_STR_EQ("zval", results[2].second);
    END_TEST("scan_sorted_order")
}

template <> void test_special_characters<std::string>() {
    TEST("special_characters")
    radix_tree::RadixTree<std::string> t;

    t.put("key:with:colons", "v1");
    t.put("key/with/slashes", "v2");
    t.put("key-with-dashes", "v3");
    t.put("key_with_underscores", "v4");
    t.put("key.with.dots", "v5");

    std::string v;
    ASSERT_TRUE(t.get("key:with:colons", v));
    ASSERT_STR_EQ("v1", v);
    ASSERT_TRUE(t.get("key/with/slashes", v));
    ASSERT_STR_EQ("v2", v);
    ASSERT_TRUE(t.get("key-with-dashes", v));
    ASSERT_STR_EQ("v3", v);
    ASSERT_TRUE(t.get("key_with_underscores", v));
    ASSERT_STR_EQ("v4", v);
    ASSERT_TRUE(t.get("key.with.dots", v));
    ASSERT_STR_EQ("v5", v);
    END_TEST("special_characters")
}

template <> void test_scan_partial_prefix<std::string>() {
    TEST("scan_partial_prefix")
    radix_tree::RadixTree<std::string> t;

    t.put("user:1001", "v100");
    t.put("user:1002", "v200");
    t.put("user:1003", "v300");

    std::vector<std::pair<std::string, std::string>> results;
    ASSERT_TRUE(t.scan("user:1002", results));
    ASSERT_EQ(2, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1002", results[0].first);
    ASSERT_STR_EQ("v200", results[0].second);
    ASSERT_STR_EQ("user:1003", results[1].first);
    ASSERT_STR_EQ("v300", results[1].second);
    END_TEST("scan_partial_prefix")
}

// Helper to run all tests for a value type
template <typename Value>
void run_radix_tree_test_suite(const std::string &value_type_name) {
    std::vector<std::pair<std::string, TestFunction>> tests = {
        {"basic_put_get", []() { test_basic_put_get<Value>(); }},
        {"get_nonexistent_key", []() { test_get_nonexistent_key<Value>(); }},
        {"overwrite_value", []() { test_overwrite_value<Value>(); }},
        {"scan_basic", []() { test_scan_basic<Value>(); }},
        {"scan_no_matches", []() { test_scan_no_matches<Value>(); }},
        {"scan_empty_prefix", []() { test_scan_empty_prefix<Value>(); }},
        {"scan_sorted_order", []() { test_scan_sorted_order<Value>(); }},
        {"scan_partial_prefix", []() { test_scan_partial_prefix<Value>(); }},
        {"large_dataset", []() { test_large_dataset<Value>(); }},
        {"special_characters", []() { test_special_characters<Value>(); }},
    };

    std::string suite_name = "RadixTree<" + value_type_name + ">";
    run_test_suite(suite_name, tests);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Radix Tree Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n=== Testing with int ===" << std::endl;
    run_radix_tree_test_suite<int>("int");

    std::cout << "\n=== Testing with std::string ===" << std::endl;
    run_radix_tree_test_suite<std::string>("std::string");

    std::cout << "\n========================================" << std::endl;
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
