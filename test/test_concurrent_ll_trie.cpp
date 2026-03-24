#include "../utils/test_assertions.h"
#include "concurrent_ll_trie/concurrent_ll_trie.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Test result tracking
int tests_passed = 0;
int tests_failed = 0;

using Trie = concurrent_ll_trie::ConcurrentLLTrie<std::string>;

void test_basic_put_get() {
    TEST("basic_put_get")
    Trie t;

    auto w_it = t.insert({"key1", "val1"});
    w_it.first.unlock();
    w_it = t.insert({"key2", "val2"});
    w_it.first.unlock();
    w_it = t.insert({"key3", "val3"});
    w_it.first.unlock();

    auto it = t.find("key1");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("val1", it->second);
    it.unlock_shared();
    it = t.find("key2");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("val2", it->second);
    it.unlock_shared();
    it = t.find("key3");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("val3", it->second);
    it.unlock_shared();
    END_TEST("basic_put_get")
}

void test_get_nonexistent_key() {
    TEST("get_nonexistent_key")
    Trie t;

    auto it = t.find("nonexistent");
    ASSERT_TRUE(it == t.shared_end());
    it.unlock_shared();
    END_TEST("get_nonexistent_key")
}

void test_overwrite_value() {
    TEST("overwrite_value")
    Trie t;

    auto w_it = t.insert({"key", "val1"});
    w_it.first.unlock();
    auto it = t.find("key");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("val1", it->second);
    it.unlock_shared();

    w_it = t.insert_or_assign("key", "val2");
    w_it.first.unlock();
    it = t.find("key");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("val2", it->second);
    it.unlock_shared();
    END_TEST("overwrite_value")
}

