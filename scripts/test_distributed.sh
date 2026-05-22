#!/bin/bash

set -e

echo "=== Distributed Routing Test ==="
echo

# Test 1: Consistent Hash
echo "[Test 1] Consistent Hash Distribution"
echo "--------------------------------------"

cat > /tmp/test_hash.cpp << 'EOF'
#include "../include/consistent_hash.hpp"
#include <iostream>
#include <map>

int main() {
    ConsistentHash hash(150);

    // Add 3 nodes
    hash.add_node("node1");
    hash.add_node("node2");
    hash.add_node("node3");

    std::cout << "Nodes: " << hash.node_count() << std::endl;

    // Test distribution
    std::map<std::string, int> distribution;
    for (int i = 0; i < 1000; ++i) {
        std::string key = "user_" + std::to_string(i);
        std::string node = hash.get_node(key);
        distribution[node]++;
    }

    std::cout << "Distribution of 1000 keys:" << std::endl;
    for (const auto& [node, count] : distribution) {
        std::cout << "  " << node << ": " << count << " ("
                  << (count * 100.0 / 1000) << "%)" << std::endl;
    }

    // Test node removal
    std::cout << "\nRemoving node2..." << std::endl;
    hash.remove_node("node2");

    distribution.clear();
    for (int i = 0; i < 1000; ++i) {
        std::string key = "user_" + std::to_string(i);
        std::string node = hash.get_node(key);
        distribution[node]++;
    }

    std::cout << "Distribution after removal:" << std::endl;
    for (const auto& [node, count] : distribution) {
        std::cout << "  " << node << ": " << count << " ("
                  << (count * 100.0 / 1000) << "%)" << std::endl;
    }

    return 0;
}
EOF

# Compile and run
cd /home/xuhao/code/gateway-system
clang++-18 -std=c++20 -I./include /tmp/test_hash.cpp src/consistent_hash.cpp -o /tmp/test_hash
/tmp/test_hash

echo
echo "[Test 2] Node Registry (Mock)"
echo "--------------------------------------"
echo "Node registry requires Redis connection."
echo "Skipping live test - see implementation in node_registry.cpp"
echo

echo "=== Tests Complete ==="
