#pragma once

#include <cstddef>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "node.hpp"
#include "utils.hpp"

namespace concurrent_ll_trie {

// References into ArrayType entries (same object as *value_it); STL-like map
// semantics.
template <typename Value> struct pair_ref {
    const std::string &first;
    Value &second;
    const pair_ref *operator->() const { return this; }
};

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
class ConcurrentLLTrieIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<const std::string, ValueType>;

    using Node_ = Node<ValueType, MapType>;

    ConcurrentLLTrieIterator() = default;
    ConcurrentLLTrieIterator(Node_ *node) : node_(node) {}

    pair_ref<ValueType> operator*() const {
        return {node_->label, node_->value};
    }
    pair_ref<ValueType> operator->() const { return operator*(); }

    template <typename VT, template <typename K, typename V> class MT>
    friend class ConcurrentLLTrie;

    ConcurrentLLTrieIterator &operator++() {
        do {
            Node_ *next_node = node_->next;
            if (next_node != nullptr) {
                next_node->lock.lock();
            }
            node_->lock.unlock();
            node_ = next_node;
        } while (node_ != nullptr && !node_->has_value);

        return *this;
    }

    ConcurrentLLTrieIterator operator++(int) { return ++*this; }

    bool operator==(const ConcurrentLLTrieIterator &other) const {
        return node_ == other.node_;
    }

    bool operator!=(const ConcurrentLLTrieIterator &other) const {
        return !(*this == other);
    }

    void unlock() {
        if (node_ != nullptr) {
            node_->lock.unlock();
        }
    }

    Node_ *node_ = nullptr;
};

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
class ConcurrentLLTrieSharedIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<const std::string, ValueType>;

    using Node_ = Node<ValueType, MapType>;

    ConcurrentLLTrieSharedIterator() = default;
    ConcurrentLLTrieSharedIterator(Node_ *node) : node_(node) {}

    pair_ref<ValueType> operator*() const {
        return {node_->label, node_->value};
    }
    pair_ref<ValueType> operator->() const { return operator*(); }

    template <typename VT, template <typename K, typename V> class MT>
    friend class ConcurrentLLTrie;

    ConcurrentLLTrieSharedIterator &operator++() {
        do {
            Node_ *next_node = node_->next;
            if (next_node != nullptr) {
                next_node->lock.lock_shared();
            }
            node_->lock.unlock_shared();
            node_ = next_node;
        } while (node_ != nullptr && !node_->has_value);

        return *this;
    }

    ConcurrentLLTrieSharedIterator operator++(int) { return ++*this; }

    bool operator==(const ConcurrentLLTrieSharedIterator &other) const {
        return node_ == other.node_;
    }

    bool operator!=(const ConcurrentLLTrieSharedIterator &other) const {
        return !(*this == other);
    }

    void unlock_shared() {
        if (node_ != nullptr) {
            node_->lock.unlock_shared();
        }
    }

    Node_ *node_ = nullptr;
};

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
class ConcurrentLLTrie {
    using Node_ = Node<ValueType, MapType>;

public:
    ConcurrentLLTrie() : root_(Node_("root")), size_(0) {}

    using KVPairType = std::pair<const std::string, ValueType>;