void test_scan_basic() {
    TEST("scan_basic")
    Trie t;

    auto w_it = t.insert({"user:1001", "v100"});
    t.debug_print_locked_nodes();
    w_it.first.unlock();

    w_it = t.insert({"user:1002", "v200"});
    t.debug_print_locked_nodes();
    w_it.first.unlock();

    w_it = t.insert({"user:1003", "v300"});
    t.debug_print_locked_nodes();
    w_it.first.unlock();

    w_it = t.insert({"product:2001", "v400"});
    t.debug_print_locked_nodes();
    w_it.first.unlock();

    std::vector<std::pair<std::string, std::string>> results;
    auto it = t.lower_bound("user:");
    while (it != t.shared_end()) {
        std::cout << it->first << " " << it->second << std::endl;
        if (it->first.compare(0, 5, "user:") != 0)
            break;
        std::cout << "Adding to results: " << it->first << " " << it->second
                  << std::endl;
        results.push_back({it->first, it->second});
        t.debug_print_locked_nodes();
        ++it;
    }
    it.unlock_shared();
    t.debug_print_locked_nodes();
    ASSERT_EQ(3, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1001", results[0].first);
    ASSERT_STR_EQ("v100", results[0].second);
    ASSERT_STR_EQ("user:1002", results[1].first);
    ASSERT_STR_EQ("v200", results[1].second);
    ASSERT_STR_EQ("user:1003", results[2].first);
    ASSERT_STR_EQ("v300", results[2].second);
    END_TEST("scan_basic")
}

void test_scan_no_matches() {
    TEST("scan_no_matches")
    Trie t;

    auto w_it = t.insert({"apple", "v10"});
    w_it.first.unlock();
    w_it = t.insert({"banana", "v20"});
    w_it.first.unlock();

    int count = 0;
    auto it = t.lower_bound("orange");
    for (; it != t.shared_end(); ++it) {
        if (it->first.compare(0, 6, "orange") != 0)
            break;
        ++count;
    }
    it.unlock_shared();
    ASSERT_EQ(0, count);
    END_TEST("scan_no_matches")
}

void test_scan_empty_prefix() {
    TEST("scan_empty_prefix")
    Trie t;

    auto w_it = t.insert({"a", "v1"});
    w_it.first.unlock();

    w_it = t.insert({"b", "v2"});
    w_it.first.unlock();

    w_it = t.insert({"c", "v3"});
    w_it.first.unlock();

    int count = 0;
    auto it = t.shared_begin();
    for (; it != t.shared_end(); ++it) {
        ++count;
        std::cout << " " << std::endl;
    }
    it.unlock_shared();
    ASSERT_EQ(3, count);
    END_TEST("scan_empty_prefix")
}

void test_scan_sorted_order() {
    TEST("scan_sorted_order")
    Trie t;

    auto w_it = t.insert({"z", "zval"});
    w_it.first.unlock();

    w_it = t.insert({"a", "aval"});
    w_it.first.unlock();

    w_it = t.insert({"m", "mval"});
    w_it.first.unlock();

    std::vector<std::pair<std::string, std::string>> results;
    for (auto it = t.shared_begin(); it != t.shared_end(); ++it)
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

void test_scan_partial_prefix() {
    TEST("scan_partial_prefix")
    Trie t;

    auto w_it = t.insert({"user:1001", "v100"});
    w_it.first.unlock();

    w_it = t.insert({"user:1002", "v200"});
    w_it.first.unlock();

    w_it = t.insert({"user:1003", "v300"});
    w_it.first.unlock();

    std::vector<std::pair<std::string, std::string>> results;
    auto it = t.lower_bound("user:1002");
    for (; it != t.shared_end(); ++it) {
        if (it->first >= "user:1004")
            break;
        if (it->first >= "user:1002")
            results.push_back({it->first, it->second});
    }
    it.unlock_shared();
    ASSERT_EQ(2, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1002", results[0].first);
    ASSERT_STR_EQ("v200", results[0].second);
    ASSERT_STR_EQ("user:1003", results[1].first);
    ASSERT_STR_EQ("v300", results[1].second);
    END_TEST("scan_partial_prefix")
}

void test_scan_range() {
    TEST("scan_range")
    Trie t;

    auto w_it = t.insert({"user:1001", "v100"});
    w_it.first.unlock();
    w_it = t.insert({"user:1002", "v200"});
    w_it.first.unlock();
    w_it = t.insert({"user:1003", "v300"});
    w_it.first.unlock();

    std::vector<std::pair<std::string, std::string>> results;
    auto it = t.lower_bound("user:1002");
    for (; it != t.shared_end() && it->first < "user:1004"; ++it)
        results.push_back({it->first, it->second});
    it.unlock_shared();
    ASSERT_EQ(2, static_cast<int>(results.size()));
    ASSERT_STR_EQ("user:1002", results[0].first);
    ASSERT_STR_EQ("user:1003", results[1].first);
    END_TEST("scan_range")
}

void test_large_dataset() {
    TEST("large_dataset")
    Trie t;

    for (int i = 0; i < 1000; ++i) {
        std::string key = "key:" + std::to_string(i);
        auto w_it = t.insert({key, std::to_string(i)});
        w_it.first.unlock();
    }

    auto it = t.find("key:0");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("0", it->second);
    it.unlock_shared();
    it = t.find("key:500");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("500", it->second);
    it.unlock_shared();
    it = t.find("key:999");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("999", it->second);
    it.unlock_shared();

    int count = 0;
    it = t.lower_bound("key:");
    for (; it != t.shared_end(); ++it) {
        if (it->first.compare(0, 4, "key:") != 0)
            break;
        ++count;
    }
    it.unlock_shared();
    ASSERT_EQ(1000, count);
    END_TEST("large_dataset")
}

void test_special_characters() {
    TEST("special_characters")
    Trie t;

    auto w_it = t.insert({"key:with:colons", "v1"});
    w_it.first.unlock();
    w_it = t.insert({"key/with/slashes", "v2"});
    w_it.first.unlock();
    w_it = t.insert({"key-with-dashes", "v3"});
    w_it.first.unlock();
    w_it = t.insert({"key_with_underscores", "v4"});
    w_it.first.unlock();
    w_it = t.insert({"key.with.dots", "v5"});
    w_it.first.unlock();

    auto it = t.find("key:with:colons");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("v1", it->second);
    it.unlock_shared();

    it = t.find("key/with/slashes");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("v2", it->second);
    it.unlock_shared();

    it = t.find("key-with-dashes");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("v3", it->second);
    it.unlock_shared();

    it = t.find("key_with_underscores");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("v4", it->second);
    it.unlock_shared();

    it = t.find("key.with.dots");
    ASSERT_TRUE(it != t.shared_end());
    ASSERT_STR_EQ("v5", it->second);
    it.unlock_shared();

    END_TEST("special_characters")
}

void test_tree_string() {
    TEST("tree_string")
    Trie t;
    auto w_it = t.insert({"a", "va"});
    w_it.first.unlock();
    w_it = t.insert({"bc", "vbc"});
    w_it.first.unlock();
    std::string s = t.tree_string();
    ASSERT_TRUE(s.find("\"a\"") != std::string::npos);
    ASSERT_TRUE(s.find("\"bc\"") != std::string::npos);
    END_TEST("tree_string")
}

/** lower_bound("0833") must be end(): keys "0597" and "0797" are the only
 * entries; they share prefix "0", but the next query digit '8' is greater than
 * every third digit among children ('5','7'). std::map has no key >= "0833";
 * LLTrie must match. (A buggy trie may wrongly return the first value under
 * "0".) */
void test_lower_bound_no_key_gte_query_after_prefix() {
    TEST("lower_bound_no_key_gte_query_after_prefix")
    Trie t;
    auto w_it = t.insert({"0597", "0597"});
    w_it.first.unlock();

    w_it = t.insert({"0797", "0797"});
    w_it.first.unlock();

    auto it = t.lower_bound("0833");
    ASSERT_TRUE(it == t.shared_end());
    it.unlock_shared();
    END_TEST("lower_bound_no_key_gte_query_after_prefix")
}

/** Insert order affects sibling "next" links. This sequence (from test_random
 * seed 42 up to iter 29) left a bug where forward iteration from
 * lower_bound("0304") skipped "0597" (jumped 0467 -> 0680). std::map order:
 * 0400, 0467, 0597, 0680, 0797, 0984. */
void test_forward_iteration_matches_map_after_insert_order() {
    TEST("forward_iteration_matches_map_after_insert_order")
    static const char *const kInsertOrder[] = {"0797", "0597", "0007", "0023",
                                               "0400", "0233", "0090", "0984",
                                               "0467", "0680"};
    Trie t;
    std::map<std::string, std::string> ref;
    for (auto k : kInsertOrder) {
        auto w_it = t.insert({k, k});
        w_it.first.unlock();
        ref.insert({k, k});
    }

    const std::string start = "0304";
    std::vector<std::pair<std::string, std::string>> from_ref;
    std::vector<std::pair<std::string, std::string>> from_trie;
    for (auto it = ref.lower_bound(start); it != ref.end(); ++it)
        from_ref.emplace_back(it->first, it->second);
    auto it = t.lower_bound(start);
    std::cout << "Before loop" << std::endl;
    t.debug_print_locked_nodes();
    std::cout << "Beginning loop" << std::endl;
    for (; it != t.shared_end(); ++it) {
        std::cout << " " << std::endl;
        t.debug_print_locked_nodes();
        from_trie.emplace_back(it->first, it->second);
    }
    std::cout << "After loop" << std::endl;
    t.debug_print_locked_nodes();
    it.unlock_shared();
    ASSERT_EQ(static_cast<int>(from_ref.size()),
              static_cast<int>(from_trie.size()));
    for (size_t i = 0; i < from_ref.size(); ++i) {
        ASSERT_STR_EQ(from_ref[i].first, from_trie[i].first);
        ASSERT_STR_EQ(from_ref[i].second, from_trie[i].second);
    }
    END_TEST("forward_iteration_matches_map_after_insert_order")
}

/** Minimal: 3 keys, 2 characters each (cannot do with 2 keys: need 3+ values to
 * observe a skip). Insert order "46", "68", "59" — lex order is 46, 59, 68, but
 * inserting the "5" branch after "6" mis-links "next", so iteration from
 * lower_bound("30") skips "46" (map: 46, 59, 68). Same class of bug as the
 * longer 10-key case. */
void test_minimal_next_chain_iterate_matches_map() {
    TEST("minimal_next_chain_iterate_matches_map")
    static const char *const kOrder[] = {"46", "68", "59"};
    Trie t;
    std::map<std::string, std::string> ref;
    for (auto k : kOrder) {
        auto w_it = t.insert({k, k});
        w_it.first.unlock();
        ref.insert({k, k});
    }

    const std::string start = "30";
    std::vector<std::pair<std::string, std::string>> from_ref;
    std::vector<std::pair<std::string, std::string>> from_trie;
    for (auto it = ref.lower_bound(start); it != ref.end(); ++it)
        from_ref.emplace_back(it->first, it->second);
    auto it = t.lower_bound(start);
    for (; it != t.shared_end(); ++it)
        from_trie.emplace_back(it->first, it->second);
    it.unlock_shared();

    ASSERT_EQ(static_cast<int>(from_ref.size()),
              static_cast<int>(from_trie.size()));
    for (size_t i = 0; i < from_ref.size(); ++i) {
        ASSERT_STR_EQ(from_ref[i].first, from_trie[i].first);
        ASSERT_STR_EQ(from_ref[i].second, from_trie[i].second);
    }
    END_TEST("minimal_next_chain_iterate_matches_map")
}

void run_concurrent_ll_trie_test_suite(const std::string &suite_name) {
    std::vector<std::pair<std::string, TestFunction>> tests = {
        {"basic_put_get", test_basic_put_get},
        {"get_nonexistent_key", test_get_nonexistent_key},
        {"overwrite_value", test_overwrite_value},
        {"scan_basic", test_scan_basic},
        {"scan_no_matches", test_scan_no_matches},
        {"scan_empty_prefix", test_scan_empty_prefix},
        {"scan_sorted_order", test_scan_sorted_order},
        {"scan_partial_prefix", test_scan_partial_prefix},
        {"scan_range", test_scan_range},
        {"large_dataset", test_large_dataset},
        {"special_characters", test_special_characters},
        {"tree_string", test_tree_string},
        {"lower_bound_no_key_gte_query_after_prefix",
         test_lower_bound_no_key_gte_query_after_prefix},
        {"forward_iteration_matches_map_after_insert_order",
         test_forward_iteration_matches_map_after_insert_order},
        {"minimal_next_chain_iterate_matches_map",
         test_minimal_next_chain_iterate_matches_map},
    };

    run_test_suite(suite_name, tests);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  ConcurrentLLTrie Test Suite" << std::endl;
    std::cout << "  (insert, find, lower_bound, begin, end)" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n=== Testing ConcurrentLLTrie<std::string> ===" << std::endl;
    run_concurrent_ll_trie_test_suite("ConcurrentLLTrie<std::string>");

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
