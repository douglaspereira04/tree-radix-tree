#include "ll_trie/ll_trie.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void print_help() {
    std::cout << "Commands:\n"
              << "  put <key> <value>   Insert or update key-value pair\n"
              << "  get <key>           Retrieve value for key\n"
              << "  scan <prefix>       List all keys with given prefix\n"
              << "  print               Show tree structure\n"
              << "  help                Show this help\n"
              << "  quit                Exit\n\n";
}

int main() {
    ll_trie::LLTrie<std::string> tree;

    std::cout << "LLTrie interactive shell. Type 'help' for commands.\n\n";

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        if (!(iss >> cmd))
            continue;

        if (cmd == "quit" || cmd == "exit" || cmd == "q") {
            break;
        }
        if (cmd == "help" || cmd == "h") {
            print_help();
            continue;
        }
        if (cmd == "print" || cmd == "tree" || cmd == "p") {
            std::cout << tree.tree_string() << '\n';
            continue;
        }
        if (cmd == "put") {
            std::string key, value;
            if (iss >> key) {
                std::getline(iss >> std::ws, value);
                tree.insert_or_assign(key, value.empty() ? "" : value);
                std::cout << "OK\n";
            } else {
                std::cout << "Usage: put <key> <value>\n";
            }
            continue;
        }
        if (cmd == "get") {
            std::string key;
            if (iss >> key) {
                auto it = tree.find(key);
                if (it != tree.end()) {
                    std::cout << it->second << '\n';
                } else {
                    std::cout << "(not found)\n";
                }
            } else {
                std::cout << "Usage: get <key>\n";
            }
            continue;
        }
        if (cmd == "scan") {
            std::string prefix;
            if (iss >> prefix) {
                std::vector<std::pair<std::string, std::string>> results;
                for (auto it = tree.lower_bound(prefix); it != tree.end();
                     ++it) {
                    if (it->first.compare(0, prefix.size(), prefix) != 0)
                        break;
                    results.push_back({it->first, it->second});
                }
                for (const auto &[k, v] : results) {
                    std::cout << "  " << k << " → " << v << '\n';
                }
                if (results.empty()) {
                    std::cout << "  (none)\n";
                }
            } else {
                std::cout << "Usage: scan <prefix>\n";
            }
            continue;
        }

        std::cout << "Unknown command '" << cmd
                  << "'. Type 'help' for commands.\n";
    }

    return 0;
}
