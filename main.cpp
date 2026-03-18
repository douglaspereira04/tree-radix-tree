#include "radix_tree/radix_tree.hpp"

int main() {
    radix_tree::RadixTree<std::string> t;
    t.put("key", "value");
    std::string v;
    (void)t.get("key", v);
    std::vector<std::pair<std::string, std::string>> results;
    (void)t.scan("k", results);
    return 0;
}
