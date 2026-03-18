#pragma once

#include <map>
#include <string>
#include <string_view>

namespace radix_tree {

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
struct Node {
    bool has_value = false;
    ValueType value{};
    MapType<char, Node<ValueType, MapType> *> children;
    std::string label;
    void clear_map() { children = MapType<char, Node<ValueType, MapType> *>(); }
    Node() = default;
    Node(const std::string &label) : label(label) {}
    Node(std::string_view label, const ValueType &value) :
        value(value), label(label) {}
    Node(const std::string &label, const ValueType &value) :
        value(value), label(label) {}
    Node(const std::string &label, const ValueType &value,
         const MapType<char, Node<ValueType, MapType> *> &children) :
        value(value), children(children), label(label) {}
    Node(std::string &&label, ValueType &&value,
         MapType<char, Node<ValueType, MapType> *> &&children) :
        value(std::move(value)), children(std::move(children)),
        label(std::move(label)) {}
};

} // namespace radix_tree
