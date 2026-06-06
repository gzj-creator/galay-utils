#include "../test_common.hpp"

void testTrieTree() {
    std::cout << "=== Testing TrieTree ===" << std::endl;

    TrieTree trie;

    trie.add("hello");
    trie.add("help");
    trie.add("world");
    trie.add("hello"); // Duplicate

    assert(trie.size() == 3);
    assert(trie.contains("hello"));
    assert(trie.contains("help"));
    assert(!trie.contains("hel"));

    assert(trie.startsWith("hel"));
    assert(trie.startsWith("wor"));
    assert(!trie.startsWith("xyz"));

    assert(trie.query("hello") == 2); // Added twice
    assert(trie.query("help") == 1);

    auto words = trie.getWordsWithPrefix("hel");
    assert(words.size() == 2);

    assert(trie.remove("hello"));
    assert(!trie.contains("hello"));
    assert(trie.size() == 2);

    std::cout << "TrieTree tests passed!" << std::endl;
}

// ==================== Huffman Tests ====================

void testHuffman() {
    std::cout << "=== Testing Huffman ===" << std::endl;

    // Build table from data
    std::vector<char> data = {'a', 'a', 'a', 'b', 'b', 'c'};
    auto table = HuffmanBuilder<char>::buildFromData(data);

    assert(table.size() == 3);
    assert(table.hasSymbol('a'));
    assert(table.hasSymbol('b'));
    assert(table.hasSymbol('c'));

    // Encode
    HuffmanEncoder<char> encoder(table);
    encoder.encode(data);
    auto encoded = encoder.finish();

    // Decode
    HuffmanDecoder<char> decoder(table, 1, 8);
    auto decoded = decoder.decode(encoded, data.size());

    assert(decoded.size() == data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        assert(decoded[i] == data[i]);
    }

    std::cout << "Huffman tests passed!" << std::endl;
}

// ==================== MVCC Tests ====================

void testMvcc() {
    std::cout << "=== Testing MVCC ===" << std::endl;

    Mvcc<std::string> mvcc;

    // Put values
    Version v1 = mvcc.putValue("value1");
    assert(v1 == 1);

    Version v2 = mvcc.putValue("value2");
    assert(v2 == 2);

    // Read current
    const std::string* current = mvcc.getCurrentValue();
    assert(current != nullptr);
    assert(*current == "value2");

    // Read by version
    const std::string* val1 = mvcc.getValue(v1);
    assert(val1 != nullptr);
    assert(*val1 == "value1");

    // Snapshot
    Snapshot snapshot(v1);
    const std::string* snapshotVal = snapshot.read(mvcc);
    assert(snapshotVal != nullptr);
    assert(*snapshotVal == "value1");

    // Transaction
    Transaction<std::string> txn(mvcc);
    const std::string* readVal = txn.read();
    assert(readVal != nullptr);
    assert(*readVal == "value2");

    txn.write(std::make_unique<std::string>("value3"));
    assert(txn.commit());
    assert(*mvcc.getCurrentValue() == "value3");

    // GC
    assert(mvcc.versionCount() == 3);
    mvcc.gc(2);
    assert(mvcc.versionCount() == 2);

    std::cout << "MVCC tests passed!" << std::endl;
}

// ==================== Parser Tests ====================

int main() {
    std::cout << "\n=== data_test ===" << std::endl;
    try {
        testTrieTree();
        testHuffman();
        testMvcc();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
