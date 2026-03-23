#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <utility>

namespace ll_trie {

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
struct Node {
    Node *next = nullptr;
    std::string label;
    bool has_value = false;
    ValueType value;
    MapType<char, Node<ValueType, MapType> *> children;

    void clear() {
        for (auto &child : children) {
            delete child.second;
        }
    }

    Node() : next(nullptr), has_value(false) {}

    explicit Node(const std::string &label) : label(label), has_value(false) {}
    explicit Node(const std::string &label, const ValueType &value) :
        label(label), value(value), has_value(true) {}

    ~Node() { clear(); }
};

} // namespace ll_trie
