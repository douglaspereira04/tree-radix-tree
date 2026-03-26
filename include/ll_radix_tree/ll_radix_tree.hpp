#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "node.hpp"
#include "utils.hpp"

extern "C" {
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
}

namespace ll_radix_tree {

// References into ArrayType entries (same object as *value_it); STL-like map
// semantics.
template <typename Value> struct pair_ref {
    const std::string &first;
    Value &second;
    const pair_ref *operator->() const { return this; }
};

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
class LLRadixTreeIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<const std::string, ValueType>;

    using Node_ = Node<ValueType, MapType>;

    LLRadixTreeIterator() = default;
    LLRadixTreeIterator(Node_ *node) : node_(node) {}

    pair_ref<ValueType> operator*() const { return {node_->key, node_->value}; }
    pair_ref<ValueType> operator->() const { return operator*(); }

    template <typename VT, template <typename K, typename V> class MT>
    friend class LLRadixTree;

    LLRadixTreeIterator &operator++() {
        if (node_ == nullptr)
            return *this;

        do {
            node_ = node_->next;
        } while (node_ != nullptr && !node_->has_value);

        return *this;
    }

    LLRadixTreeIterator operator++(int) { return ++*this; }

    bool operator==(const LLRadixTreeIterator &other) const {
        return node_ == other.node_;
    }

    bool operator!=(const LLRadixTreeIterator &other) const {
        return !(*this == other);
    }

private:
    Node_ *node_ = nullptr;
};

template <typename ValueType,
          template <typename K, typename V> class MapType = std::map>
class LLRadixTree {
    using Node_ = Node<ValueType, MapType>;

public:
    LLRadixTree() : root_(Node_("root")), size_(0) {}

    using KVPairType = std::pair<const std::string, ValueType>;

