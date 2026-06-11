#include "../test_common.hpp"

#include <expected>
#include <variant>

enum class CircuitBreakerTestError {
    OperationFailed
};

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

void testCircuitBreakerExpectedExecution() {
    std::cout << "=== Testing CircuitBreaker expected execution ===" << std::endl;

    using Error = std::variant<CircuitBreakerTestError, CircuitBreakerError>;
    using Result = std::expected<int, Error>;

    static_assert(CircuitBreakerExpected<Result>);
    static_assert(!CircuitBreakerExpected<int>);

    CircuitBreakerConfig config;
    config.failureThreshold = 2;
    config.successThreshold = 1;
    config.resetTimeout = std::chrono::seconds(1);

    CircuitBreaker cb(config);
    int calls = 0;

    auto success = cb.execute([&]() -> Result {
        ++calls;
        return 7;
    });
    assert(success.has_value());
    assert(*success == 7);
    assert(calls == 1);
    assert(cb.failureCount() == 0);
    assert(cb.state() == CircuitState::Closed);

    auto firstFailure = cb.execute([&]() -> Result {
        ++calls;
        return std::unexpected(Error{CircuitBreakerTestError::OperationFailed});
    });
    assert(!firstFailure.has_value());
    assert(std::holds_alternative<CircuitBreakerTestError>(firstFailure.error()));
    assert(calls == 2);
    assert(cb.failureCount() == 1);
    assert(cb.state() == CircuitState::Closed);

    auto secondFailure = cb.execute([&]() -> Result {
        ++calls;
        return std::unexpected(Error{CircuitBreakerTestError::OperationFailed});
    });
    assert(!secondFailure.has_value());
    assert(calls == 3);
    assert(cb.state() == CircuitState::Open);

    auto blocked = cb.execute([&]() -> Result {
        ++calls;
        return 99;
    });
    assert(!blocked.has_value());
    assert(std::get<CircuitBreakerError>(blocked.error()) == CircuitBreakerError::Open);
    assert(calls == 3);

    std::cout << "CircuitBreaker expected execution tests passed!" << std::endl;
}

void testCircuitBreakerExpectedFallback() {
    std::cout << "=== Testing CircuitBreaker expected fallback ===" << std::endl;

    using Result = std::expected<int, CircuitBreakerTestError>;

    static_assert(CircuitBreakerExpected<Result>);

    CircuitBreakerConfig config;
    config.failureThreshold = 1;
    config.successThreshold = 1;
    config.resetTimeout = std::chrono::seconds(1);

    CircuitBreaker cb(config);
    int primaryCalls = 0;
    int fallbackCalls = 0;

    auto fallbackAfterFailure = cb.executeWithFallback(
        [&]() -> Result {
            ++primaryCalls;
            return std::unexpected(CircuitBreakerTestError::OperationFailed);
        },
        [&]() -> Result {
            ++fallbackCalls;
            return 42;
        });
    assert(fallbackAfterFailure.has_value());
    assert(*fallbackAfterFailure == 42);
    assert(primaryCalls == 1);
    assert(fallbackCalls == 1);
    assert(cb.state() == CircuitState::Open);

    auto fallbackWhenOpen = cb.executeWithFallback(
        [&]() -> Result {
            ++primaryCalls;
            return 7;
        },
        [&]() -> Result {
            ++fallbackCalls;
            return 43;
        });
    assert(fallbackWhenOpen.has_value());
    assert(*fallbackWhenOpen == 43);
    assert(primaryCalls == 1);
    assert(fallbackCalls == 2);

    std::cout << "CircuitBreaker expected fallback tests passed!" << std::endl;
}

void testCircuitBreakerManualClockTimeout() {
    std::cout << "=== Testing CircuitBreaker manual clock timeout ===" << std::endl;

    ManualClock::reset();

    CircuitBreakerConfig config;
    config.failureThreshold = 1;
    config.successThreshold = 1;
    config.resetTimeout = std::chrono::seconds(1);

    BasicCircuitBreaker<ManualClock> cb(config);

    cb.onFailure();
    assert(cb.state() == CircuitState::Open);

    ManualClock::advance(std::chrono::milliseconds(999));
    assert(!cb.allowRequest());
    assert(cb.state() == CircuitState::Open);

    ManualClock::advance(std::chrono::milliseconds(1));
    assert(cb.allowRequest());
    assert(cb.state() == CircuitState::HalfOpen);

    std::cout << "CircuitBreaker manual clock timeout tests passed!" << std::endl;
}

void testCircuitBreakerHalfOpenProbeLimit() {
    std::cout << "=== Testing CircuitBreaker half-open probe limit ===" << std::endl;

    ManualClock::reset();

    CircuitBreakerConfig config;
    config.failureThreshold = 1;
    config.successThreshold = 2;
    config.resetTimeout = std::chrono::seconds(1);
    config.halfOpenMaxRequests = 1;

    BasicCircuitBreaker<ManualClock> cb(config);

    cb.onFailure();
    assert(cb.state() == CircuitState::Open);

    ManualClock::advance(std::chrono::seconds(1));
    assert(cb.allowRequest());
    assert(cb.state() == CircuitState::HalfOpen);
    assert(!cb.allowRequest());

    cb.onSuccess();
    assert(cb.state() == CircuitState::HalfOpen);
    assert(cb.allowRequest());

    cb.onSuccess();
    assert(cb.state() == CircuitState::Closed);

    std::cout << "CircuitBreaker half-open probe limit tests passed!" << std::endl;
}

void testCircuitBreakerForceOpenUsesCurrentTime() {
    std::cout << "=== Testing CircuitBreaker force open timestamp ===" << std::endl;

    ManualClock::reset();

    CircuitBreakerConfig config;
    config.failureThreshold = 1;
    config.successThreshold = 1;
    config.resetTimeout = std::chrono::seconds(1);

    BasicCircuitBreaker<ManualClock> cb(config);

    ManualClock::advance(std::chrono::seconds(10));
    cb.forceOpen();
    assert(cb.state() == CircuitState::Open);

    ManualClock::advance(std::chrono::milliseconds(999));
    assert(!cb.allowRequest());
    assert(cb.state() == CircuitState::Open);

    ManualClock::advance(std::chrono::milliseconds(1));
    assert(cb.allowRequest());
    assert(cb.state() == CircuitState::HalfOpen);

    std::cout << "CircuitBreaker force open timestamp tests passed!" << std::endl;
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
        testCircuitBreakerExpectedExecution();
        testCircuitBreakerExpectedFallback();
        testCircuitBreakerManualClockTimeout();
        testCircuitBreakerHalfOpenProbeLimit();
        testCircuitBreakerForceOpenUsesCurrentTime();
        stressTestCircuitBreaker();
        stressTestRateLimiter();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
