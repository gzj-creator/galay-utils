#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <iomanip>

// Include all modules
#include "../galay-utils/galay-utils.hpp"

// galay-kernel for coroutine tests
#include <galay-kernel/kernel/Runtime.h>

using namespace galay::utils;

// ==================== String Tests ====================
void testString() {
    std::cout << "=== Testing String ===" << std::endl;

    // Split
    auto parts = StringUtils::split("a,b,c", ',');
    assert(parts.size() == 3);
    assert(parts[0] == "a" && parts[1] == "b" && parts[2] == "c");

    // Join
    assert(StringUtils::join(parts, "-") == "a-b-c");

    // Trim
    assert(StringUtils::trim("  hello  ") == "hello");
    assert(StringUtils::trimLeft("  hello") == "hello");
    assert(StringUtils::trimRight("hello  ") == "hello");

    // Case conversion
    assert(StringUtils::toLower("HELLO") == "hello");
    assert(StringUtils::toUpper("hello") == "HELLO");

    // StartsWith/EndsWith
    assert(StringUtils::startsWith("hello world", "hello"));
    assert(StringUtils::endsWith("hello world", "world"));
    assert(StringUtils::contains("hello world", "lo wo"));

    // Replace
    assert(StringUtils::replace("aaa", "a", "b") == "bbb");
    assert(StringUtils::replaceFirst("aaa", "a", "b") == "baa");

    // Count
    assert(StringUtils::count("hello", 'l') == 2);
    assert(StringUtils::count("ababa", "ab") == 2);

    // Hex conversion
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    assert(StringUtils::toHex(data, 4, true) == "DEADBEEF");
    auto bytes = StringUtils::fromHex("DEADBEEF");
    assert(bytes.size() == 4 && bytes[0] == 0xDE);

    // Validation
    assert(StringUtils::isInteger("123"));
    assert(StringUtils::isInteger("-456"));
    assert(!StringUtils::isInteger("12.3"));
    assert(StringUtils::isFloat("3.14"));
    assert(StringUtils::isFloat("1e10"));
    assert(StringUtils::isBlank("   "));
    assert(!StringUtils::isBlank("  a  "));

    // Format
    assert(StringUtils::format("Hello %s, %d", "World", 42) == "Hello World, 42");

    // Parse
    assert(StringUtils::parse<int>("42") == 42);
    assert(StringUtils::parse<double>("3.14") == 3.14);

    // Edge cases for split
    auto emptySplit = StringUtils::split("", ',');
    assert(emptySplit.empty());

    auto singleChar = StringUtils::split("a", ',');
    assert(singleChar.size() == 1 && singleChar[0] == "a");

    auto onlyDelimiter = StringUtils::split(",", ',');
    assert(onlyDelimiter.size() == 2 && onlyDelimiter[0].empty() && onlyDelimiter[1].empty());

    auto endsWithDelimiter = StringUtils::split("a,", ',');
    assert(endsWithDelimiter.size() == 2 && endsWithDelimiter[0] == "a" && endsWithDelimiter[1].empty());

    auto startsWithDelimiter = StringUtils::split(",a", ',');
    assert(startsWithDelimiter.size() == 2 && startsWithDelimiter[0].empty() && startsWithDelimiter[1] == "a");

    auto multipleDelimiters = StringUtils::split("a,,b", ',');
    assert(multipleDelimiters.size() == 3 && multipleDelimiters[0] == "a" && multipleDelimiters[1].empty() && multipleDelimiters[2] == "b");

    // Edge cases for string operations
    assert(StringUtils::trim("").empty());
    assert(StringUtils::toLower("").empty());
    assert(StringUtils::toUpper("").empty());
    assert(StringUtils::replace("", "a", "b").empty());
    assert(StringUtils::replaceFirst("", "a", "b").empty());
    assert(StringUtils::count("", 'a') == 0);

    // Edge cases for hex conversion
    assert(StringUtils::toHex(nullptr, 0).empty());
    assert(StringUtils::fromHex("").empty());
    assert(StringUtils::fromHex("invalid").empty());

    // Edge cases for validation
    assert(!StringUtils::isInteger(""));
    assert(!StringUtils::isFloat(""));
    assert(StringUtils::isBlank(""));
    assert(StringUtils::isBlank("   "));
    assert(StringUtils::isBlank("\t\n"));

    std::cout << "String tests passed!" << std::endl;
}

// ==================== Random Tests ====================
void testRandom() {
    std::cout << "=== Testing Random ===" << std::endl;

    auto& rng = Randomizer::instance();

    // Integer range
    for (int i = 0; i < 100; ++i) {
        int val = rng.randomInt(10, 20);
        assert(val >= 10 && val <= 20);
    }

    // Double range
    for (int i = 0; i < 100; ++i) {
        double val = rng.randomDouble(0.0, 1.0);
        assert(val >= 0.0 && val < 1.0);
    }

    // Random string
    std::string str = rng.randomString(10);
    assert(str.length() == 10);

    // Random hex
    std::string hex = rng.randomHex(8);
    assert(hex.length() == 8);

    // UUID
    std::string uuid = rng.uuid();
    assert(uuid.length() == 36);
    assert(uuid[8] == '-' && uuid[13] == '-' && uuid[18] == '-' && uuid[23] == '-');
    assert(uuid[14] == '4'); // Version 4

    // Random bytes
    uint8_t buffer[16];
    rng.randomBytes(buffer, 16);

    // Edge cases
    // Same min/max should return min
    assert(rng.randomInt(5, 5) == 5);
    assert(rng.randomUint32(10, 10) == 10);
    assert(rng.randomUint64(20, 20) == 20);
    assert(rng.randomDouble(1.5, 1.5) == 1.5);
    assert(rng.randomFloat(2.5f, 2.5f) == 2.5f);

    // Empty/zero length strings
    assert(rng.randomString(0).empty());
    assert(rng.randomHex(0).empty());
    rng.randomBytes(nullptr, 0); // Should not crash

    // Empty charset
    assert(rng.randomString(5, "").empty());

    std::cout << "Random tests passed!" << std::endl;
}

// ==================== System Tests ====================
void testSystem() {
    std::cout << "=== Testing System ===" << std::endl;

    // Time
    int64_t ms = System::currentTimeMs();
    assert(ms > 0);

    std::string gmt = System::currentGMTTime();
    assert(!gmt.empty());

    std::string local = System::currentLocalTime();
    assert(!local.empty());
    std::cout << "  Current time: " << local << std::endl;

    // File operations
    std::string testFile = "/tmp/galay_test_file.txt";
    assert(System::writeFile(testFile, "Hello, World!"));
    assert(System::fileExists(testFile));

    auto content = System::readFile(testFile);
    assert(content.has_value());
    assert(*content == "Hello, World!");

    assert(System::fileSize(testFile) == 13);
    assert(System::remove(testFile));
    assert(!System::fileExists(testFile));

    // Directory
    std::string testDir = "/tmp/galay_test_dir";
    assert(System::createDirectory(testDir));
    assert(System::isDirectory(testDir));
    assert(System::remove(testDir));

    // Environment
    System::setEnv("GALAY_TEST_VAR", "test_value");
    assert(System::getEnv("GALAY_TEST_VAR") == "test_value");
    System::unsetEnv("GALAY_TEST_VAR");
    assert(System::getEnv("GALAY_TEST_VAR", "default") == "default");

    // System info
    assert(System::cpuCount() > 0);
    assert(!System::hostname().empty());
    assert(!System::currentDir().empty());

    std::cout << "  CPU count: " << System::cpuCount() << std::endl;
    std::cout << "  Hostname: " << System::hostname() << std::endl;

    // Edge cases for file operations
    // Reading non-existent file
    auto nonExistent = System::readFile("/tmp/non_existent_file.txt");
    assert(!nonExistent.has_value());

    // Writing empty content
    assert(System::writeFile("/tmp/empty_file.txt", ""));
    assert(System::fileExists("/tmp/empty_file.txt"));
    assert(System::fileSize("/tmp/empty_file.txt") == 0);
    System::remove("/tmp/empty_file.txt");

    // Environment variables with empty values
    System::setEnv("GALAY_EMPTY_VAR", "");
    assert(System::getEnv("GALAY_EMPTY_VAR") == "");
    System::unsetEnv("GALAY_EMPTY_VAR");

    std::cout << "System tests passed!" << std::endl;
}

