#include "../utils/test_assertions.h"
#include "ll_trie/ll_trie.hpp"
#include <iostream>
#include <map>
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

template <typename Map> void test_erase_nonexistent_noop() {
    TEST("erase_nonexistent_noop")
    Map t;
    ASSERT_EQ(0, static_cast<int>(t.erase("missing")));
    ASSERT_TRUE(t.find("missing") == t.end());

    t.insert({"only", "v"});
    ASSERT_EQ(0, static_cast<int>(t.erase("other")));
    ASSERT_TRUE(t.find("only") != t.end());
    ASSERT_EQ(1u, t.size());
    END_TEST("erase_nonexistent_noop")
}

template <typename Map> void test_erase_existing_key() {
    TEST("erase_existing_key")
    Map t;
    t.insert({"k", "v"});
    ASSERT_EQ(1, static_cast<int>(t.erase("k")));
    ASSERT_TRUE(t.find("k") == t.end());
    ASSERT_EQ(0u, t.size());
    ASSERT_EQ(0, static_cast<int>(t.erase("k")));
    END_TEST("erase_existing_key")
}

template <typename Map> void test_erase_preserves_other_keys() {
    TEST("erase_preserves_other_keys")
    Map t;
    t.insert({"a", "va"});
    t.insert({"b", "vb"});
    t.insert({"c", "vc"});
    ASSERT_EQ(1, static_cast<int>(t.erase("b")));
    ASSERT_TRUE(t.find("a") != t.end());
    ASSERT_TRUE(t.find("b") == t.end());
    ASSERT_TRUE(t.find("c") != t.end());
    ASSERT_STR_EQ("va", t.find("a")->second);
    ASSERT_STR_EQ("vc", t.find("c")->second);
    ASSERT_EQ(2u, t.size());
    END_TEST("erase_preserves_other_keys")
}

template <typename Map> void test_erase_iterator_advances_like_map() {
    TEST("erase_iterator_advances_like_map")
    Map t;
    std::map<std::string, std::string> ref;
    t.insert({"a", "a"});
    t.insert({"b", "b"});
    t.insert({"c", "c"});
    ref.insert({"a", "a"});
    ref.insert({"b", "b"});
    ref.insert({"c", "c"});

    auto it = t.find("b");
    auto ref_it = ref.find("b");
    ASSERT_TRUE(it != t.end());
    auto next_t = t.erase(it);
    auto next_r = ref.erase(ref_it);

    if (next_r == ref.end()) {
        ASSERT_TRUE(next_t == t.end());
    } else {
        ASSERT_TRUE(next_t != t.end());
        ASSERT_STR_EQ(next_r->first, next_t->first);
        ASSERT_STR_EQ(next_r->second, next_t->second);
    }
    END_TEST("erase_iterator_advances_like_map")
}

template <typename Map> void test_erase_iterator_only_element_returns_end() {
    TEST("erase_iterator_only_element_returns_end")
    Map t;
    t.insert({"solo", "v"});
    auto it = t.begin();
    ASSERT_TRUE(it != t.end());
    auto next = t.erase(it);
    ASSERT_TRUE(next == t.end());
    ASSERT_EQ(0u, t.size());
    END_TEST("erase_iterator_only_element_returns_end")
}

