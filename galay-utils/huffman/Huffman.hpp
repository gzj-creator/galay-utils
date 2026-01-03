#ifndef GALAY_UTILS_HUFFMAN_HPP
#define GALAY_UTILS_HUFFMAN_HPP

#include "../common/Defn.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include <stdexcept>

namespace galay::utils {

struct HuffmanCode {
    uint32_t code;
    uint8_t length;

    HuffmanCode() : code(0), length(0) {}
    HuffmanCode(uint32_t c, uint8_t len) : code(c), length(len) {}
};

template<typename T>
struct HuffmanNode {
    T symbol;
    size_t frequency;
    std::unique_ptr<HuffmanNode> left;
    std::unique_ptr<HuffmanNode> right;

    HuffmanNode(T sym, size_t freq)
        : symbol(sym), frequency(freq) {}

    HuffmanNode(std::unique_ptr<HuffmanNode> l, std::unique_ptr<HuffmanNode> r)
        : symbol(), frequency(l->frequency + r->frequency)
        , left(std::move(l)), right(std::move(r)) {}

    bool isLeaf() const { return !left && !right; }
};

template<typename T>
class HuffmanTable {
public:
    HuffmanTable() = default;

    void addCode(const T& symbol, uint32_t code, uint8_t length) {
        if (length > 32) {
            throw std::invalid_argument("Code length exceeds 32 bits");
        }
        m_encodeTable[symbol] = HuffmanCode(code, length);
        m_decodeTable[{code, length}] = symbol;
    }

    const HuffmanCode& getCode(const T& symbol) const {
        auto it = m_encodeTable.find(symbol);
        if (it == m_encodeTable.end()) {
            throw std::runtime_error("Symbol not found in table");
        }
        return it->second;
    }

    bool hasSymbol(const T& symbol) const {
        return m_encodeTable.find(symbol) != m_encodeTable.end();
    }

    const T& getSymbol(uint32_t code, uint8_t length) const {
        auto it = m_decodeTable.find({code, length});
        if (it == m_decodeTable.end()) {
            throw std::runtime_error("Code not found in table");
        }
        return it->second;
    }

    bool tryGetSymbol(uint32_t code, uint8_t length, T& symbol) const {
        auto it = m_decodeTable.find({code, length});
        if (it != m_decodeTable.end()) {
            symbol = it->second;
            return true;
        }
        return false;
    }

    std::vector<T> getSymbols() const {
        std::vector<T> result;
        result.reserve(m_encodeTable.size());
        for (const auto& [sym, code] : m_encodeTable) {
            result.push_back(sym);
        }
        return result;
    }

    size_t size() const { return m_encodeTable.size(); }

    void clear() {
        m_encodeTable.clear();
        m_decodeTable.clear();
    }

private:
    struct CodeKey {
        uint32_t code;
        uint8_t length;

        bool operator==(const CodeKey& other) const {
            return code == other.code && length == other.length;
        }
    };

    struct CodeKeyHash {
        size_t operator()(const CodeKey& key) const {
            return std::hash<uint32_t>()(key.code) ^
                   (std::hash<uint8_t>()(key.length) << 1);
        }
    };

    std::unordered_map<T, HuffmanCode> m_encodeTable;
    std::unordered_map<CodeKey, T, CodeKeyHash> m_decodeTable;
};

template<typename T>
class HuffmanEncoder {
public:
    explicit HuffmanEncoder(const HuffmanTable<T>& table)
        : m_table(table), m_buffer(0), m_bufferBits(0) {}

    void encode(const T& symbol) {
        const auto& code = m_table.getCode(symbol);
        appendBits(code.code, code.length);
    }

    void encode(const std::vector<T>& symbols) {
        for (const auto& sym : symbols) {
            encode(sym);
        }
    }

    std::vector<uint8_t> finish() {
        if (m_bufferBits > 0) {
            m_output.push_back(static_cast<uint8_t>(m_buffer << (8 - m_bufferBits)));
        }
        auto result = std::move(m_output);
        reset();
        return result;
    }

    size_t bitCount() const {
        return m_output.size() * 8 + m_bufferBits;
    }