// ==================== BackTrace Tests ====================
void testBackTrace() {
    std::cout << "=== Testing BackTrace ===" << std::endl;

    auto frames = BackTrace::getStackTrace(10, 0);
    assert(!frames.empty());

    std::string traceStr = BackTrace::getStackTraceString(5, 0);
    assert(!traceStr.empty());

    std::cout << "  Got " << frames.size() << " stack frames" << std::endl;
    std::cout << "BackTrace tests passed!" << std::endl;
}

// ==================== SignalHandler Tests ====================
void testSignalHandler() {
    std::cout << "=== Testing SignalHandler ===" << std::endl;

    auto& handler = SignalHandler::instance();

    bool signalReceived = false;
    handler.setHandler(SIGUSR1, [&signalReceived](int) {
        signalReceived = true;
    });

    assert(handler.hasHandler(SIGUSR1));

    // Send signal to self
    raise(SIGUSR1);
    assert(signalReceived);

    handler.removeHandler(SIGUSR1);
    assert(!handler.hasHandler(SIGUSR1));

    std::cout << "SignalHandler tests passed!" << std::endl;
}

// ==================== Pool Tests ====================
void testPool() {
    std::cout << "=== Testing Pool ===" << std::endl;

    // Basic object pool
    ObjectPool<std::string> pool(5, 10);
    assert(pool.size() == 5);

    {
        auto obj1 = pool.acquire();
        *obj1 = "test";
        assert(pool.size() == 4);

        auto obj2 = pool.acquire();
        assert(pool.size() == 3);
    }
    // Objects returned to pool
    assert(pool.size() == 5);

    // Blocking pool
    BlockingObjectPool<int> blockingPool(3);
    assert(blockingPool.available() == 3);

    {
        auto obj = blockingPool.acquire();
        assert(blockingPool.available() == 2);
    }
    assert(blockingPool.available() == 3);

    std::cout << "Pool tests passed!" << std::endl;
}

// ==================== Thread Tests ====================
void testThread() {
    std::cout << "=== Testing Thread ===" << std::endl;

    ThreadPool pool(4);
    assert(pool.threadCount() == 4);

    // Add tasks with futures
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.addTask([i]() {
            return i * i;
        }));
    }

    // Check results
    for (int i = 0; i < 10; ++i) {
        assert(futures[i].get() == i * i);
    }

    // Task waiter
    TaskWaiter waiter;
    std::atomic<int> counter{0};

    for (int i = 0; i < 5; ++i) {
        waiter.addTask(pool, [&counter]() {
            ++counter;
        });
    }

    waiter.wait();
    assert(counter == 5);

    // Thread-safe list
    ThreadSafeList<int> list;
    list.pushBack(1);
    list.pushBack(2);
    list.pushFront(0);

    assert(list.size() == 3);
    assert(list.popFront().value() == 0);
    assert(list.popBack().value() == 2);
    assert(list.size() == 1);

    // Edge cases for thread pool
    // Zero thread pool
    ThreadPool zeroPool(0);
    assert(zeroPool.threadCount() > 0); // Should default to hardware concurrency

    // Empty task list
    TaskWaiter emptyWaiter;
    emptyWaiter.wait(); // Should not hang

    // Thread-safe list edge cases
    ThreadSafeList<int> emptyList;
    assert(emptyList.size() == 0);
    assert(!emptyList.popFront().has_value());
    assert(!emptyList.popBack().has_value());

    std::cout << "Thread tests passed!" << std::endl;
}

// ==================== RateLimiter Tests ====================
void testRateLimiter() {
    std::cout << "=== Testing RateLimiter ===" << std::endl;

    // Counting semaphore - 使用 tryAcquire 测试基本功能
    CountingSemaphore sem(3);
    assert(sem.available() == 3);

    assert(sem.tryAcquire(2));
    assert(sem.available() == 1);

    sem.release(2);
    assert(sem.available() == 3);

    // Token bucket
    TokenBucketLimiter tokenBucket(100, 10); // 100 tokens/sec, capacity 10

    // Should be able to acquire immediately (bucket starts full)
    assert(tokenBucket.tryAcquire(5));
    assert(tokenBucket.availableTokens() >= 4); // At least 5 consumed

    // Sliding window
    SlidingWindowLimiter slidingWindow(5, std::chrono::milliseconds(100));

    for (int i = 0; i < 5; ++i) {
        assert(slidingWindow.tryAcquire());
    }
    assert(!slidingWindow.tryAcquire()); // Should be rate limited

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    assert(slidingWindow.tryAcquire()); // Should work after window expires

    // 协程环境测试
    galay::kernel::Runtime runtime(galay::kernel::LoadBalanceStrategy::ROUND_ROBIN, 2, 0);
    runtime.start();

    CountingSemaphore asyncSem(2);
    std::atomic<bool> testDone{false};

    // 测试 Semaphore 的协程 acquire
    runtime.getNextIOScheduler()->spawn([&]() -> galay::kernel::Coroutine {
        // 异步获取
        co_await asyncSem.acquire(1);
        assert(asyncSem.available() == 1);

        // 带超时的获取
        auto result = co_await asyncSem.acquire(1).timeout(std::chrono::milliseconds(100));
        assert(result.has_value());
        assert(asyncSem.available() == 0);

        asyncSem.release(2);
        assert(asyncSem.available() == 2);

        testDone = true;
        co_return;
    }());

    // 等待协程完成
    while (!testDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    runtime.stop();

    std::cout << "RateLimiter tests passed!" << std::endl;
}

// ==================== CircuitBreaker Tests ====================
void testCircuitBreaker() {
    std::cout << "=== Testing CircuitBreaker ===" << std::endl;

    CircuitBreakerConfig config;
    config.failureThreshold = 3;
    config.successThreshold = 2;
    config.resetTimeout = std::chrono::seconds(1);

    CircuitBreaker cb(config);

    assert(cb.state() == CircuitState::Closed);
    assert(cb.allowRequest());

    // Simulate failures
    cb.onFailure();
    cb.onFailure();
    assert(cb.state() == CircuitState::Closed);

    cb.onFailure(); // Third failure
    assert(cb.state() == CircuitState::Open);
    assert(!cb.allowRequest());

    // Wait for reset timeout
    std::this_thread::sleep_for(std::chrono::seconds(1));
    assert(cb.allowRequest()); // Should transition to half-open
    assert(cb.state() == CircuitState::HalfOpen);

    // Success in half-open
    cb.onSuccess();
    cb.onSuccess();
    assert(cb.state() == CircuitState::Closed);

    std::cout << "CircuitBreaker tests passed!" << std::endl;
}

// ==================== ConsistentHash Tests ====================
void testConsistentHash() {
    std::cout << "=== Testing ConsistentHash ===" << std::endl;

    ConsistentHash hash(100);

    hash.addNode({"node1", "192.168.1.1:8080", 1});
    hash.addNode({"node2", "192.168.1.2:8080", 1});
    hash.addNode({"node3", "192.168.1.3:8080", 2}); // Weight 2

    assert(hash.nodeCount() == 3);
    assert(hash.virtualNodeCount() == 400); // 100 + 100 + 200

    // Get node for key
    auto node = hash.getNode("test_key");
    assert(node.has_value());

    // Same key should always return same node
    auto node2 = hash.getNode("test_key");
    assert(node2.has_value());
    assert(node->id == node2->id);

    // Get multiple nodes for replication
    auto nodes = hash.getNodes("test_key", 2);
    assert(nodes.size() == 2);
    assert(nodes[0].id != nodes[1].id);

    // Remove node
    hash.removeNode("node1");
    assert(hash.nodeCount() == 2);

    std::cout << "ConsistentHash tests passed!" << std::endl;
}

// ==================== TrieTree Tests ====================
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
void testParser() {
    std::cout << "=== Testing Parser ===" << std::endl;

    // Config parser
    ConfigParser config;
    std::string configContent = R"(
# Comment
[database]
host = localhost
port = 5432
name = "test_db"

[server]
port = 8080
debug = true
)";

    assert(config.parseString(configContent));

    assert(config.getValue("database.host").value() == "localhost");
    assert(config.getValueAs<int>("database.port", 0) == 5432);
    assert(config.getValue("database.name").value() == "test_db");
    assert(config.getValueAs<int>("server.port", 0) == 8080);

    auto dbKeys = config.getKeysInSection("database");
    assert(dbKeys.size() == 3);

    // Env parser
    EnvParser env;
    std::string envContent = R"(
# Environment variables
DATABASE_URL=postgres://localhost/db
export API_KEY=secret123
DEBUG=true
)";

    assert(env.parseString(envContent));
    assert(env.getValue("DATABASE_URL").value() == "postgres://localhost/db");
    assert(env.getValue("API_KEY").value() == "secret123");
    assert(env.getValue("DEBUG").value() == "true");

    std::cout << "Parser tests passed!" << std::endl;
}

