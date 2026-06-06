#include "../test_common.hpp"

void testRateLimiter() {
    std::cout << "=== Testing RateLimiter ===" << std::endl;

    // Counting semaphore - 使用 tryAcquire 测试基本功能
    CountingSemaphore sem(3);
    assert(sem.available() == 3);

    assert(sem.tryAcquire(2));
    assert(sem.available() == 1);
    assert(!sem.tryAcquire(2));
    assert(sem.available() == 1);

    sem.release(2);
    assert(sem.available() == 3);
    assert(sem.tryAcquire(3));
    assert(sem.available() == 0);
    sem.release(3);

    // Token bucket
    TokenBucketLimiter tokenBucket(100, 10); // 100 tokens/sec, capacity 10

    // Should be able to acquire immediately (bucket starts full)
    assert(tokenBucket.tryAcquire(5));
    assert(tokenBucket.availableTokens() >= 4); // At least 5 consumed
    assert(tokenBucket.tryAcquire(5));
    assert(!tokenBucket.tryAcquire(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    assert(tokenBucket.tryAcquire(1));
    tokenBucket.setCapacity(3);
    assert(tokenBucket.capacity() == 3);
    assert(tokenBucket.availableTokens() <= 3.0);
    tokenBucket.setRate(200);
    assert(tokenBucket.rate() == 200);

    // Sliding window
    SlidingWindowLimiter slidingWindow(5, std::chrono::milliseconds(100));
    assert(slidingWindow.maxRequests() == 5);
    assert(slidingWindow.windowSize() == std::chrono::milliseconds(100));

    for (int i = 0; i < 5; ++i) {
        assert(slidingWindow.tryAcquire());
    }
    assert(!slidingWindow.tryAcquire()); // Should be rate limited

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    assert(slidingWindow.tryAcquire()); // Should work after window expires

    // Leaky bucket
    LeakyBucketLimiter leakyBucket(100, 3);
    assert(leakyBucket.rate() == 100);
    assert(leakyBucket.capacity() == 3);
    assert(leakyBucket.tryAcquire(2));
    assert(!leakyBucket.tryAcquire(2));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(leakyBucket.tryAcquire(1));
    assert(leakyBucket.currentWater() <= 3.0);

    LeakyBucketLimiter slowLeakyBucket(10, 2);
    assert(slowLeakyBucket.tryAcquire(2));
    assert(!slowLeakyBucket.tryAcquire(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    assert(slowLeakyBucket.tryAcquire(1));

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

    const int threadCount = 4;
    const int iterations = 100000;

    // Test CountingSemaphore
    {
        CountingSemaphore sem(100);
        std::atomic<int> acquired{0};

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (int thread_index = 0; thread_index < threadCount; ++thread_index) {
            threads.emplace_back([&]() {
                for (int i = 0; i < iterations; ++i) {
                    if (sem.tryAcquire(1)) {
                        ++acquired;
                        sem.release(1);
                    }
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        int totalOps = acquired.load();
        double opsPerSec = duration > 0 ? (totalOps * 1000.0) / duration : 0;

        std::cout << "  [CountingSemaphore - Thread]" << std::endl;
        std::cout << "    Threads: " << threadCount << ", Iterations: " << threadCount * iterations << std::endl;
        std::cout << "    Duration: " << duration << "ms" << std::endl;
        std::cout << "    Throughput: " << std::fixed << std::setprecision(0) << opsPerSec << " ops/sec" << std::endl;
        std::cout << "    Acquired: " << acquired << std::endl;
        assert(sem.available() == 100);
    }

    // Test TokenBucketLimiter
    {
        TokenBucketLimiter limiter(10000000, 100000); // 10M tokens/sec, capacity 100000
        std::atomic<int> acquired{0};

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (int thread_index = 0; thread_index < threadCount; ++thread_index) {
            threads.emplace_back([&]() {
                for (int i = 0; i < iterations; ++i) {
                    if (limiter.tryAcquire(1)) {
                        ++acquired;
                    }
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        int totalOps = threadCount * iterations;
        double opsPerSec = duration > 0 ? (totalOps * 1000.0) / duration : 0;

        std::cout << "  [TokenBucketLimiter - Thread]" << std::endl;
        std::cout << "    Threads: " << threadCount << ", Iterations: " << totalOps << std::endl;
        std::cout << "    Duration: " << duration << "ms" << std::endl;
        std::cout << "    Throughput: " << std::fixed << std::setprecision(0) << opsPerSec << " ops/sec" << std::endl;
        std::cout << "    Acquired: " << acquired << std::endl;
    }

    // Test SlidingWindowLimiter
    {
        SlidingWindowLimiter limiter(1000000, std::chrono::milliseconds(1000));
        std::atomic<int> acquired{0};

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (int thread_index = 0; thread_index < threadCount; ++thread_index) {
            threads.emplace_back([&]() {
                for (int i = 0; i < iterations / 2; ++i) {
                    if (limiter.tryAcquire()) {
                        ++acquired;
                    }
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        int totalOps = threadCount * (iterations / 2);
        double opsPerSec = duration > 0 ? (totalOps * 1000.0) / duration : 0;

        std::cout << "  [SlidingWindowLimiter - Thread]" << std::endl;
        std::cout << "    Threads: " << threadCount << ", Iterations: " << totalOps << std::endl;
        std::cout << "    Duration: " << duration << "ms" << std::endl;
        std::cout << "    Throughput: " << std::fixed << std::setprecision(0) << opsPerSec << " ops/sec" << std::endl;
        std::cout << "    Acquired: " << acquired << std::endl;
    }

    std::cout << "RateLimiter stress test passed!" << std::endl;
}

int main() {
    std::cout << "\n=== resilience_test ===" << std::endl;
    try {
        testRateLimiter();
        testCircuitBreaker();
        stressTestCircuitBreaker();
        stressTestRateLimiter();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
