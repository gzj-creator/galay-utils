#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

// Include all modules
#include "../galay-utils/galay-utils.hpp"

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

    // Counting semaphore
    CountingSemaphore sem(3);
    assert(sem.available() == 3);

    sem.acquire(2);
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
        testLoadBalancer();

        std::cout << "\n========================================" << std::endl;
        std::cout << "    All tests passed!" << std::endl;
        std::cout << "========================================\n" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
