#pragma once

#include <cstddef>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "node.hpp"
#include "utils.hpp"

namespace ll_trie {

// References into ArrayType entries (same object as *value_it); STL-like map
// semantics.
template <typename Value> struct pair_ref {
    const std::string &first;
    Value &second;
    const pair_ref *operator->() const { return this; }
};

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
class LLTrieIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<const std::string, ValueType>;

    using Node_ = Node<ValueType, MapType>;

    LLTrieIterator() = default;
    LLTrieIterator(Node_ *node) : node_(node) {}

    pair_ref<ValueType> operator*() const {
        return {node_->label, node_->value};
    }
    pair_ref<ValueType> operator->() const { return operator*(); }

    template <typename VT, template <typename K, typename V> class MT>
    friend class LLTrie;

    LLTrieIterator &operator++() {
        if (node_ == nullptr)
            return *this;

        do {
            node_ = node_->next;
        } while (node_ != nullptr && !node_->has_value);

        return *this;
    }

    LLTrieIterator operator++(int) { return ++*this; }

    bool operator==(const LLTrieIterator &other) const {
        return node_ == other.node_;
    }

    bool operator!=(const LLTrieIterator &other) const {
        return !(*this == other);
    }

private:
    Node_ *node_ = nullptr;
};

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
class LLTrie {
    using Node_ = Node<ValueType, MapType>;

public:
    LLTrie() : root_(Node_("root")), size_(0) {}

    using KVPairType = std::pair<const std::string, ValueType>;

