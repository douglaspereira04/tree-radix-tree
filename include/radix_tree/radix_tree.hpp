#pragma once

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../utils.hpp"
#include "node.hpp"

namespace radix_tree {

template <typename Value,
          template <typename K, typename V> class MapType = std::map>
class RadixTree {

    using Node_ = Node<Value, MapType>;

public:
    void put(const std::string &key, const Value &value);
    bool get(const std::string &key, Value &value) const;
    bool scan(const std::string &prefix,
              std::vector<std::pair<std::string, Value>> &results) const;
    std::string to_string() const;

private:
    Node_ root_;
};

template <typename Value, template <typename K, typename V> class MapType> void
RadixTree<Value, MapType>::put(const std::string &key, const Value &value) {

    Node_ *node = &root_;
    std::string_view key_view(key);
    size_t key_char_index = 0;

    /*
    Example:
    Keys in the tree:
    "Casa", "Casei"

    Root
    |
    C
    "as"____
    |       |
    E       A
    "i"      ""

    Adding "Case"
    Root
    |
    C
    "as"____
    |       |
    E       A
    ""      ""
    |
    I
    ""
    */

    auto next_node_it = node->children.find(key_view[0]);
    if (next_node_it == node->children.end()) {
        Node_ *new_node = new Node_(key_view.substr(1), value);
        node->children[key_view[0]] = new_node;
        new_node->has_value = true;
        return;
    }
    node = next_node_it->second;

    while (key_char_index < key.size()) {

        std::string_view key_label(key_view.substr(key_char_index + 1));
        size_t common_prefix = common_prefix_length(node->label, key_label);

        if (common_prefix == node->label.size()) {

            if (common_prefix == key_label.size()) {
                // found
                node->value = value;
                node->has_value = true;
                return;
            }

            char next_key_char = key_label[common_prefix];
            auto next_node_it = node->children.find(next_key_char);
            if (next_node_it != node->children.end()) {
                // there is a child node with the next key character
                node = next_node_it->second;
                key_char_index += common_prefix + 1;
                continue;
            }

            // no child node with the next key character
            // branching from here
            Node_ *new_node =
                new Node_(key_label.substr(common_prefix + 1), value);
            new_node->has_value = true;
            node->children[next_key_char] = new_node;
            return;
        }

        // requires splitting the node

        // create new node for splitting
        // new node will hold the remaining of the original node and link the
        // children
        Node_ *new_node =
            new Node_(std::move(node->label), std::move(node->value),
                      std::move(node->children));
        new_node->has_value = node->has_value;
        char new_node_key_char = new_node->label[common_prefix];
        new_node->label.erase(0, common_prefix + 1);

        node->clear_map();
        node->children[new_node_key_char] = new_node;

        if (common_prefix == key_label.size()) {
            // existing node will hold the new key value
            node->label = key_label;
            node->value = value;
            node->has_value = true;

            return;
        }

        // existing node will be just a linking between two branches
        node->label = key_label.substr(0, common_prefix);
        node->has_value = false;

        // new branch
        Node_ *new_right_node =
            new Node_(key_label.substr(common_prefix + 1), value);
        node->children[key_label[common_prefix]] = new_right_node;
        new_right_node->has_value = true;

        return;
    }
}

template <typename Value, template <typename K, typename V> class MapType> bool
RadixTree<Value, MapType>::get(const std::string &key, Value &value) const {
    const Node_ *node = &root_;
    std::string_view key_view(key);
    size_t key_char_index = 0;

    /*
    Example:
    Keys in the tree:
    "Casa", "Casei"

    Root
    |
    C
    "as"____
    |       |
    E       A
    "i"      ""
    */

    auto next_node_it = node->children.find(key_view[0]);
    if (next_node_it == node->children.end()) {
        return false;
    }
    node = next_node_it->second;

    while (key_char_index < key.size()) {

        std::string_view key_label(key_view.substr(key_char_index + 1));
        size_t common_prefix = common_prefix_length(node->label, key_label);

        if (common_prefix == node->label.size()) {

            if (common_prefix == key_label.size()) {
                // found
                value = node->value;
                return node->has_value;
            }

            char next_key_char = key_label[common_prefix];
            auto next_node_it = node->children.find(next_key_char);
            if (next_node_it != node->children.end()) {
                // there is a child node with the next key character
                node = next_node_it->second;
                key_char_index += common_prefix + 1;
                continue;
            }
            return false;
        }
        return false;
    }
    return false;
}

template <typename Value, template <typename K, typename V> class MapType>
bool RadixTree<Value, MapType>::scan(
    const std::string &prefix,
    std::vector<std::pair<std::string, Value>> &results) const {
    (void)prefix;
    (void)results;
    // unimplemented
    return false;
}

namespace {

template <typename Value> std::string value_to_string(const Value &v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

template <typename Node> void to_string_impl(const Node *node, std::string &out,
                                             const std::string &prefix,
                                             bool is_last, char edge_char) {
    if (!node)
        return;

    std::string value_str;
    if (node->has_value) {
        value_str = " → " + value_to_string(node->value);
    }

    out += prefix;
    out += is_last ? "└── " : "├── ";
    out += "'";
    out += edge_char;
    out += "' \"";
    out += node->label;
    out += "\"";
    out += value_str;
    out += "\n";

    std::string child_prefix = prefix + (is_last ? "    " : "│   ");
    auto it = node->children.begin();
    auto end = node->children.end();
    while (it != end) {
        auto next = std::next(it);
        bool last_child = (next == end);
        to_string_impl(it->second, out, child_prefix, last_child, it->first);
        it = next;
    }
}

} // namespace

template <typename Value, template <typename K, typename V> class MapType>
std::string RadixTree<Value, MapType>::to_string() const {
    std::string out = "(root)\n";
    auto it = root_.children.begin();
    auto end = root_.children.end();
    while (it != end) {
        auto next = std::next(it);
        bool last_child = (next == end);
        to_string_impl(it->second, out, "", last_child, it->first);
        it = next;
    }
    return out;
}

} // namespace radix_tree