// ==================== App (Args) Tests ====================
void testApp() {
    std::cout << "=== Testing App ===" << std::endl;

    App app("test-app", "Test application");

    app.addArg(Arg("name", "User name").shortName('n').required());
    app.addArg(Arg("count", "Count").shortName('c').type(ArgType::Int).defaultValue(1));
    app.addArg(Arg("verbose", "Verbose mode").shortName('v').flag());

    bool callbackCalled = false;
    app.callback([&callbackCalled](galay::utils::Cmd& cmd) {
        callbackCalled = true;
        assert(cmd.getAs<std::string>("name") == "John");
        assert(cmd.getAs<int>("count") == 5);
        assert(cmd.getAs<bool>("verbose") == true);
        return 0;
    });

    char* argv[] = {const_cast<char*>("test-app"),
                   const_cast<char*>("--name"),
                   const_cast<char*>("John"),
                   const_cast<char*>("-c"),
                   const_cast<char*>("5"),
                   const_cast<char*>("-v")};
    int result = app.run(6, argv);

    assert(result == 0);
    assert(callbackCalled);

    std::cout << "App tests passed!" << std::endl;
}

// ==================== Process Tests ====================
void testProcess() {
    std::cout << "=== Testing Process ===" << std::endl;

    // Current process info
    ProcessId pid = Process::currentId();
    assert(pid > 0);

    ProcessId ppid = Process::parentId();
    assert(ppid > 0);

    std::cout << "  Current PID: " << pid << std::endl;
    std::cout << "  Parent PID: " << ppid << std::endl;

    // Execute command
    auto [status, output] = Process::executeWithOutput("echo hello");
    assert(status.success());
    assert(output.find("hello") != std::string::npos);

    // Check if process is running
    assert(Process::isRunning(pid));

    std::cout << "Process tests passed!" << std::endl;
}

// ==================== TypeName Tests ====================
void testTypeName() {
    std::cout << "=== Testing TypeName ===" << std::endl;

    std::string intName = getTypeName<int>();
    assert(intName == "int");

    std::string strName = getTypeName<std::string>();
    assert(strName.find("string") != std::string::npos);

    std::vector<int> vec;
    std::string vecName = getTypeName(vec);
    assert(vecName.find("vector") != std::string::npos);

    std::cout << "  int type name: " << intName << std::endl;
    std::cout << "  string type name: " << strName << std::endl;

    std::cout << "TypeName tests passed!" << std::endl;
}