    using Iterator = LLTrieIterator<ValueType, MapType>;

    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }

    void clear() {
        root_.clear();
        size_ = 0;
    }

    Node_ *next_geq_value_node(Node_ *node, const std::string &k) {
        UTILS_LOG("next_greater_value_node: " + node->label);
        while (node != nullptr) {
            UTILS_LOG("Node: " + node->label);
            UTILS_LOG("Node has value: " + std::to_string(node->has_value));
            UTILS_LOG("Node label: " + node->label);
            UTILS_LOG("k: " + k);
            if (node->has_value && node->label >= k) {
                UTILS_LOG("Found node: " + node->label);
                return node;
            }
            node = node->next;
        }
        UTILS_LOG("No node found");
        return nullptr;
    }

    Node_ *next_value_node(Node_ *node) {
        UTILS_LOG("next_value_node: " + node->label);
        while (node != nullptr && !node->has_value) {
            UTILS_LOG("Next node: " + node->label);
            node = node->next;
        }
        return node;
    }

    Node_ *lower_bound_node(const std::string &k) {
        UTILS_LOG("lower_bound_node: " + k);
        size_t k_char_idx = 0;

        Node_ *node = &root_;
        while (k_char_idx < k.size()) {
            char k_char = k[k_char_idx];
            auto next_node_it = node->children.lower_bound(k_char);
            if (next_node_it != node->children.end()) {
                UTILS_LOG(std::string("Found char: ") + next_node_it->first);
                char lower_bound_char = next_node_it->first;
                Node_ *next_node = next_node_it->second;
                if (lower_bound_char > k_char) {
                    UTILS_LOG(
                        "Lower bound char is greater than k char. Returning "
                        "next value node: ");
                    return next_value_node(next_node);
                }
                node = next_node;
                k_char_idx++;
                continue;
            }
            break;
        }
        UTILS_LOG("Last found node: " + node->label);
        return next_geq_value_node(node, k);
    }

    template <size_t N> Node_ *traverse_prefix(const std::string &k) {
        size_t k_char_idx = 0;

        Node_ *node = &root_;
        while (k_char_idx < k.size() - N) {
            char k_char = k[k_char_idx];
            auto next_node_it = node->children.find(k_char);
            if (next_node_it != node->children.end()) {
                // Advance to the next node
                node = next_node_it->second;
                k_char_idx++;
                continue;
            }
            return nullptr;
        }

        return node;
    }

    Node_ *traverse_rightmost(Node_ *node) {
        while (node->children.size() > 0) {
            node = std::prev(node->children.end())->second;
        }
        return node;
    }

    Node_ *traverse_creating(const std::string &k) {
        size_t k_char_idx = 0;

        Node_ *node = &root_;
        UTILS_LOG("traverse_creating: " + k);
        while (k_char_idx < k.size()) {
            char k_char = k[k_char_idx];
            UTILS_LOG("node: " + node->label);
            UTILS_LOG(std::string("char: ") + k_char);
            auto next_node_it = node->children.lower_bound(k_char);
            if (next_node_it != node->children.end() &&
                next_node_it->first == k_char) {
                // Advance to the next node
                node = next_node_it->second;
                k_char_idx++;
                continue;
            }

            // No child node with the next key character
            // Create intermediate node and advance to it
            // Adjust the next pointers
            Node_ *new_node = new Node_();
            UTILS_LOG("No child node with char. Creating new "
                      "node: ");
            auto &children_it = next_node_it;
            if (children_it != node->children.end()) {
                new_node->next = children_it->second;
                UTILS_LOG("Next of new node: " + new_node->next->label);
                if (children_it != node->children.begin()) {
                    UTILS_LOG("New node will be in the middle");
                    --children_it;
                    Node_ *rightmost_node =
                        traverse_rightmost(children_it->second);
                    rightmost_node->next = new_node;
                } else {
                    UTILS_LOG("New node will be the first");
                    node->next = new_node;
                }
            } else if (node->children.size() > 0) {
                UTILS_LOG("New node will be the last, after the last child");
                Node_ *rightmost_node = traverse_rightmost(node);
                new_node->next = rightmost_node->next;
                rightmost_node->next = new_node;
            } else {
                UTILS_LOG("New node will be the last, and first");
                new_node->next = node->next;
                node->next = new_node;
            }

            node->children[k_char] = new_node;

            node = new_node;
            k_char_idx++;
        }

        return node;
    }

    std::pair<Iterator, bool> insert(const KVPairType &value) {
        const std::string &k = value.first;
        Node_ *node = traverse_creating(k);

        // Reached the node of the key
        // Insert the value if it is not already present
        if (node->has_value) {
            return {Iterator{node}, false};
        }
        node->label = value.first;
        node->value = value.second;
        node->has_value = true;
        ++size_;
        return {Iterator{node}, true};
    }

    std::pair<Iterator, bool> insert(KVPairType &&value) {
        const std::string &k = value.first;
        Node_ *node = traverse_creating(k);

        // Reached the node of the key
        // Insert the value if it is not already present
        if (node->has_value) {
            return {Iterator{node}, false};
        }
        node->label = std::move(value.first);
        node->value = std::move(value.second);
        node->has_value = true;
        ++size_;
        return {Iterator{node}, true};
    }

    template <typename M> std::pair<Iterator, bool>
    insert_or_assign(const std::string &key, M &&value) {
        Node_ *node = traverse_creating(key);

        const bool inserted = !node->has_value;
        node->label = key;
        node->value = std::forward<M>(value);
        node->has_value = true;
        if (inserted) {
            ++size_;
        }
        return {Iterator{node}, inserted};
    }

    template <typename... Args>
    std::pair<Iterator, bool> emplace(Args &&...args) {
        KVPairType v(std::forward<Args>(args)...);
        return insert(std::move(v));
    }

    size_t erase(const std::string &key) {
        Node_ *node = traverse_prefix<0>(key);
        if (node != nullptr && node->has_value) {
            node->has_value = false;
            node->value = ValueType();
            --size_;
            return 1;
        }
        return 0;
    }

    Iterator erase(Iterator it) {
        Node_ *node = it.node_;
        if (node != nullptr && node->has_value) {
            node->has_value = false;
            node->value = ValueType();
            --size_;
            return Iterator{next_value_node(node->next)};
        }
        return Iterator{nullptr};
    }

    Iterator find(const std::string &key) {
        Node_ *node = traverse_prefix<0>(key);
        if (node != nullptr && node->has_value) {
            return Iterator{node};
        }
        return Iterator{nullptr};
    }

    size_t count(const std::string &key) {
        Node_ *node = traverse_prefix<0>(key);
        return node != nullptr && node->has_value ? 1 : 0;
    }

    Iterator lower_bound(const std::string &key) {
        Node_ *node = lower_bound_node(key);
        if (node != nullptr) {
            return Iterator{node};
        }
        return Iterator{nullptr};
    }

    Iterator upper_bound(const std::string &key) {
        Node_ *node = lower_bound_node(key);
        if (node != nullptr) {
            Iterator it = Iterator{node};
            ++it;
            return it;
        }
        return Iterator{nullptr};
    }

    ValueType &at(const std::string &key) {
        Node_ *node = traverse_prefix<0>(key);
        if (node == nullptr)
            throw std::out_of_range("LLTrie::at");
        return node->value;
    }

    Iterator begin() {
        if (root_.has_value) {
            return Iterator{&root_};
        }
        auto it = Iterator{&root_};
        it++;
        return it;
    }

    Iterator end() { return Iterator{nullptr}; }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "(root) entries: " << size_ << "\n";
        for (const auto &kv : root_.values)
            oss << "  " << kv.first << " -> " << kv.second << "\n";
        return oss.str();
    }

    /** DFS dump of the trie (edge labels and stored keys/values). For tests and
     * debugging. */
    std::string tree_string() const {
        std::ostringstream oss;
        tree_string_append(oss, &root_, 0);
        return oss.str();
    }

private:
    void tree_string_append(std::ostringstream &oss, const Node_ *node,
                            int depth) const {
        for (const auto &kv : node->children) {
            const Node_ *child = kv.second;
            std::string indent(static_cast<size_t>(depth) * 2, ' ');
            oss << indent << "[" << kv.first << "]";
            if (child->has_value)
                oss << " \"" << child->label << "\" => " << child->value;
            oss << "\n";
            tree_string_append(oss, child, depth + 1);
        }
    }

    Node_ root_;
    size_t size_ = 0;
};

} // namespace ll_trie