/** Same inserts/erases as std::map; forward iteration from begin must match. */
template <typename Map> void test_erase_sequence_matches_map() {
    TEST("erase_sequence_matches_map")
    Map t;
    std::map<std::string, std::string> ref;
    for (auto k : {"x", "y", "z", "m", "n"}) {
        t.insert({k, k});
        ref.insert({k, k});
    }
    ASSERT_EQ(1, static_cast<int>(t.erase("y")));
    ASSERT_EQ(1u, ref.erase("y"));
    ASSERT_EQ(1, static_cast<int>(t.erase("m")));
    ASSERT_EQ(1u, ref.erase("m"));

    std::vector<std::pair<std::string, std::string>> from_trie;
    std::vector<std::pair<std::string, std::string>> from_ref;
    for (auto it = t.begin(); it != t.end(); ++it)
        from_trie.emplace_back(it->first, it->second);
    for (auto it = ref.begin(); it != ref.end(); ++it)
        from_ref.emplace_back(it->first, it->second);

    ASSERT_EQ(static_cast<int>(from_ref.size()),
              static_cast<int>(from_trie.size()));
    for (size_t i = 0; i < from_ref.size(); ++i) {
        ASSERT_STR_EQ(from_ref[i].first, from_trie[i].first);
        ASSERT_STR_EQ(from_ref[i].second, from_trie[i].second);
    }
    ASSERT_EQ(3u, t.size());
    ASSERT_EQ(3u, ref.size());
    END_TEST("erase_sequence_matches_map")
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

/** lower_bound("0833") must be end(): keys "0597" and "0797" are the only
 * entries; they share prefix "0", but the next query digit '8' is greater than
 * every third digit among children ('5','7'). std::map has no key >= "0833";
 * LLTrie must match. (A buggy trie may wrongly return the first value under
 * "0".) */
template <typename Map> void test_lower_bound_no_key_gte_query_after_prefix() {
    TEST("lower_bound_no_key_gte_query_after_prefix")
    Map t;
    t.insert({"0597", "0597"});
    t.insert({"0797", "0797"});

    auto it = t.lower_bound("0833");
    ASSERT_TRUE(it == t.end());
    END_TEST("lower_bound_no_key_gte_query_after_prefix")
}

/** Insert order affects sibling "next" links. This sequence (from test_random
 * seed 42 up to iter 29) left a bug where forward iteration from
 * lower_bound("0304") skipped "0597" (jumped 0467 -> 0680). std::map order:
 * 0400, 0467, 0597, 0680, 0797, 0984. */
template <typename Map>
void test_forward_iteration_matches_map_after_insert_order() {
    TEST("forward_iteration_matches_map_after_insert_order")
    static const char *const kInsertOrder[] = {"0797", "0597", "0007", "0023",
                                               "0400", "0233", "0090", "0984",
                                               "0467", "0680"};
    Map t;
    std::map<std::string, std::string> ref;
    for (auto k : kInsertOrder) {
        t.insert({k, k});
        ref.insert({k, k});
    }

    const std::string start = "0304";
    std::vector<std::pair<std::string, std::string>> from_ref;
    std::vector<std::pair<std::string, std::string>> from_trie;
    for (auto it = ref.lower_bound(start); it != ref.end(); ++it)
        from_ref.emplace_back(it->first, it->second);
    for (auto it = t.lower_bound(start); it != t.end(); ++it)
        from_trie.emplace_back(it->first, it->second);

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
template <typename Map> void test_minimal_next_chain_iterate_matches_map() {
    TEST("minimal_next_chain_iterate_matches_map")
    static const char *const kOrder[] = {"46", "68", "59"};
    Map t;
    std::map<std::string, std::string> ref;
    for (auto k : kOrder) {
        t.insert({k, k});
        ref.insert({k, k});
    }

    const std::string start = "30";
    std::vector<std::pair<std::string, std::string>> from_ref;
    std::vector<std::pair<std::string, std::string>> from_trie;
    for (auto it = ref.lower_bound(start); it != ref.end(); ++it)
        from_ref.emplace_back(it->first, it->second);
    for (auto it = t.lower_bound(start); it != t.end(); ++it)
        from_trie.emplace_back(it->first, it->second);

    ASSERT_EQ(static_cast<int>(from_ref.size()),
              static_cast<int>(from_trie.size()));
    for (size_t i = 0; i < from_ref.size(); ++i) {
        ASSERT_STR_EQ(from_ref[i].first, from_trie[i].first);
        ASSERT_STR_EQ(from_ref[i].second, from_trie[i].second);
    }
    END_TEST("minimal_next_chain_iterate_matches_map")
}

template <typename Map> void run_map_test_suite(const std::string &map_name) {
    std::vector<std::pair<std::string, TestFunction>> tests = {
        {"basic_put_get", []() { test_basic_put_get<Map>(); }},
        {"get_nonexistent_key", []() { test_get_nonexistent_key<Map>(); }},
        {"overwrite_value", []() { test_overwrite_value<Map>(); }},
        {"erase_nonexistent_noop",
         []() { test_erase_nonexistent_noop<Map>(); }},
        {"erase_existing_key", []() { test_erase_existing_key<Map>(); }},
        {"erase_preserves_other_keys",
         []() { test_erase_preserves_other_keys<Map>(); }},
        {"erase_iterator_advances_like_map",
         []() { test_erase_iterator_advances_like_map<Map>(); }},
        {"erase_iterator_only_element_returns_end",
         []() { test_erase_iterator_only_element_returns_end<Map>(); }},
        {"erase_sequence_matches_map",
         []() { test_erase_sequence_matches_map<Map>(); }},
        {"scan_basic", []() { test_scan_basic<Map>(); }},
        {"scan_no_matches", []() { test_scan_no_matches<Map>(); }},
        {"scan_empty_prefix", []() { test_scan_empty_prefix<Map>(); }},
        {"scan_sorted_order", []() { test_scan_sorted_order<Map>(); }},
        {"scan_partial_prefix", []() { test_scan_partial_prefix<Map>(); }},
        {"scan_range", []() { test_scan_range<Map>(); }},
        {"large_dataset", []() { test_large_dataset<Map>(); }},
        {"special_characters", []() { test_special_characters<Map>(); }},
        {"tree_string", []() { test_tree_string<Map>(); }},
        {"lower_bound_no_key_gte_query_after_prefix",
         []() { test_lower_bound_no_key_gte_query_after_prefix<Map>(); }},
        {"forward_iteration_matches_map_after_insert_order",
         []() {
             test_forward_iteration_matches_map_after_insert_order<Map>();
         }},
        {"minimal_next_chain_iterate_matches_map",
         []() { test_minimal_next_chain_iterate_matches_map<Map>(); }},
    };

    run_test_suite(map_name, tests);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  LLTrie Test Suite" << std::endl;
    std::cout << "  (insert, find, erase, lower_bound, begin, end)"
              << std::endl;
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
