#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <utility>

#include <shared_mutex>
namespace concurrent_ll_trie {

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
struct Node {
    Node *next = nullptr;
    std::string label;
    bool has_value = false;
    std::shared_mutex lock;
    ValueType value;
    MapType<char, Node<ValueType, MapType> *> children;

    void clear() {
        lock.lock();
        for (auto &child : children) {
            delete child.second;
        }
        lock.unlock();
    }

    Node() : next(nullptr), label(""), has_value(false) {}

    explicit Node(const std::string &label) : label(label), has_value(false) {}
    explicit Node(const std::string &label, const ValueType &value) :
        label(label), value(value), has_value(true) {}

    ~Node() { clear(); }
};

} // namespace concurrent_ll_trie