    using Iterator = ConcurrentLLTrieIterator<ValueType, MapType>;
    using SharedIterator = ConcurrentLLTrieSharedIterator<ValueType, MapType>;

    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }

    void clear() {
        root_.clear();
        size_ = 0;
    }

    Node_ *next_geq_value_node(Node_ *node, const std::string &k) {
        // assumes the node is locked and is not nullptr
        UTILS_LOG("next_greater_value_node: " + node->label);
        while (true) {
            UTILS_LOG("Node: " + node->label);
            UTILS_LOG("Node has value: " + std::to_string(node->has_value));
            UTILS_LOG("Node label: " + node->label);
            UTILS_LOG("k: " + k);

            if (node->has_value && node->label >= k) {
                UTILS_LOG("Found node: " + node->label);
                return node;
            }

            Node_ *next_node = node->next;
            if (next_node == nullptr) {
                node->lock.unlock_shared();
                break;
            }
            next_node->lock.lock_shared();
            node->lock.unlock_shared();
            node = next_node;
        }
        UTILS_LOG("No node found");
        return nullptr;
    }

    Node_ *next_value_node_shared(Node_ *node) {
        // assumes the node is shared locked
        UTILS_LOG("next_value_node: " + node->label);
        while (node != nullptr) {
            if (!node->has_value) {
                if (node->next != nullptr) {
                    UTILS_LOG("Next node: " + node->label);
                    Node_ *next_node = node->next;
                    next_node->lock.lock_shared();
                    node->lock.unlock_shared();
                    node = next_node;
                    continue;
                }
                node->lock.unlock_shared();
                return nullptr;
            }
            return node;
        }
        return node;
    }

    Node_ *next_value_node(Node_ *node) {
        UTILS_LOG("next_value_node: " + node->label);
        node->lock.lock();
        while (node != nullptr) {
            if (!node->has_value) {
                if (node->next != nullptr) {
                    UTILS_LOG("Next node: " + node->label);
                    Node_ *next_node = node->next;
                    next_node->lock.lock();
                    node->lock.unlock();
                    node = next_node;
                    continue;
                }
                node->lock.unlock();
                return nullptr;
            }
            return node;
        }
        return node;
    }

    Node_ *lower_bound_node(const std::string &k) {
        UTILS_LOG("lower_bound_node: " + k);
        size_t k_char_idx = 0;

        Node_ *node = &root_;
        node->lock.lock_shared();
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

                    next_node->lock.lock_shared();
                    node->lock.unlock_shared();
                    return next_value_node_shared(next_node);
                }
                next_node->lock.lock_shared();
                node->lock.unlock_shared();
                node = next_node;
                k_char_idx++;
                continue;
            }
            break;
        }
        UTILS_LOG("Last found node: " + node->label);
        return next_geq_value_node(node, k);
    }

    template <size_t N> Node_ *traverse_prefix_shared(const std::string &k) {
        size_t k_char_idx = 0;

        Node_ *node = &root_;

        node->lock.lock_shared();
        while (k_char_idx < k.size() - N) {
            char k_char = k[k_char_idx];
            auto next_node_it = node->children.find(k_char);
            if (next_node_it != node->children.end()) {
                // Advance to the next node
                Node_ *next_node = next_node_it->second;
                next_node->lock.lock_shared();
                node->lock.unlock_shared();
                node = next_node;
                k_char_idx++;
                continue;
            }
            node->lock.unlock_shared();
            return nullptr;
        }

        return node;
    }

    template <size_t N> Node_ *traverse_prefix(const std::string &k) {
        size_t k_char_idx = 0;

        Node_ *node = &root_;

        node->lock.lock();
        while (k_char_idx < k.size() - N) {
            char k_char = k[k_char_idx];
            auto next_node_it = node->children.find(k_char);
            if (next_node_it != node->children.end()) {
                // Advance to the next node
                Node_ *next_node = next_node_it->second;
                next_node->lock.lock();
                node->lock.unlock();
                node = next_node;
                k_char_idx++;
                continue;
            }
            node->lock.unlock();
            return nullptr;
        }

        return node;
    }

    Node_ *traverse_rightmost(Node_ *node) {
        // assumes the node is locked
        while (node->children.size() > 0) {
            Node_ *next_node = std::prev(node->children.end())->second;
            next_node->lock.lock();
            node->lock.unlock();
            node = next_node;
        }
        return node;
    }

    Node_ *traverse_rightmost_from_children(Node_ *node) {
        // assumes the node is locked and size of children is greater than 0
        // will not release lock of first node
        Node_ *next_node = std::prev(node->children.end())->second;
        next_node->lock.lock();
        node = next_node;
        while (node->children.size() > 0) {
            Node_ *next_node = std::prev(node->children.end())->second;
            next_node->lock.lock();
            node->lock.unlock();
            node = next_node;
        }
        return node;
    }

    Node_ *traverse_creating(const std::string &k) {
        size_t k_char_idx = 0;

        Node_ *node = &root_;
        node->lock.lock();
        UTILS_LOG("traverse_creating: " + k);
        while (k_char_idx < k.size()) {
            char k_char = k[k_char_idx];
            UTILS_LOG("node: " + node->label);
            UTILS_LOG(std::string("char: ") + k_char);
            auto next_node_it = node->children.lower_bound(k_char);
            if (next_node_it != node->children.end() &&
                next_node_it->first == k_char) {
                // Advance to the next node
                Node_ *next_node = next_node_it->second;
                next_node->lock.lock();
                node->lock.unlock();
                node = next_node;
                k_char_idx++;
                continue;
            }

            // No child node with the next key character
            // Create intermediate node and advance to it
            // Adjust the next pointers
            Node_ *new_node = new Node_();
            new_node->lock.lock();
            UTILS_LOG("No child node with char. Creating new "
                      "node: ");
            auto &children_it = next_node_it;
            if (children_it != node->children.end()) {
                Node_ *upper_bound_node = children_it->second;
                new_node->next = upper_bound_node;
                UTILS_LOG("Next of new node: " + new_node->next->label);
                if (children_it != node->children.begin()) {
                    UTILS_LOG("New node will be in the middle");
                    --children_it;
                    Node_ *prev_root = children_it->second;
                    prev_root->lock.lock();
                    Node_ *rightmost_node = traverse_rightmost(prev_root);
                    rightmost_node->next = new_node;
                    rightmost_node->lock.unlock();
                } else {
                    UTILS_LOG("New node will be the first");
                    node->next = new_node;
                }
            } else if (node->children.size() > 0) {
                UTILS_LOG("New node will be the last, after the last child");
                Node_ *rightmost_node = traverse_rightmost_from_children(node);
                new_node->next = rightmost_node->next;
                rightmost_node->next = new_node;
                rightmost_node->lock.unlock();
            } else {
                UTILS_LOG("New node will be the last, and first");
                new_node->next = node->next;
                node->next = new_node;
            }

            node->children[k_char] = new_node;
            node->lock.unlock();

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
        size_++;
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
        size_++;
        return {Iterator{node}, true};
    }

    template <typename M> std::pair<Iterator, bool>
    insert_or_assign(const std::string &key, M &&value) {
        Node_ *node = traverse_creating(key);
        node->label = key;
        node->value = std::forward<M>(value);
        bool inserted = !node->has_value;
        node->has_value = true;
        size_++;
        return {Iterator{node}, inserted};
    }

    template <typename... Args>
    std::pair<Iterator, bool> emplace(Args &&...args) {
        KVPairType v(std::forward<Args>(args)...);
        return insert(std::move(v));
    }

    size_t erase(const std::string &key) {
        Node_ *node = traverse_prefix<0>(key);
        if (node != nullptr) {
            if (node->has_value) {
                node->has_value = false;
                node->value = ValueType();
                --size_;
                node->lock.unlock();
                return 1;
            } else {
                node->lock.unlock();
                return 0;
            }
        }

        return 0;
    }

    Iterator erase(Iterator it) {
        Node_ *node = it.node_;
        if (node != nullptr && node->has_value) {
            node->has_value = false;
            node->value = ValueType();
            --size_;
            Node_ *next_node = node->next;
            if (next_node != nullptr) {
                next_node->lock.lock();
            }
            node->lock.unlock();
            return Iterator{next_node};
        }
        return Iterator{nullptr};
    }

    SharedIterator find(const std::string &key) {
        Node_ *node = traverse_prefix_shared<0>(key);
        if (node != nullptr) {
            if (node->has_value) {
                return SharedIterator{node};
            }
            node->lock.unlock_shared();
        }
        return SharedIterator{nullptr};
    }

    size_t count(const std::string &key) {
        Node_ *node = traverse_prefix_shared<0>(key);
        if (node != nullptr) {
            size_t count = node->has_value ? 1 : 0;
            node->lock.unlock_shared();
            return count;
        }
        return 0;
    }

    SharedIterator lower_bound(const std::string &key) {
        Node_ *node = lower_bound_node(key);
        if (node != nullptr) {
            return SharedIterator{node};
        }
        return SharedIterator{nullptr};
    }

    SharedIterator upper_bound(const std::string &key) {
        Node_ *node = lower_bound_node(key);
        if (node != nullptr) {
            SharedIterator it = SharedIterator{node};
            if (node->label == key) {
                ++it;
            }
            return it;
        }
        return SharedIterator{nullptr};
    }

    ValueType &at(const std::string &key) {
        Node_ *node = traverse_prefix_shared<0>(key);
        if (node != nullptr) {
            if (node->has_value) {
                auto v = node->value;
                node->lock.unlock_shared();
                return v;
            }
            node->lock.unlock_shared();
        }
        throw std::out_of_range("ConcurrentLLTrie::at");
    }

    const ValueType &at(const std::string &key) const {
        Node_ *node = traverse_prefix_shared<0>(key);
        if (node != nullptr) {
            if (node->has_value) {
                auto v = node->value;
                node->lock.unlock_shared();
                return v;
            }
            node->lock.unlock_shared();
        }
        throw std::out_of_range("ConcurrentLLTrie::at");
    }

    Iterator begin() {
        Node_ *node = next_value_node(&root_);
        return Iterator{node};
    }

    SharedIterator shared_begin() {
        root_.lock.lock_shared();
        Node_ *node = next_value_node_shared(&root_);
        return SharedIterator{node};
    }

    Iterator end() { return Iterator{nullptr}; }

    SharedIterator shared_end() { return SharedIterator{nullptr}; }

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

    /** DFS all nodes; prints each node whose mutex is currently held, with
     *  label and \c lock (exclusive) or \c lock_shared (readers). Best-effort
     *  probe via \c try_lock / \c try_lock_shared; for single-threaded debug.
     */
    void debug_print_locked_nodes(std::ostream &os = std::cout) {
        debug_print_locked_nodes_impl(os, &root_);
    }

private:
    static std::string probe_shared_mutex_lock_kind(std::shared_mutex &m) {
        if (m.try_lock()) {
            m.unlock();
            return {};
        }
        if (m.try_lock_shared()) {
            m.unlock_shared();
            return "lock_shared";
        }
        return "lock";
    }

    void debug_print_locked_nodes_impl(std::ostream &os, Node_ *node) {
        std::string kind = probe_shared_mutex_lock_kind(node->lock);
        if (!kind.empty())
            os << "label=\"" << node->label << "\" " << kind << '\n';
        for (auto &kv : node->children)
            debug_print_locked_nodes_impl(os, kv.second);
    }

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

} // namespace concurrent_ll_trie
