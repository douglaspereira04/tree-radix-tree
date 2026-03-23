#include "ll_trie/ll_trie.hpp"
#include <string>

int main() {
    ll_trie::LLTrie<std::string> t;
    t.insert({"key", "value"});
    auto it = t.find("key");
    (void)(it != t.end());
    return 0;
}
