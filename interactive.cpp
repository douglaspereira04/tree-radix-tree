#include "radix_tree/radix_tree.hpp"

#include <iostream>
#include <sstream>
#include <string>

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
    radix_tree::RadixTree<std::string> tree;

    std::cout << "Radix tree interactive shell. Type 'help' for commands.\n\n";

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
            std::cout << tree.to_string() << '\n';
            continue;
        }
        if (cmd == "put") {
            std::string key, value;
            if (iss >> key) {
                std::getline(iss >> std::ws, value);
                tree.put(key, value.empty() ? "" : value);
                std::cout << "OK\n";
            } else {
                std::cout << "Usage: put <key> <value>\n";
            }
            continue;
        }
        if (cmd == "get") {
            std::string key;
            if (iss >> key) {
                std::string value;
                if (tree.get(key, value)) {
                    std::cout << value << '\n';
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
                tree.scan(prefix, results);
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