    using Iterator = LLRadixTreeIterator<ValueType, MapType>;

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
            if (node->has_value && node->key >= k) {
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
                Node_ *next = next_node_it->second;
                if (lower_bound_char > k_char) {
                    UTILS_LOG(
                        "Lower bound char is greater than k char. Returning "
                        "next value node: ");
                    return next_value_node(next);
                }
                size_t common_prefix =
                    common_prefix_length(next->label, k, k_char_idx + 1);
                if (common_prefix == next->label.size()) {
                    node = next;
                    k_char_idx += common_prefix + 1;
                    continue;
                }

                if (next->label[common_prefix] >=
                    k[k_char_idx + common_prefix + 1]) {
                    return next_value_node(next);
                }
                node = next;
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
                Node_ *possible_next = next_node_it->second;
                size_t common_prefix = common_prefix_length(
                    possible_next->label, k, k_char_idx + 1);
                if (common_prefix == possible_next->label.size()) {
                    node = possible_next;
                    k_char_idx += common_prefix + 1;
                    continue;
                }
                return nullptr;
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

    Node_ *split(Node_ *node, const std::string &k, size_t k_char_idx,
                 size_t common_prefix) {
        std::string &label = node->label;

        if (k_char_idx + common_prefix + 1 == k.size()) {
            // key is prefix of label
            // new key will be in the middle of current existing path
            // it will assume the current node and a new node
            // will contain the continuation.

            // create the node to hold existing subtree
            char old_branch_char = label[common_prefix];
            std::string old_branch_label = label.substr(common_prefix + 1);

            Node_ *old_branch_node =
                new Node_(std::move(old_branch_label), std::move(node->key),
                          std::move(node->value), std::move(node->children));
            old_branch_node->next = node->next;
            old_branch_node->has_value = node->has_value;

            // updating the node to be the middle node
            // its key and value will be in the old_branch node
            node->key = std::string();
            node->value = ValueType();
            node->label.resize(common_prefix);
            node->children.clear();
            node->has_value = false;

            node->children[old_branch_char] = old_branch_node;

            node->next = old_branch_node;

            return node;
        }

        // it will branch in two paths
        // create new branch to hold existing subtree
        char old_branch_char = label[common_prefix];
        std::string old_branch_label = label.substr(common_prefix + 1);

        Node_ *old_branch_node =
            new Node_(std::move(old_branch_label), std::move(node->key),
                      std::move(node->value), std::move(node->children));
        old_branch_node->has_value = node->has_value;

        // create new branch to new subtree
        char new_branch_char = k[k_char_idx + 1 + common_prefix];
        std::string new_branch_label =
            k.substr(k_char_idx + 1 + common_prefix + 1);
        Node_ *new_branch_node = new Node_(std::move(new_branch_label));

        // updating old branch root to have both new branches
        // its key and value will be in the old_branch node
        node->key = std::string();
        node->value = ValueType();
        node->label.resize(common_prefix);
        node->children.clear();
        node->has_value = false;

        node->children[old_branch_char] = old_branch_node;
        node->children[new_branch_char] = new_branch_node;

        // Adjust next pointers
        Node_ *node_next = node->next;
        if (new_branch_char > old_branch_char) {
            node->next = old_branch_node;
            old_branch_node->next = node_next;
            Node_ *rightmost = traverse_rightmost(old_branch_node);
            new_branch_node->next = rightmost->next;
            rightmost->next = new_branch_node;
        } else {
            node->next = new_branch_node;
            new_branch_node->next = old_branch_node;
            old_branch_node->next = node_next;
        }

        return new_branch_node;
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

                // Get the next node by splitting if necessary
                Node_ *possible_next = next_node_it->second;
                size_t common_prefix = common_prefix_length(
                    possible_next->label, k, k_char_idx + 1);
                if (common_prefix == possible_next->label.size()) {
                    node = possible_next;
                    k_char_idx += common_prefix + 1;
                } else {
                    node = split(possible_next, k, k_char_idx, common_prefix);
                    return node;
                }
                continue;
            }

            // No child node with the next key character
            // Create node and return it
            std::string new_label = k.substr(k_char_idx + 1);
            Node_ *new_node = new Node_(std::move(new_label));
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
            return node;
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
        node->key = value.first;
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
        node->key = std::move(value.first);
        node->value = std::move(value.second);
        node->has_value = true;
        ++size_;
        return {Iterator{node}, true};
    }

    template <typename M> std::pair<Iterator, bool>
    insert_or_assign(const std::string &key, M &&value) {
        Node_ *node = traverse_creating(key);

        const bool inserted = !node->has_value;
        node->key = key;
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
            node->key = std::string();
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
            node->key = std::string();
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
        auto it = root_.values.find(key);
        if (it == root_.values.end())
            throw std::out_of_range("LLRadixTree::at");
        return it->second;
    }