// ==================== Base64 Tests ====================
void testBase64() {
    std::cout << "=== Testing Base64 ===" << std::endl;

    // Basic encoding
    std::string input = "Hello, World!";
    std::string encoded = Base64Util::Base64Encode(input);
    assert(!encoded.empty());
    std::cout << "  Encoded: " << encoded << std::endl;

    // Basic decoding
    std::string decoded = Base64Util::Base64Decode(encoded);
    assert(decoded == input);
    std::cout << "  Decoded: " << decoded << std::endl;

    // Test with binary data
    unsigned char binaryData[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
    std::string binaryEncoded = Base64Util::Base64Encode(binaryData, sizeof(binaryData));
    assert(!binaryEncoded.empty());
    std::string binaryDecoded = Base64Util::Base64Decode(binaryEncoded);
    assert(binaryDecoded.size() == sizeof(binaryData));
    for (size_t i = 0; i < sizeof(binaryData); ++i) {
        assert(static_cast<unsigned char>(binaryDecoded[i]) == binaryData[i]);
    }

    // Test URL-safe encoding
    std::string urlEncoded = Base64Util::Base64Encode(input, true);
    assert(!urlEncoded.empty());
    std::string urlDecoded = Base64Util::Base64Decode(urlEncoded);
    assert(urlDecoded == input);

    // Test PEM format (64 chars per line)
    std::string longInput = "This is a longer string that will be encoded in PEM format with line breaks every 64 characters to test the line breaking functionality.";
    std::string pemEncoded = Base64Util::Base64EncodePem(longInput);
    assert(pemEncoded.find('\n') != std::string::npos); // Should contain line breaks
    std::string pemDecoded = Base64Util::Base64Decode(pemEncoded, true); // Remove line breaks
    assert(pemDecoded == longInput);

    // Test MIME format (76 chars per line)
    std::string mimeEncoded = Base64Util::Base64EncodeMime(longInput);
    assert(mimeEncoded.find('\n') != std::string::npos); // Should contain line breaks
    std::string mimeDecoded = Base64Util::Base64Decode(mimeEncoded, true);
    assert(mimeDecoded == longInput);

    // Edge cases
    // Empty string
    assert(Base64Util::Base64Encode("").empty());
    assert(Base64Util::Base64Decode("").empty());

    // Single character
    std::string singleChar = "A";
    std::string singleEncoded = Base64Util::Base64Encode(singleChar);
    assert(Base64Util::Base64Decode(singleEncoded) == singleChar);

    // Two characters
    std::string twoChars = "AB";
    std::string twoEncoded = Base64Util::Base64Encode(twoChars);
    assert(Base64Util::Base64Decode(twoEncoded) == twoChars);

    // Three characters (no padding needed)
    std::string threeChars = "ABC";
    std::string threeEncoded = Base64Util::Base64Encode(threeChars);
    assert(Base64Util::Base64Decode(threeEncoded) == threeChars);

    // Test all ASCII characters
    std::string allAscii;
    for (int i = 0; i < 128; ++i) {
        allAscii += static_cast<char>(i);
    }
    std::string asciiEncoded = Base64Util::Base64Encode(allAscii);
    std::string asciiDecoded = Base64Util::Base64Decode(asciiEncoded);
    assert(asciiDecoded == allAscii);

    // Test with special characters
    std::string special = "!@#$%^&*()_+-=[]{}|;':\",./<>?`~";
    std::string specialEncoded = Base64Util::Base64Encode(special);
    std::string specialDecoded = Base64Util::Base64Decode(specialEncoded);
    assert(specialDecoded == special);

    // Test with UTF-8 characters
    std::string utf8 = "Hello 世界 🌍";
    std::string utf8Encoded = Base64Util::Base64Encode(utf8);
    std::string utf8Decoded = Base64Util::Base64Decode(utf8Encoded);
    assert(utf8Decoded == utf8);

    // Test standard vs URL-safe encoding differences
    std::string testData = "\xFB\xFF"; // Will produce '+' and '/' in standard encoding
    std::string standardEncoded = Base64Util::Base64Encode(testData, false);
    std::string urlSafeEncoded = Base64Util::Base64Encode(testData, true);
    // Both should decode to the same value
    assert(Base64Util::Base64Decode(standardEncoded) == testData);
    assert(Base64Util::Base64Decode(urlSafeEncoded) == testData);

    // Test decoding with line breaks
    std::string encodedWithBreaks = "SGVsbG8s\nIFdvcmxk\nIQ==";
    std::string decodedWithBreaks = Base64Util::Base64Decode(encodedWithBreaks, true);
    assert(decodedWithBreaks == "Hello, World!");

    // Test invalid input handling
    bool exceptionThrown = false;
    try {
        Base64Util::Base64Decode("Invalid@#$%");
    } catch (const std::runtime_error&) {
        exceptionThrown = true;
    }
    assert(exceptionThrown);

    // Performance test with large data
    std::string largeData(10000, 'X');
    auto startEncode = std::chrono::high_resolution_clock::now();
    std::string largeEncoded = Base64Util::Base64Encode(largeData);
    auto endEncode = std::chrono::high_resolution_clock::now();
    auto encodeTime = std::chrono::duration_cast<std::chrono::microseconds>(endEncode - startEncode).count();

    auto startDecode = std::chrono::high_resolution_clock::now();
    std::string largeDecoded = Base64Util::Base64Decode(largeEncoded);
    auto endDecode = std::chrono::high_resolution_clock::now();
    auto decodeTime = std::chrono::duration_cast<std::chrono::microseconds>(endDecode - startDecode).count();

    assert(largeDecoded == largeData);
    std::cout << "  Performance (10KB): Encode=" << encodeTime << "μs, Decode=" << decodeTime << "μs" << std::endl;

#if __cplusplus >= 201703L
    // Test string_view overloads (C++17+)
    std::cout << "  Testing string_view interfaces..." << std::endl;

    std::string_view inputView = "Test string_view";
    std::string viewEncoded = Base64Util::Base64EncodeView(inputView);
    std::string viewDecoded = Base64Util::Base64DecodeView(viewEncoded);
    assert(viewDecoded == inputView);

    // Test URL-safe with string_view
    std::string viewUrlEncoded = Base64Util::Base64EncodeView(inputView, true);
    std::string viewUrlDecoded = Base64Util::Base64DecodeView(viewUrlEncoded);
    assert(viewUrlDecoded == inputView);

    // Test PEM with string_view
    std::string_view viewForPem = "Test PEM with string_view that is long enough to trigger line breaks in the output";
    std::string pemViewEncoded = Base64Util::Base64EncodePemView(viewForPem);
    assert(pemViewEncoded.find('\n') != std::string::npos);
    std::string pemViewDecoded = Base64Util::Base64DecodeView(pemViewEncoded, true);
    assert(pemViewDecoded == viewForPem);

    // Test MIME with string_view
    std::string_view viewForMime = "Test MIME with string_view that is long enough to trigger line breaks in the output";
    std::string mimeViewEncoded = Base64Util::Base64EncodeMimeView(viewForMime);
    assert(mimeViewEncoded.find('\n') != std::string::npos);
    std::string mimeViewDecoded = Base64Util::Base64DecodeView(mimeViewEncoded, true);
    assert(mimeViewDecoded == viewForMime);

    // Test with substring view
    std::string fullString = "Hello, World! This is a test.";
    std::string_view subView(fullString.data() + 7, 5); // "World"
    std::string subEncoded = Base64Util::Base64EncodeView(subView);
    std::string subDecoded = Base64Util::Base64DecodeView(subEncoded);
    assert(subDecoded == "World");

    std::cout << "  string_view tests passed!" << std::endl;
#endif

    std::cout << "Base64 tests passed!" << std::endl;
}

// ==================== MD5 Tests ====================
void testMD5() {
    std::cout << "=== Testing MD5 ===" << std::endl;

    // Test empty string
    std::string emptyHash = MD5Util::MD5("");
    assert(emptyHash == "d41d8cd98f00b204e9800998ecf8427e");
    std::cout << "  Empty string: " << emptyHash << std::endl;

    // Test simple string
    std::string simpleHash = MD5Util::MD5("hello");
    assert(simpleHash == "5d41402abc4b2a76b9719d911017c592");
    std::cout << "  'hello': " << simpleHash << std::endl;

    // Test standard test vectors
    assert(MD5Util::MD5("a") == "0cc175b9c0f1b6a831c399e269772661");
    assert(MD5Util::MD5("abc") == "900150983cd24fb0d6963f7d28e17f72");
    assert(MD5Util::MD5("message digest") == "f96b697d7cb7938d525a2f31aaf161d0");
    assert(MD5Util::MD5("abcdefghijklmnopqrstuvwxyz") == "c3fcd3d76192e4007dfb496cca67e13b");
    assert(MD5Util::MD5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") == "d174ab98d277d9f5a5611c2c9f419d9f");
    assert(MD5Util::MD5("12345678901234567890123456789012345678901234567890123456789012345678901234567890") == "57edf4a22be3c955ac49da2e2107b67a");

    // Test with binary data
    unsigned char binaryData[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
    std::string binaryHash = MD5Util::MD5(binaryData, sizeof(binaryData));
    assert(!binaryHash.empty());
    assert(binaryHash.length() == 32); // MD5 is 128 bits = 32 hex chars
    std::cout << "  Binary data: " << binaryHash << std::endl;

    // Test MD5Raw (returns raw bytes)
    auto rawDigest = MD5Util::MD5Raw("hello");
    assert(rawDigest.size() == 16); // MD5 is 128 bits = 16 bytes

    // Verify raw digest matches hex string
    std::string hexFromRaw;
    for (auto byte : rawDigest) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", byte);
        hexFromRaw += buf;
    }
    assert(hexFromRaw == simpleHash);

    // Test with special characters
    std::string specialHash = MD5Util::MD5("!@#$%^&*()_+-=[]{}|;':\",./<>?`~");
    assert(!specialHash.empty());
    assert(specialHash.length() == 32);

    // Test with UTF-8
    std::string utf8Hash = MD5Util::MD5("Hello 世界 🌍");
    assert(!utf8Hash.empty());
    assert(utf8Hash.length() == 32);
    std::cout << "  UTF-8: " << utf8Hash << std::endl;

    // Test with long string
    std::string longString(10000, 'X');
    auto startHash = std::chrono::high_resolution_clock::now();
    std::string longHash = MD5Util::MD5(longString);
    auto endHash = std::chrono::high_resolution_clock::now();
    auto hashTime = std::chrono::duration_cast<std::chrono::microseconds>(endHash - startHash).count();

    assert(!longHash.empty());
    assert(longHash.length() == 32);
    std::cout << "  Performance (10KB): " << hashTime << "μs" << std::endl;

    // Test consistency - same input should always produce same hash
    for (int i = 0; i < 10; ++i) {
        assert(MD5Util::MD5("test") == "098f6bcd4621d373cade4e832627b4f6");
    }

    // Test different inputs produce different hashes
    std::string hash1 = MD5Util::MD5("test1");
    std::string hash2 = MD5Util::MD5("test2");
    assert(hash1 != hash2);

    // Test case sensitivity
    std::string lowerHash = MD5Util::MD5("hello");
    std::string upperHash = MD5Util::MD5("HELLO");
    assert(lowerHash != upperHash);

#if __cplusplus >= 201703L
    // Test string_view version
    std::cout << "  Testing string_view interface..." << std::endl;

    std::string_view view = "test string_view";
    std::string viewHash = MD5Util::MD5View(view);
    assert(!viewHash.empty());
    assert(viewHash.length() == 32);

    // Verify string_view produces same result as string
    std::string str = "test string_view";
    assert(MD5Util::MD5View(view) == MD5Util::MD5(str));

    // Test with substring view
    std::string fullString = "Hello, World! This is a test.";
    std::string_view subView(fullString.data() + 7, 5); // "World"
    std::string subHash = MD5Util::MD5View(subView);
    assert(subHash == MD5Util::MD5("World"));

    // Test MD5RawView
    auto rawViewDigest = MD5Util::MD5RawView(view);
    assert(rawViewDigest.size() == 16);

    std::cout << "  string_view tests passed!" << std::endl;
#endif

    // Test known MD5 collisions awareness (informational)
    // Note: MD5 is cryptographically broken, but still useful for checksums
    std::cout << "  Note: MD5 is not cryptographically secure" << std::endl;

    std::cout << "MD5 tests passed!" << std::endl;
}

// ==================== MurmurHash3 Tests ====================
void testMurmurHash3() {
    std::cout << "=== Testing MurmurHash3 ===" << std::endl;

    // Test 32-bit hash with empty string
    uint32_t emptyHash32 = MurmurHash3Util::Hash32("", 0);
    std::cout << "  Empty string (32-bit): 0x" << std::hex << emptyHash32 << std::dec << std::endl;
    assert(emptyHash32 == 0); // Empty string with seed 0 should give 0

    // Test 32-bit hash with simple strings
    uint32_t hash32_hello = MurmurHash3Util::Hash32("hello");
    std::cout << "  'hello' (32-bit): 0x" << std::hex << hash32_hello << std::dec << std::endl;
    assert(hash32_hello != 0);

    // Test with different seeds produce different hashes
    uint32_t hash_seed0 = MurmurHash3Util::Hash32("test", 0);
    uint32_t hash_seed1 = MurmurHash3Util::Hash32("test", 1);
    uint32_t hash_seed42 = MurmurHash3Util::Hash32("test", 42);
    assert(hash_seed0 != hash_seed1);
    assert(hash_seed0 != hash_seed42);
    assert(hash_seed1 != hash_seed42);

    // Test 128-bit hash
    std::string hash128_empty = MurmurHash3Util::Hash128("");
    assert(hash128_empty.length() == 32); // 128 bits = 32 hex chars
    std::cout << "  Empty string (128-bit): " << hash128_empty << std::endl;

    std::string hash128_hello = MurmurHash3Util::Hash128("hello");
    assert(hash128_hello.length() == 32);
    std::cout << "  'hello' (128-bit): " << hash128_hello << std::endl;

    // Test 128-bit raw output
    auto raw128 = MurmurHash3Util::Hash128Raw("test");
    assert(raw128.size() == 2); // Two 64-bit values
    std::cout << "  'test' (128-bit raw): [0x" << std::hex << raw128[0]
              << ", 0x" << raw128[1] << "]" << std::dec << std::endl;

    // Test consistency - same input should always produce same hash
    for (int i = 0; i < 10; ++i) {
        assert(MurmurHash3Util::Hash32("consistent") == MurmurHash3Util::Hash32("consistent"));
        assert(MurmurHash3Util::Hash128("consistent") == MurmurHash3Util::Hash128("consistent"));
    }

    // Test different inputs produce different hashes
    uint32_t hash1 = MurmurHash3Util::Hash32("test1");
    uint32_t hash2 = MurmurHash3Util::Hash32("test2");
    assert(hash1 != hash2);

    std::string hash128_1 = MurmurHash3Util::Hash128("test1");
    std::string hash128_2 = MurmurHash3Util::Hash128("test2");
    assert(hash128_1 != hash128_2);

    // Test case sensitivity
    uint32_t lowerHash = MurmurHash3Util::Hash32("hello");
    uint32_t upperHash = MurmurHash3Util::Hash32("HELLO");
    assert(lowerHash != upperHash);

    // Test with binary data
    unsigned char binaryData[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
    uint32_t binaryHash32 = MurmurHash3Util::Hash32(binaryData, sizeof(binaryData));
    assert(binaryHash32 != 0);

    std::string binaryHash128 = MurmurHash3Util::Hash128(binaryData, sizeof(binaryData));
    assert(binaryHash128.length() == 32);

    // Test with various lengths to cover all tail cases
    for (size_t len = 0; len <= 20; ++len) {
        std::string testStr(len, 'X');
        uint32_t h32 = MurmurHash3Util::Hash32(testStr);
        std::string h128 = MurmurHash3Util::Hash128(testStr);
        assert(h128.length() == 32);
        // Just verify it doesn't crash and produces output
    }

    // Test with special characters
    uint32_t specialHash = MurmurHash3Util::Hash32("!@#$%^&*()_+-=[]{}|;':\",./<>?`~");
    assert(specialHash != 0);

    // Test with UTF-8
    uint32_t utf8Hash = MurmurHash3Util::Hash32("Hello 世界 🌍");
    assert(utf8Hash != 0);
    std::cout << "  UTF-8 (32-bit): 0x" << std::hex << utf8Hash << std::dec << std::endl;

    // Performance test with large data
    std::string largeData(10000, 'X');

    auto start32 = std::chrono::high_resolution_clock::now();
    uint32_t largeHash32 = MurmurHash3Util::Hash32(largeData);
    auto end32 = std::chrono::high_resolution_clock::now();
    auto time32 = std::chrono::duration_cast<std::chrono::microseconds>(end32 - start32).count();

    auto start128 = std::chrono::high_resolution_clock::now();
    std::string largeHash128 = MurmurHash3Util::Hash128(largeData);
    auto end128 = std::chrono::high_resolution_clock::now();
    auto time128 = std::chrono::duration_cast<std::chrono::microseconds>(end128 - start128).count();

    assert(largeHash32 != 0);
    assert(largeHash128.length() == 32);
    std::cout << "  Performance (10KB): Hash32=" << time32 << "μs, Hash128=" << time128 << "μs" << std::endl;

    // Test avalanche effect - small change in input should cause large change in output
    uint32_t hashA = MurmurHash3Util::Hash32("test");
    uint32_t hashB = MurmurHash3Util::Hash32("tess"); // Changed one character

    // Count different bits
    uint32_t diff = hashA ^ hashB;
    int bitsDifferent = 0;
    for (int i = 0; i < 32; ++i) {
        if (diff & (1U << i)) bitsDifferent++;
    }

    // Good hash should have roughly 50% bits different (avalanche effect)
    assert(bitsDifferent > 10); // At least some bits should differ
    std::cout << "  Avalanche effect: " << bitsDifferent << "/32 bits differ" << std::endl;

#if __cplusplus >= 201703L
    // Test string_view versions
    std::cout << "  Testing string_view interfaces..." << std::endl;

    std::string_view view = "test string_view";
    uint32_t viewHash32 = MurmurHash3Util::Hash32View(view);
    std::string viewHash128 = MurmurHash3Util::Hash128View(view);
    auto viewHash128Raw = MurmurHash3Util::Hash128RawView(view);

    assert(viewHash32 != 0);
    assert(viewHash128.length() == 32);
    assert(viewHash128Raw.size() == 2);

    // Verify string_view produces same result as string
    std::string str = "test string_view";
    assert(MurmurHash3Util::Hash32View(view) == MurmurHash3Util::Hash32(str));
    assert(MurmurHash3Util::Hash128View(view) == MurmurHash3Util::Hash128(str));

    // Test with substring view
    std::string fullString = "Hello, World! This is a test.";
    std::string_view subView(fullString.data() + 7, 5); // "World"
    uint32_t subHash = MurmurHash3Util::Hash32View(subView);
    assert(subHash == MurmurHash3Util::Hash32("World"));

    std::cout << "  string_view tests passed!" << std::endl;
#endif

    // Test distribution quality (basic check)
    std::cout << "  Testing hash distribution..." << std::endl;
    const int numBuckets = 100;
    std::vector<int> buckets(numBuckets, 0);

    for (int i = 0; i < 10000; ++i) {
        std::string key = "key" + std::to_string(i);
        uint32_t hash = MurmurHash3Util::Hash32(key);
        buckets[hash % numBuckets]++;
    }

    // Check that distribution is reasonably uniform
    // Each bucket should have roughly 100 items (10000/100)
    int minCount = *std::min_element(buckets.begin(), buckets.end());
    int maxCount = *std::max_element(buckets.begin(), buckets.end());

    // Allow some variance but not too much
    assert(minCount > 50);  // Not too few
    assert(maxCount < 150); // Not too many
    std::cout << "  Distribution: min=" << minCount << ", max=" << maxCount << " (expected ~100)" << std::endl;

    std::cout << "MurmurHash3 tests passed!" << std::endl;
}

// ==================== Salt Generator Tests ====================
void testSaltGenerator() {
    std::cout << "=== Testing Salt Generator ===" << std::endl;

    // Test hex salt generation
    std::string hexSalt = SaltGenerator::generateHex(16);
    assert(hexSalt.length() == 32); // 16 bytes = 32 hex chars
    assert(SaltGenerator::isValidHex(hexSalt));
    std::cout << "  Hex salt (16 bytes): " << hexSalt << std::endl;

    // Test base64 salt generation
    std::string base64Salt = SaltGenerator::generateBase64(16);
    assert(!base64Salt.empty());
    assert(SaltGenerator::isValidBase64(base64Salt));
    std::cout << "  Base64 salt (16 bytes): " << base64Salt << std::endl;

    // Test raw bytes generation
    auto bytesSalt = SaltGenerator::generateBytes(32);
    assert(bytesSalt.size() == 32);
    std::cout << "  Raw bytes salt: " << bytesSalt.size() << " bytes generated" << std::endl;

    // Test secure hex generation
    std::string secureHex = SaltGenerator::generateSecureHex(32);
    assert(secureHex.length() == 64); // 32 bytes = 64 hex chars
    assert(SaltGenerator::isValidHex(secureHex));
    std::cout << "  Secure hex salt (32 bytes): " << secureHex.substr(0, 32) << "..." << std::endl;

    // Test secure base64 generation
    std::string secureBase64 = SaltGenerator::generateSecureBase64(24);
    assert(!secureBase64.empty());
    assert(SaltGenerator::isValidBase64(secureBase64));
    std::cout << "  Secure base64 salt: " << secureBase64 << std::endl;

    // Test secure bytes generation
    auto secureBytes = SaltGenerator::generateSecureBytes(16);
    assert(secureBytes.size() == 16);

    // Test bcrypt salt generation
    std::string bcryptSalt = SaltGenerator::generateBcryptSalt();
    assert(bcryptSalt.length() == 22);
    std::cout << "  Bcrypt salt (22 chars): " << bcryptSalt << std::endl;

    // Test custom charset generation
    std::string customSalt = SaltGenerator::generateCustom(20, "0123456789");
    assert(customSalt.length() == 20);
    for (char c : customSalt) {
        assert(c >= '0' && c <= '9');
    }
    std::cout << "  Custom salt (digits only): " << customSalt << std::endl;

    // Test with alphanumeric charset
    std::string alphanumericSalt = SaltGenerator::generateCustom(16,
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    assert(alphanumericSalt.length() == 16);
    std::cout << "  Alphanumeric salt: " << alphanumericSalt << std::endl;

    // Test timestamped salt
    std::string timestampedSalt1 = SaltGenerator::generateTimestamped(32);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::string timestampedSalt2 = SaltGenerator::generateTimestamped(32);
    assert(timestampedSalt1 != timestampedSalt2); // Should be different due to timestamp
    assert(timestampedSalt1.length() == 32);
    std::cout << "  Timestamped salt: " << timestampedSalt1 << std::endl;

    // Test uniqueness - generate multiple salts and ensure they're different
    std::cout << "  Testing uniqueness..." << std::endl;
    std::set<std::string> uniqueSalts;
    for (int i = 0; i < 100; ++i) {
        uniqueSalts.insert(SaltGenerator::generateHex(16));
    }
    assert(uniqueSalts.size() == 100); // All should be unique

    // Test different lengths
    for (size_t len : {8, 16, 24, 32, 64}) {
        std::string salt = SaltGenerator::generateHex(len);
        assert(salt.length() == len * 2);
        assert(SaltGenerator::isValidHex(salt));
    }

    // Test validation functions
    assert(SaltGenerator::isValidHex("0123456789abcdef"));
    assert(SaltGenerator::isValidHex("ABCDEF0123456789"));
    assert(!SaltGenerator::isValidHex("xyz123"));
    assert(!SaltGenerator::isValidHex(""));

    assert(SaltGenerator::isValidBase64("SGVsbG8gV29ybGQ="));
    assert(SaltGenerator::isValidBase64("YWJjZGVmZ2hpams="));
    assert(!SaltGenerator::isValidBase64("Hello@World"));
    assert(!SaltGenerator::isValidBase64(""));

    // Test edge cases
    std::string emptySalt = SaltGenerator::generateHex(0);
    assert(emptySalt.empty());

    std::string emptyCustom = SaltGenerator::generateCustom(10, "");
    assert(emptyCustom.empty());

    std::string zeroLengthCustom = SaltGenerator::generateCustom(0, "abc");
    assert(zeroLengthCustom.empty());

    // Test randomness quality (basic statistical test)
    std::cout << "  Testing randomness distribution..." << std::endl;
    std::map<char, int> hexCharCounts;
    for (int i = 0; i < 1000; ++i) {
        std::string salt = SaltGenerator::generateHex(1);
        for (char c : salt) {
            hexCharCounts[c]++;
        }
    }

    // Each hex char (0-9, a-f) should appear roughly equally
    // With 2000 chars total (1000 salts * 2 chars), each should appear ~125 times
    for (const auto& [ch, count] : hexCharCounts) {
        assert(count > 50);  // Not too few
        assert(count < 200); // Not too many
    }

    // Performance test
    auto startPerf = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        SaltGenerator::generateHex(32);
    }
    auto endPerf = std::chrono::high_resolution_clock::now();
    auto perfTime = std::chrono::duration_cast<std::chrono::microseconds>(endPerf - startPerf).count();
    std::cout << "  Performance (1000 salts): " << perfTime << "μs ("
              << (perfTime / 1000.0) << "μs per salt)" << std::endl;

    // Test secure vs non-secure (both should work)
    std::string normalSalt = SaltGenerator::generateHex(16);
    std::string secureSalt = SaltGenerator::generateSecureHex(16);
    assert(normalSalt.length() == 32);
    assert(secureSalt.length() == 32);
    assert(normalSalt != secureSalt); // Should be different

    // Test bcrypt salt format
    for (int i = 0; i < 10; ++i) {
        std::string bcrypt = SaltGenerator::generateBcryptSalt();
        assert(bcrypt.length() == 22);
        // Verify it only contains bcrypt base64 chars
        for (char c : bcrypt) {
            bool valid = (c >= '0' && c <= '9') ||
                        (c >= 'A' && c <= 'Z') ||
                        (c >= 'a' && c <= 'z') ||
                        c == '.' || c == '/';
            assert(valid);
        }
    }

    std::cout << "Salt Generator tests passed!" << std::endl;
}

// ==================== LoadBalancer Tests ====================
void testLoadBalancer() {
    std::cout << "=== Testing LoadBalancer ===" << std::endl;

    std::vector<std::string> nodes = {"node1", "node2", "node3"};

    // Round Robin
    RoundRobinLoadBalancer<std::string> rrBalancer(nodes);
    assert(rrBalancer.size() == 3);

    auto selected1 = rrBalancer.select();
    auto selected2 = rrBalancer.select();
    auto selected3 = rrBalancer.select();
    auto selected4 = rrBalancer.select();

    assert(selected1.has_value() && selected2.has_value() && selected3.has_value() && selected4.has_value());
    assert(selected1.value() == "node1" && selected2.value() == "node2" && selected3.value() == "node3");
    assert(selected4.value() == "node1"); // Round robin

    rrBalancer.append("node4");
    assert(rrBalancer.size() == 4);

    // Weighted Round Robin
    std::vector<uint32_t> weights = {3, 2, 1};
    WeightRoundRobinLoadBalancer<std::string> wrrBalancer(nodes, weights);
    assert(wrrBalancer.size() == 3);

    // Test weighted selection (should prefer higher weight nodes)
    std::map<std::string, int> counts;
    for (int i = 0; i < 12; ++i) { // 3+2+1=6, test 2 rounds
        auto selected = wrrBalancer.select();
        assert(selected.has_value());
        counts[selected.value()]++;
    }

    // node1 (weight 3) should be selected more than node3 (weight 1)
    assert(counts["node1"] > counts["node3"]);

    // Random Load Balancer
    RandomLoadBalancer<std::string> randomBalancer(nodes);
    assert(randomBalancer.size() == 3);

    auto randomSelected = randomBalancer.select();
    assert(randomSelected.has_value());
    assert(std::find(nodes.begin(), nodes.end(), randomSelected.value()) != nodes.end());

    randomBalancer.append("node5");
    assert(randomBalancer.size() == 4);

    // Weighted Random Load Balancer
    WeightedRandomLoadBalancer<std::string> weightedRandomBalancer(nodes, weights);
    assert(weightedRandomBalancer.size() == 3);

    // Test weighted random selection
    counts.clear();
    for (int i = 0; i < 1000; ++i) {
        auto selected = weightedRandomBalancer.select();
        assert(selected.has_value());
        counts[selected.value()]++;
    }

    // node1 should be selected roughly 3 times more than node3
    assert(counts["node1"] > counts["node2"] && counts["node2"] > counts["node3"]);

    // Edge cases
    std::vector<std::string> emptyNodes;
    RoundRobinLoadBalancer<std::string> emptyRR(emptyNodes);
    assert(emptyRR.size() == 0);
    assert(!emptyRR.select().has_value());

    RandomLoadBalancer<std::string> emptyRandom(emptyNodes);
    assert(emptyRandom.size() == 0);
    assert(!emptyRandom.select().has_value());

    std::cout << "LoadBalancer tests passed!" << std::endl;
}

// ==================== Stress Tests ====================
void stressTestCircuitBreaker() {
    std::cout << "=== Stress Testing CircuitBreaker ===" << std::endl;

    CircuitBreakerConfig config;
    config.failureThreshold = 100;
    config.successThreshold = 50;
    config.resetTimeout = std::chrono::seconds(1);

    CircuitBreaker cb(config);

    const int numThreads = 8;
    const int opsPerThread = 100000;
    std::atomic<int> successOps{0};
    std::atomic<int> failureOps{0};
    std::atomic<int> allowedRequests{0};

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < opsPerThread; ++i) {
                if (cb.allowRequest()) {
                    ++allowedRequests;
                    if (i % 10 == 0) {
                        cb.onFailure();
                        ++failureOps;
                    } else {
                        cb.onSuccess();
                        ++successOps;
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    int totalOps = numThreads * opsPerThread;
    double opsPerSec = (totalOps * 1000.0) / duration;

    std::cout << "  Threads: " << numThreads << std::endl;
    std::cout << "  Total ops: " << totalOps << std::endl;
    std::cout << "  Duration: " << duration << "ms" << std::endl;
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0) << opsPerSec << " ops/sec" << std::endl;
    std::cout << "  Allowed requests: " << allowedRequests << std::endl;
    std::cout << "  Success ops: " << successOps << std::endl;
    std::cout << "  Failure ops: " << failureOps << std::endl;
    std::cout << "  Final state: " << cb.stateString() << std::endl;

    std::cout << "CircuitBreaker stress test passed!" << std::endl;
}

void stressTestRateLimiter() {
    std::cout << "=== Stress Testing RateLimiter ===" << std::endl;

    // 多调度器协程压测
    const int numSchedulers = 4;
    const int coroutinesPerScheduler = 1000;

    // Test CountingSemaphore
    {
        CountingSemaphore sem(100);
        std::atomic<int> acquired{0};
        std::atomic<int> completed{0};

        auto start = std::chrono::high_resolution_clock::now();

        galay::kernel::Runtime runtime(galay::kernel::LoadBalanceStrategy::ROUND_ROBIN, numSchedulers, 0);
        runtime.start();

        for (int c = 0; c < numSchedulers * coroutinesPerScheduler; ++c) {
            runtime.getNextIOScheduler()->spawn([&]() -> galay::kernel::Coroutine {
                for (int i = 0; i < 100; ++i) {
                    if (sem.tryAcquire(1)) {
                        ++acquired;
                        sem.release(1);
                    }
                }
                ++completed;
                co_return;
            }());
        }

        // 等待所有协程完成
        while (completed < numSchedulers * coroutinesPerScheduler) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        runtime.stop();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        int totalOps = acquired.load();
        double opsPerSec = duration > 0 ? (totalOps * 1000.0) / duration : 0;

        std::cout << "  [CountingSemaphore - Coroutine]" << std::endl;
        std::cout << "    Schedulers: " << numSchedulers << ", Coroutines: " << numSchedulers * coroutinesPerScheduler << std::endl;
        std::cout << "    Duration: " << duration << "ms" << std::endl;
        std::cout << "    Throughput: " << std::fixed << std::setprecision(0) << opsPerSec << " ops/sec" << std::endl;
        std::cout << "    Acquired: " << acquired << std::endl;
        assert(sem.available() == 100);
    }

    // Test TokenBucketLimiter
    {
        TokenBucketLimiter limiter(10000000, 100000); // 10M tokens/sec, capacity 100000
        std::atomic<int> acquired{0};
        std::atomic<int> completed{0};

        auto start = std::chrono::high_resolution_clock::now();

        galay::kernel::Runtime runtime(galay::kernel::LoadBalanceStrategy::ROUND_ROBIN, numSchedulers, 0);
        runtime.start();

        for (int c = 0; c < numSchedulers * coroutinesPerScheduler; ++c) {
            runtime.getNextIOScheduler()->spawn([&]() -> galay::kernel::Coroutine {
                for (int i = 0; i < 100; ++i) {
                    if (limiter.tryAcquire(1)) {
                        ++acquired;
                    }
                }
                ++completed;
                co_return;
            }());
        }

        while (completed < numSchedulers * coroutinesPerScheduler) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        runtime.stop();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        int totalOps = numSchedulers * coroutinesPerScheduler * 100;
        double opsPerSec = duration > 0 ? (totalOps * 1000.0) / duration : 0;

        std::cout << "  [TokenBucketLimiter - Coroutine]" << std::endl;
        std::cout << "    Schedulers: " << numSchedulers << ", Coroutines: " << numSchedulers * coroutinesPerScheduler << std::endl;
        std::cout << "    Duration: " << duration << "ms" << std::endl;
        std::cout << "    Throughput: " << std::fixed << std::setprecision(0) << opsPerSec << " ops/sec" << std::endl;
        std::cout << "    Acquired: " << acquired << std::endl;
    }

    // Test SlidingWindowLimiter
    {
        SlidingWindowLimiter limiter(1000000, std::chrono::milliseconds(1000));
        std::atomic<int> acquired{0};
        std::atomic<int> completed{0};

        auto start = std::chrono::high_resolution_clock::now();

        galay::kernel::Runtime runtime(galay::kernel::LoadBalanceStrategy::ROUND_ROBIN, numSchedulers, 0);
        runtime.start();

        for (int c = 0; c < numSchedulers * coroutinesPerScheduler; ++c) {
            runtime.getNextIOScheduler()->spawn([&]() -> galay::kernel::Coroutine {
                for (int i = 0; i < 50; ++i) {
                    if (limiter.tryAcquire()) {
                        ++acquired;
                    }
                }
                ++completed;
                co_return;
            }());
        }

        while (completed < numSchedulers * coroutinesPerScheduler) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        runtime.stop();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        int totalOps = numSchedulers * coroutinesPerScheduler * 50;
        double opsPerSec = duration > 0 ? (totalOps * 1000.0) / duration : 0;

        std::cout << "  [SlidingWindowLimiter - Coroutine]" << std::endl;
        std::cout << "    Schedulers: " << numSchedulers << ", Coroutines: " << numSchedulers * coroutinesPerScheduler << std::endl;
        std::cout << "    Duration: " << duration << "ms" << std::endl;
        std::cout << "    Throughput: " << std::fixed << std::setprecision(0) << opsPerSec << " ops/sec" << std::endl;
        std::cout << "    Acquired: " << acquired << std::endl;
    }

    std::cout << "RateLimiter stress test passed!" << std::endl;
}

void stressTestPool() {
    std::cout << "=== Stress Testing Pool ===" << std::endl;

    BlockingObjectPool<int> pool(100);
    const int numThreads = 8;
    const int opsPerThread = 50000;
    std::atomic<int> acquired{0};

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < opsPerThread; ++i) {
                auto obj = pool.tryAcquireFor(std::chrono::microseconds(1));
                if (obj) {
                    ++acquired;
                    *obj = i;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    int totalOps = numThreads * opsPerThread;
    double opsPerSec = (totalOps * 1000.0) / duration;

    std::cout << "  Threads: " << numThreads << std::endl;
    std::cout << "  Total ops: " << totalOps << std::endl;
    std::cout << "  Duration: " << duration << "ms" << std::endl;
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0) << opsPerSec << " ops/sec" << std::endl;
    std::cout << "  Acquired: " << acquired << std::endl;
    std::cout << "  Pool available: " << pool.available() << std::endl;

    assert(pool.available() == 100);

    std::cout << "Pool stress test passed!" << std::endl;
}

void stressTestThreadPool() {
    std::cout << "=== Stress Testing ThreadPool ===" << std::endl;

    ThreadPool pool(8);
    const int numTasks = 100000;
    std::atomic<int> completed{0};

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numTasks; ++i) {
        pool.execute([&completed]() {
            ++completed;
        });
    }

    pool.waitAll();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    double tasksPerSec = (numTasks * 1000.0) / duration;

    std::cout << "  Thread count: " << pool.threadCount() << std::endl;
    std::cout << "  Total tasks: " << numTasks << std::endl;
    std::cout << "  Duration: " << duration << "ms" << std::endl;
    std::cout << "  Throughput: " << std::fixed << std::setprecision(0) << tasksPerSec << " tasks/sec" << std::endl;
    std::cout << "  Completed: " << completed << std::endl;

    assert(completed == numTasks);

    std::cout << "ThreadPool stress test passed!" << std::endl;
}

// ==================== Main ====================
int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "    galay-utils Test Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;

    try {
        testString();
        testRandom();
        testSystem();
        testBackTrace();
        testSignalHandler();
        testPool();
        testThread();
        testRateLimiter();
        testCircuitBreaker();
        testConsistentHash();
        testTrieTree();
        testHuffman();
        testMvcc();
        testParser();
        testApp();
        testProcess();
        testTypeName();
        testBase64();
        testMD5();
        testMurmurHash3();
        testSaltGenerator();
        testLoadBalancer();

        // Stress tests
        std::cout << "\n========================================" << std::endl;
        std::cout << "    Stress Tests" << std::endl;
        std::cout << "========================================\n" << std::endl;

        stressTestCircuitBreaker();
        stressTestRateLimiter();
        stressTestPool();
        stressTestThreadPool();

        std::cout << "\n========================================" << std::endl;
        std::cout << "    All tests passed!" << std::endl;
        std::cout << "========================================\n" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
