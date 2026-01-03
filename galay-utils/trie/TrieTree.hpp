#ifndef GALAY_UTILS_TRIE_TREE_HPP
#define GALAY_UTILS_TRIE_TREE_HPP

#include "../common/Defn.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace galay::utils {

class TrieNode {
public:
    TrieNode() : m_isEnd(false), m_count(0) {}

    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool m_isEnd;
    int m_count;
};

class TrieTree {
public:
    TrieTree() : m_root(std::make_unique<TrieNode>()), m_size(0) {}

    void add(const std::string& word) {
        if (word.empty()) return;

        TrieNode* node = m_root.get();
        for (char c : word) {
            if (node->children.find(c) == node->children.end()) {
                node->children[c] = std::make_unique<TrieNode>();
            }
            node = node->children[c].get();
        }

        if (!node->m_isEnd) {
            node->m_isEnd = true;
            ++m_size;
        }
        ++node->m_count;
    }

    bool contains(const std::string& word) const {
        TrieNode* node = findNode(word);
        return node != nullptr && node->m_isEnd;
    }

    bool startsWith(const std::string& prefix) const {
        return findNode(prefix) != nullptr;
    }

    int query(const std::string& word) const {
        TrieNode* node = findNode(word);
        if (node && node->m_isEnd) {
            return node->m_count;
        }
        return 0;
    }

    bool remove(const std::string& word) {
        if (word.empty() || !contains(word)) {
            return false;
        }

        removeHelper(m_root.get(), word, 0);
        --m_size;
        return true;
    }

    std::vector<std::string> getWordsWithPrefix(const std::string& prefix) const {
        std::vector<std::string> result;
        TrieNode* node = findNode(prefix);

        if (node) {
            collectWords(node, prefix, result);
        }

        return result;
    }

    std::vector<std::string> getAllWords() const {
        std::vector<std::string> result;
        collectWords(m_root.get(), "", result);
        return result;
    }

    size_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }

    void clear() {
        m_root = std::make_unique<TrieNode>();
        m_size = 0;
    }

private:
    TrieNode* findNode(const std::string& prefix) const {
        TrieNode* node = m_root.get();
        for (char c : prefix) {
            auto it = node->children.find(c);
            if (it == node->children.end()) {
                return nullptr;
            }
            node = it->second.get();
        }
        return node;
    }

    void collectWords(TrieNode* node, const std::string& prefix, std::vector<std::string>& result) const {
        if (node->m_isEnd) {
            result.push_back(prefix);
        }

        for (const auto& [c, child] : node->children) {
            collectWords(child.get(), prefix + c, result);
        }
    }

    bool removeHelper(TrieNode* node, const std::string& word, size_t depth) {
        if (depth == word.length()) {
            if (node->m_isEnd) {
                node->m_isEnd = false;
                node->m_count = 0;
                return node->children.empty();
            }
            return false;
        }

        char c = word[depth];
        auto it = node->children.find(c);
        if (it == node->children.end()) {
            return false;
        }

        bool shouldDelete = removeHelper(it->second.get(), word, depth + 1);

        if (shouldDelete) {
            node->children.erase(it);
            return !node->m_isEnd && node->children.empty();
        }

        return false;
    }

    std::unique_ptr<TrieNode> m_root;
    size_t m_size;
};

} // namespace galay::utils

#endif // GALAY_UTILS_TRIE_TREE_HPP