    const ValueType &at(const std::string &key) const {
        auto it = root_.values.find(key);
        if (it == root_.values.end())
            throw std::out_of_range("LLRadixTree::at");
        return it->second;
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

    /** Renders the tree to a PNG in the temp directory using libgraphviz
     * (cgraph + gvc: layout + `gvRenderFilename`). Link with `libgvc` (e.g.
     * pkg-config `libgvc`). */
    void display_tree() const {
        GVC_t *gvc = gvContext();
        if (gvc == nullptr)
            return;

        Agraph_t *g =
            agopen(const_cast<char *>("LLRadixTree"), Agdirected, nullptr);
        if (g == nullptr) {
            gvFreeContext(gvc);
            return;
        }

        agsafeset(g, const_cast<char *>("rankdir"), const_cast<char *>("TB"),
                  const_cast<char *>(""));

        size_t next_id = 0;
        char empty_edge_name[] = "";
        std::unordered_map<const Node_ *, Agnode_t *> node_map;
        graphviz_emit_cgraph(g, nullptr, &root_, next_id, 0, '\0',
                             empty_edge_name, node_map);

        size_t next_edge_seq = 0;
        graphviz_add_next_edges(g, &root_, node_map, next_edge_seq);

        if (gvLayout(gvc, g, "dot") != 0) {
            agclose(g);
            gvFreeContext(gvc);
            return;
        }

        namespace fs = std::filesystem;
        const fs::path tmp = fs::temp_directory_path();
        const auto stamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        const fs::path png_file =
            tmp / ("ll_radix_tree_" + std::to_string(stamp) + ".png");

        gvRenderFilename(gvc, g, "png", png_file.string().c_str());

        gvFreeLayout(gvc, g);
        agclose(g);
        gvFreeContext(gvc);
    }

private:
    void graphviz_add_next_edges(
        Agraph_t *graph, const Node_ *node,
        const std::unordered_map<const Node_ *, Agnode_t *> &node_map,
        size_t &next_edge_seq) const {
        if (node->next != nullptr) {
            auto it_from = node_map.find(node);
            auto it_to = node_map.find(node->next);
            if (it_from != node_map.end() && it_to != node_map.end()) {
                std::string ename = "next" + std::to_string(next_edge_seq++);
                Agedge_t *e = agedge(graph, it_from->second, it_to->second,
                                     const_cast<char *>(ename.c_str()), 1);
                if (e != nullptr) {
                    agsafeset(e, const_cast<char *>("style"),
                              const_cast<char *>("dashed"),
                              const_cast<char *>(""));
                    agsafeset(e, const_cast<char *>("color"),
                              const_cast<char *>("gray40"),
                              const_cast<char *>(""));
                    agsafeset(e, const_cast<char *>("constraint"),
                              const_cast<char *>("false"),
                              const_cast<char *>(""));
                }
            }
        }
        for (const auto &kv : node->children) {
            graphviz_add_next_edges(graph, kv.second, node_map, next_edge_seq);
        }
    }

    void graphviz_emit_cgraph(
        Agraph_t *graph, Agnode_t *parent_ag, const Node_ *node,
        size_t &next_id, int depth, char edge_from_parent,
        char *empty_edge_name,
        std::unordered_map<const Node_ *, Agnode_t *> &node_map) const {
        const std::string name = "n" + std::to_string(next_id++);
        Agnode_t *n = agnode(graph, const_cast<char *>(name.c_str()), 1);
        if (n == nullptr)
            return;

        node_map[node] = n;

        std::ostringstream label_oss;
        if (depth == 0) {
            label_oss << "root\nsize=" << size_;
        } else {
            label_oss << "label: " << node->label;
            if (node->has_value) {
                label_oss << "\nkey: " << node->key;
                std::ostringstream vo;
                vo << node->value;
                label_oss << "\nval: " << vo.str();
            }
        }
        const std::string label_str = label_oss.str();
        agsafeset(n, const_cast<char *>("shape"), const_cast<char *>("box"),
                  const_cast<char *>(""));
        agsafeset(n, const_cast<char *>("fontname"),
                  const_cast<char *>("monospace"), const_cast<char *>(""));
        agsafeset(n, const_cast<char *>("label"),
                  const_cast<char *>(label_str.c_str()),
                  const_cast<char *>(""));

        if (parent_ag != nullptr) {
            Agedge_t *e = agedge(graph, parent_ag, n, empty_edge_name, 1);
            if (e != nullptr) {
                agsafeset(e, const_cast<char *>("fontname"),
                          const_cast<char *>("monospace"),
                          const_cast<char *>(""));
                const std::string el(1, edge_from_parent);
                agsafeset(e, const_cast<char *>("label"),
                          const_cast<char *>(el.c_str()),
                          const_cast<char *>(""));
            }
        }

        for (const auto &kv : node->children) {
            graphviz_emit_cgraph(graph, n, kv.second, next_id, depth + 1,
                                 kv.first, empty_edge_name, node_map);
        }
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

} // namespace ll_radix_tree
