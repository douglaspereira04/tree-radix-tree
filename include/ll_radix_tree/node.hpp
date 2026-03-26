#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <utility>

namespace ll_radix_tree {

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
struct Node {
    Node *next = nullptr;
    std::string label;
    std::string key;
    ValueType value;
    bool has_value = false;
    MapType<char, Node<ValueType, MapType> *> children;

    void clear() {
        for (auto &child : children) {
            delete child.second;
        }
    }

    Node() : next(nullptr), label(""), has_value(false) {}

    explicit Node(std::string &&label) :
        label(std::move(label)), has_value(false) {}
    explicit Node(const std::string &label, const ValueType &value) :
        label(label), value(value), has_value(true) {}
    explicit Node(std::string &&label, std::string &&key, ValueType &&value,
                  MapType<char, Node<ValueType, MapType> *> &&children) :
        label(std::move(label)), key(std::move(key)), value(std::move(value)),
        has_value(true), children(std::move(children)) {}

    ~Node() { clear(); }
};

} // namespace ll_radix_tree