    void reset() {
        m_output.clear();
        m_buffer = 0;
        m_bufferBits = 0;
    }

private:
    void appendBits(uint32_t bits, uint8_t count) {
        while (count > 0) {
            uint8_t available = 8 - m_bufferBits;
            uint8_t toWrite = std::min(available, count);

            uint8_t shift = count - toWrite;
            uint8_t extracted = (bits >> shift) & ((1 << toWrite) - 1);

            m_buffer = (m_buffer << toWrite) | extracted;
            m_bufferBits += toWrite;
            count -= toWrite;

            if (m_bufferBits == 8) {
                m_output.push_back(static_cast<uint8_t>(m_buffer));
                m_buffer = 0;
                m_bufferBits = 0;
            }
        }
    }

    const HuffmanTable<T>& m_table;
    std::vector<uint8_t> m_output;
    uint32_t m_buffer;
    uint8_t m_bufferBits;
};

template<typename T>
class HuffmanDecoder {
public:
    HuffmanDecoder(const HuffmanTable<T>& table, uint8_t minCodeLen = 1, uint8_t maxCodeLen = 32)
        : m_table(table), m_minCodeLen(minCodeLen), m_maxCodeLen(maxCodeLen) {}

    std::vector<T> decode(const std::vector<uint8_t>& data, size_t symbolCount = 0) {
        std::vector<T> result;
        uint32_t code = 0;
        uint8_t codeLen = 0;
        size_t bitIndex = 0;

        while (bitIndex < data.size() * 8) {
            size_t byteIndex = bitIndex / 8;
            size_t bitOffset = 7 - (bitIndex % 8);
            uint8_t bit = (data[byteIndex] >> bitOffset) & 1;

            code = (code << 1) | bit;
            ++codeLen;
            ++bitIndex;

            if (codeLen >= m_minCodeLen) {
                T symbol;
                if (m_table.tryGetSymbol(code, codeLen, symbol)) {
                    result.push_back(symbol);
                    code = 0;
                    codeLen = 0;

                    if (symbolCount > 0 && result.size() >= symbolCount) {
                        break;
                    }
                }
            }

            if (codeLen > m_maxCodeLen) {
                throw std::runtime_error("Invalid Huffman code");
            }
        }

        return result;
    }

private:
    const HuffmanTable<T>& m_table;
    uint8_t m_minCodeLen;
    uint8_t m_maxCodeLen;
};

template<typename T>
class HuffmanBuilder {
public:
    static HuffmanTable<T> build(const std::unordered_map<T, size_t>& frequencies) {
        if (frequencies.empty()) {
            return HuffmanTable<T>();
        }

        auto cmp = [](const std::unique_ptr<HuffmanNode<T>>& a,
                      const std::unique_ptr<HuffmanNode<T>>& b) {
            return a->frequency > b->frequency;
        };

        std::priority_queue<std::unique_ptr<HuffmanNode<T>>,
                           std::vector<std::unique_ptr<HuffmanNode<T>>>,
                           decltype(cmp)> pq(cmp);

        for (const auto& [symbol, freq] : frequencies) {
            pq.push(std::make_unique<HuffmanNode<T>>(symbol, freq));
        }

        while (pq.size() > 1) {
            auto left = std::move(const_cast<std::unique_ptr<HuffmanNode<T>>&>(pq.top()));
            pq.pop();
            auto right = std::move(const_cast<std::unique_ptr<HuffmanNode<T>>&>(pq.top()));
            pq.pop();

            pq.push(std::make_unique<HuffmanNode<T>>(std::move(left), std::move(right)));
        }

        HuffmanTable<T> table;
        if (!pq.empty()) {
            generateCodes(pq.top().get(), 0, 0, table);
        }

        return table;
    }

    static HuffmanTable<T> buildFromData(const std::vector<T>& data) {
        std::unordered_map<T, size_t> frequencies;
        for (const auto& item : data) {
            ++frequencies[item];
        }
        return build(frequencies);
    }

private:
    static void generateCodes(HuffmanNode<T>* node, uint32_t code, uint8_t length,
                              HuffmanTable<T>& table) {
        if (!node) return;

        if (node->isLeaf()) {
            if (length == 0) {
                table.addCode(node->symbol, 0, 1);
            } else {
                table.addCode(node->symbol, code, length);
            }
            return;
        }

        generateCodes(node->left.get(), (code << 1), length + 1, table);
        generateCodes(node->right.get(), (code << 1) | 1, length + 1, table);
    }
};

} // namespace galay::utils

#endif // GALAY_UTILS_HUFFMAN_HPP
