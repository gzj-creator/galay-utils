#include "../test_common.hpp"

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
    std::cout << "\n=== concurrency_test ===" << std::endl;
    try {
        testPool();
        testThread();
        stressTestPool();
        stressTestThreadPool();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
