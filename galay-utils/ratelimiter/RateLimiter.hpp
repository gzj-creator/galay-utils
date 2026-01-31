#ifndef GALAY_UTILS_RATE_LIMITER_HPP
#define GALAY_UTILS_RATE_LIMITER_HPP

#include <atomic>
#include <chrono>

#include <galay-kernel/kernel/Coroutine.h>
#include <galay-kernel/kernel/Waker.h>
#include <galay-kernel/kernel/Timeout.hpp>
#include <galay-kernel/common/Error.h>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include <expected>

namespace galay::utils {

class SemaphoreAwaitable;
class TokenBucketAwaitable;
class SlidingWindowAwaitable;
class LeakyBucketAwaitable;

/**
 * @brief 无锁计数信号量
 *
 * 使用原子变量实现，适合高并发和协程环境。
 */
class CountingSemaphore {
public:
    explicit CountingSemaphore(size_t initial = 0)
        : m_count(initial) {}

    /**
     * @brief 尝试获取信号量（非阻塞）
     * @param n 获取数量
     * @return true 成功，false 失败
     */
    bool tryAcquire(size_t n = 1) {
        size_t current = m_count.load(std::memory_order_acquire);
        while (current >= n) {
            if (m_count.compare_exchange_weak(current, current - n,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 异步获取信号量
     * @param n 获取数量
     * @return 可等待对象，支持 co_await 和 .timeout()
     *
     * @code
     * co_await semaphore.acquire();
     * // 或带超时
     * auto result = co_await semaphore.acquire().timeout(100ms);
     * if (!result) { // 超时 }
     * @endcode
     */
    SemaphoreAwaitable acquire(size_t n = 1);

    void release(size_t n = 1) {
        m_count.fetch_add(n, std::memory_order_release);
        wakeWaiters();
    }

    size_t available() const {
        return m_count.load(std::memory_order_acquire);
    }

private:
    friend class SemaphoreAwaitable;

    struct WaiterInfo {
        kernel::Waker waker;
        size_t count;
    };

    moodycamel::ConcurrentQueue<WaiterInfo> m_waiters;

    void wakeWaiters() {
        WaiterInfo info;
        while (m_waiters.try_dequeue(info)) {
            if (tryAcquire(info.count)) {
                info.waker.wakeUp();
            } else {
                m_waiters.enqueue(info);
                break;
            }
        }
    }

    std::atomic<size_t> m_count;
};

/**
 * @brief 无锁令牌桶限流器
 *
 * 使用原子变量和 CAS 操作实现，高性能无锁设计。
 */
class TokenBucketLimiter {
public:
    TokenBucketLimiter(double rate, size_t capacity)
        : m_rate(rate)
        , m_capacity(capacity)
        , m_tokens(static_cast<int64_t>(capacity) * kPrecision)
        , m_lastRefillTime(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    /**
     * @brief 尝试获取令牌（非阻塞）
     * @param tokens 获取令牌数
     * @return true 成功，false 失败
     */
    bool tryAcquire(size_t tokens = 1) {
        refill();

        int64_t needed = static_cast<int64_t>(tokens) * kPrecision;
        int64_t current = m_tokens.load(std::memory_order_acquire);

        while (current >= needed) {
            if (m_tokens.compare_exchange_weak(current, current - needed,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                wakeWaiters();
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 异步获取令牌
     * @param tokens 获取令牌数
     * @return 可等待对象，支持 co_await 和 .timeout()
     */
    TokenBucketAwaitable acquire(size_t tokens = 1);

    double availableTokens() const {
        return static_cast<double>(m_tokens.load(std::memory_order_acquire)) / kPrecision;
    }

    void setRate(double rate) {
        refill();
        m_rate = rate;
    }

    void setCapacity(size_t capacity) {
        m_capacity = capacity;
        int64_t maxTokens = static_cast<int64_t>(capacity) * kPrecision;
        int64_t current = m_tokens.load(std::memory_order_acquire);
        while (current > maxTokens) {
            if (m_tokens.compare_exchange_weak(current, maxTokens,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                break;
            }
        }
    }

    double rate() const { return m_rate; }
    size_t capacity() const { return m_capacity; }

private:
    static constexpr int64_t kPrecision = 1000000; // 微秒精度

    friend class TokenBucketAwaitable;

    struct WaiterInfo {
        kernel::Waker waker;
        size_t tokens;
    };

    moodycamel::ConcurrentQueue<WaiterInfo> m_waiters;

    void wakeWaiters() {
        WaiterInfo info;
        while (m_waiters.try_dequeue(info)) {
            if (tryAcquire(info.tokens)) {
                info.waker.wakeUp();
            } else {
                m_waiters.enqueue(info);
                break;
            }
        }
    }

    void refill() {
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        int64_t lastTime = m_lastRefillTime.load(std::memory_order_acquire);

        double elapsed = static_cast<double>(now - lastTime) / 1e9;
        if (elapsed <= 0) return;

        if (!m_lastRefillTime.compare_exchange_strong(lastTime, now,
                std::memory_order_acq_rel, std::memory_order_acquire)) {
            return;
        }

        int64_t newTokens = static_cast<int64_t>(elapsed * m_rate * kPrecision);
        int64_t maxTokens = static_cast<int64_t>(m_capacity) * kPrecision;

        int64_t current = m_tokens.load(std::memory_order_acquire);
        int64_t desired;
        do {
            desired = std::min(current + newTokens, maxTokens);
        } while (!m_tokens.compare_exchange_weak(current, desired,
                std::memory_order_acq_rel, std::memory_order_acquire));

        wakeWaiters();
    }

    double m_rate;
    size_t m_capacity;
    std::atomic<int64_t> m_tokens;
    std::atomic<int64_t> m_lastRefillTime;
};

/**
 * @brief 无锁滑动窗口限流器
 *
 * 使用原子计数器和时间窗口实现，简化版高性能设计。
 */
class SlidingWindowLimiter {
public:
    SlidingWindowLimiter(size_t maxRequests, std::chrono::milliseconds windowSize)
        : m_maxRequests(maxRequests)
        , m_windowSizeNs(std::chrono::duration_cast<std::chrono::nanoseconds>(windowSize).count())
        , m_windowStart(std::chrono::steady_clock::now().time_since_epoch().count())
        , m_count(0) {}

    /**
     * @brief 尝试获取（非阻塞）
     * @return true 成功，false 失败
     */
    bool tryAcquire() {
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        int64_t windowStart = m_windowStart.load(std::memory_order_acquire);

        if (now - windowStart >= m_windowSizeNs) {
            if (m_windowStart.compare_exchange_strong(windowStart, now,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                m_count.store(0, std::memory_order_release);
                wakeWaiters();
            }
        }

        size_t current = m_count.load(std::memory_order_acquire);
        while (current < m_maxRequests) {
            if (m_count.compare_exchange_weak(current, current + 1,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 异步获取
     * @return 可等待对象，支持 co_await 和 .timeout()
     */
    SlidingWindowAwaitable acquire();

    size_t currentCount() const {
        return m_count.load(std::memory_order_acquire);
    }

    void reset() {
        m_windowStart.store(std::chrono::steady_clock::now().time_since_epoch().count(),
                            std::memory_order_release);
        m_count.store(0, std::memory_order_release);
        wakeWaiters();
    }

private:
    friend class SlidingWindowAwaitable;

    moodycamel::ConcurrentQueue<kernel::Waker> m_waiters;

    void wakeWaiters() {
        kernel::Waker waker;
        while (m_waiters.try_dequeue(waker)) {
            if (tryAcquire()) {
                waker.wakeUp();
            } else {
                m_waiters.enqueue(waker);
                break;
            }
        }
    }

    size_t m_maxRequests;
    int64_t m_windowSizeNs;
    std::atomic<int64_t> m_windowStart;
    std::atomic<size_t> m_count;
};

/**
 * @brief 无锁漏桶限流器
 *
 * 使用原子变量实现，控制请求的平滑流出。
 */
class LeakyBucketLimiter {
public:
    LeakyBucketLimiter(double rate, size_t capacity)
        : m_rate(rate)
        , m_capacity(capacity)
        , m_water(0)
        , m_lastLeakTime(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    /**
     * @brief 尝试获取（非阻塞）
     * @param amount 获取数量
     * @return true 成功，false 失败
     */
    bool tryAcquire(size_t amount = 1) {
        leak();

        int64_t needed = static_cast<int64_t>(amount) * kPrecision;
        int64_t maxWater = static_cast<int64_t>(m_capacity) * kPrecision;

        int64_t current = m_water.load(std::memory_order_acquire);
        while (current + needed <= maxWater) {
            if (m_water.compare_exchange_weak(current, current + needed,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 异步获取
     * @param amount 获取数量
     * @return 可等待对象，支持 co_await 和 .timeout()
     */
    LeakyBucketAwaitable acquire(size_t amount = 1);

    double currentLevel() const {
        return static_cast<double>(m_water.load(std::memory_order_acquire)) / kPrecision;
    }

private:
    static constexpr int64_t kPrecision = 1000000;

    friend class LeakyBucketAwaitable;

    struct WaiterInfo {
        kernel::Waker waker;
        size_t amount;
    };

    moodycamel::ConcurrentQueue<WaiterInfo> m_waiters;

    void wakeWaiters() {
        WaiterInfo info;
        while (m_waiters.try_dequeue(info)) {
            if (tryAcquire(info.amount)) {
                info.waker.wakeUp();
            } else {
                m_waiters.enqueue(info);
                break;
            }
        }
    }

    void leak() {
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        int64_t lastTime = m_lastLeakTime.load(std::memory_order_acquire);

        double elapsed = static_cast<double>(now - lastTime) / 1e9;
        if (elapsed <= 0) return;

        if (!m_lastLeakTime.compare_exchange_strong(lastTime, now,
                std::memory_order_acq_rel, std::memory_order_acquire)) {
            return;
        }

        int64_t leaked = static_cast<int64_t>(elapsed * m_rate * kPrecision);

        int64_t current = m_water.load(std::memory_order_acquire);
        int64_t desired;
        do {
            desired = std::max(int64_t(0), current - leaked);
        } while (!m_water.compare_exchange_weak(current, desired,
                std::memory_order_acq_rel, std::memory_order_acquire));

        wakeWaiters();
    }

    double m_rate;
    size_t m_capacity;
    std::atomic<int64_t> m_water;
    std::atomic<int64_t> m_lastLeakTime;
};

// ==================== Awaitable 实现 ====================

class SemaphoreAwaitable : public kernel::TimeoutSupport<SemaphoreAwaitable> {
public:
    SemaphoreAwaitable(CountingSemaphore* sem, size_t n)
        : m_semaphore(sem), m_count(n) {}

    bool await_ready() const noexcept {
        if (m_semaphore->tryAcquire(m_count)) {
            const_cast<SemaphoreAwaitable*>(this)->m_result = {};
            return true;
        }
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        m_semaphore->m_waiters.enqueue({kernel::Waker(handle), m_count});
        if (m_semaphore->tryAcquire(m_count)) {
            m_result = {};
            return false;
        }
        return true;
    }

    std::expected<void, kernel::IOError> await_resume() noexcept { return m_result; }

private:
    friend struct kernel::WithTimeout<SemaphoreAwaitable>;
    CountingSemaphore* m_semaphore;
    size_t m_count;
    std::expected<void, kernel::IOError> m_result;
};

inline SemaphoreAwaitable CountingSemaphore::acquire(size_t n) {
    return SemaphoreAwaitable(this, n);
}

class TokenBucketAwaitable : public kernel::TimeoutSupport<TokenBucketAwaitable> {
public:
    TokenBucketAwaitable(TokenBucketLimiter* limiter, size_t tokens)
        : m_limiter(limiter), m_tokens(tokens) {}

    bool await_ready() const noexcept {
        if (m_limiter->tryAcquire(m_tokens)) {
            const_cast<TokenBucketAwaitable*>(this)->m_result = {};
            return true;
        }
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        m_limiter->m_waiters.enqueue({kernel::Waker(handle), m_tokens});
        if (m_limiter->tryAcquire(m_tokens)) {
            m_result = {};
            return false;
        }
        return true;
    }

    std::expected<void, kernel::IOError> await_resume() noexcept { return m_result; }

private:
    friend struct kernel::WithTimeout<TokenBucketAwaitable>;
    TokenBucketLimiter* m_limiter;
    size_t m_tokens;
    std::expected<void, kernel::IOError> m_result;
};

inline TokenBucketAwaitable TokenBucketLimiter::acquire(size_t tokens) {
    return TokenBucketAwaitable(this, tokens);
}

class SlidingWindowAwaitable : public kernel::TimeoutSupport<SlidingWindowAwaitable> {
public:
    SlidingWindowAwaitable(SlidingWindowLimiter* limiter)
        : m_limiter(limiter) {}

    bool await_ready() const noexcept {
        if (m_limiter->tryAcquire()) {
            const_cast<SlidingWindowAwaitable*>(this)->m_result = {};
            return true;
        }
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        m_limiter->m_waiters.enqueue(kernel::Waker(handle));
        if (m_limiter->tryAcquire()) {
            m_result = {};
            return false;
        }
        return true;
    }

    std::expected<void, kernel::IOError> await_resume() noexcept { return m_result; }

private:
    friend struct kernel::WithTimeout<SlidingWindowAwaitable>;
    SlidingWindowLimiter* m_limiter;
    std::expected<void, kernel::IOError> m_result;
};

inline SlidingWindowAwaitable SlidingWindowLimiter::acquire() {
    return SlidingWindowAwaitable(this);
}

class LeakyBucketAwaitable : public kernel::TimeoutSupport<LeakyBucketAwaitable> {
public:
    LeakyBucketAwaitable(LeakyBucketLimiter* limiter, size_t amount)
        : m_limiter(limiter), m_amount(amount) {}

    bool await_ready() const noexcept {
        if (m_limiter->tryAcquire(m_amount)) {
            const_cast<LeakyBucketAwaitable*>(this)->m_result = {};
            return true;
        }
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        m_limiter->m_waiters.enqueue({kernel::Waker(handle), m_amount});
        if (m_limiter->tryAcquire(m_amount)) {
            m_result = {};
            return false;
        }
        return true;
    }

    std::expected<void, kernel::IOError> await_resume() noexcept { return m_result; }

private:
    friend struct kernel::WithTimeout<LeakyBucketAwaitable>;
    LeakyBucketLimiter* m_limiter;
    size_t m_amount;
    std::expected<void, kernel::IOError> m_result;
};

inline LeakyBucketAwaitable LeakyBucketLimiter::acquire(size_t amount) {
    return LeakyBucketAwaitable(this, amount);
}

} // namespace galay::utils

#endif // GALAY_UTILS_RATE_LIMITER_HPP
