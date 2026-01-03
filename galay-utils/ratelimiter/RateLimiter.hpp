#ifndef GALAY_UTILS_RATE_LIMITER_HPP
#define GALAY_UTILS_RATE_LIMITER_HPP

#include "../common/Defn.hpp"
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <atomic>
#include <thread>

namespace galay::utils {

/**
 * @brief Counting semaphore for resource limiting
 */
class CountingSemaphore {
public:
    explicit CountingSemaphore(size_t initial = 0) : m_count(initial) {}

    void acquire(size_t n = 1) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this, n] { return m_count >= n; });
        m_count -= n;
    }

    bool tryAcquire(size_t n = 1) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_count >= n) {
            m_count -= n;
            return true;
        }
        return false;
    }

    template<typename Rep, typename Period>
    bool tryAcquireFor(size_t n, const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cv.wait_for(lock, timeout, [this, n] { return m_count >= n; })) {
            return false;
        }
        m_count -= n;
        return true;
    }

    void release(size_t n = 1) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_count += n;
        }
        m_cv.notify_all();
    }

    size_t available() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    size_t m_count;
};

/**
 * @brief Token bucket rate limiter
 */
class TokenBucketLimiter {
public:
    TokenBucketLimiter(double rate, size_t capacity)
        : m_rate(rate)
        , m_capacity(capacity)
        , m_tokens(static_cast<double>(capacity))
        , m_lastRefill(std::chrono::steady_clock::now())
        , m_stopped(false) {}

    ~TokenBucketLimiter() {
        stop();
    }

    bool tryAcquire(size_t tokens = 1) {
        std::lock_guard<std::mutex> lock(m_mutex);
        refill();

        if (m_tokens >= tokens) {
            m_tokens -= tokens;
            return true;
        }
        return false;
    }

    void acquire(size_t tokens = 1) {
        std::unique_lock<std::mutex> lock(m_mutex);

        while (true) {
            refill();

            if (m_tokens >= tokens) {
                m_tokens -= tokens;
                return;
            }

            double needed = tokens - m_tokens;
            auto waitTime = std::chrono::duration<double>(needed / m_rate);
            m_cv.wait_for(lock, waitTime);
        }
    }

    double availableTokens() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tokens;
    }

    void setRate(double rate) {
        std::lock_guard<std::mutex> lock(m_mutex);
        refill();
        m_rate = rate;
    }

    void setCapacity(size_t capacity) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_capacity = capacity;
        if (m_tokens > capacity) {
            m_tokens = static_cast<double>(capacity);
        }
    }

    double rate() const { return m_rate; }
    size_t capacity() const { return m_capacity; }

    void stop() {
        m_stopped = true;
        m_cv.notify_all();
    }

private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - m_lastRefill).count();

        double newTokens = elapsed * m_rate;
        m_tokens = std::min(m_tokens + newTokens, static_cast<double>(m_capacity));
        m_lastRefill = now;
    }

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    double m_rate;
    size_t m_capacity;
    double m_tokens;
    std::chrono::steady_clock::time_point m_lastRefill;
    std::atomic<bool> m_stopped;
};

/**
 * @brief Sliding window rate limiter
 */
class SlidingWindowLimiter {
public:
    SlidingWindowLimiter(size_t maxRequests, std::chrono::milliseconds windowSize)
        : m_maxRequests(maxRequests)
        , m_windowSize(windowSize) {}

    bool tryAcquire() {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto now = std::chrono::steady_clock::now();

        while (!m_timestamps.empty() &&
               now - m_timestamps.front() > m_windowSize) {
            m_timestamps.pop();
        }

        if (m_timestamps.size() < m_maxRequests) {
            m_timestamps.push(now);
            return true;
        }

        return false;
    }

    void acquire() {
        while (!tryAcquire()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    size_t currentCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_timestamps.size();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<std::chrono::steady_clock::time_point> empty;
        std::swap(m_timestamps, empty);
    }

private:
    mutable std::mutex m_mutex;
    size_t m_maxRequests;
    std::chrono::milliseconds m_windowSize;
    std::queue<std::chrono::steady_clock::time_point> m_timestamps;
};

/**
 * @brief Leaky bucket rate limiter
 */
class LeakyBucketLimiter {
public:
    LeakyBucketLimiter(double rate, size_t capacity)
        : m_rate(rate)
        , m_capacity(capacity)
        , m_water(0)
        , m_lastLeak(std::chrono::steady_clock::now()) {}

    bool tryAcquire(size_t amount = 1) {
        std::lock_guard<std::mutex> lock(m_mutex);
        leak();

        if (m_water + amount <= m_capacity) {
            m_water += amount;
            return true;
        }
        return false;
    }

    void acquire(size_t amount = 1) {
        while (!tryAcquire(amount)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    double currentLevel() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_water;
    }

private:
    void leak() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - m_lastLeak).count();

        double leaked = elapsed * m_rate;
        m_water = std::max(0.0, m_water - leaked);
        m_lastLeak = now;
    }

    mutable std::mutex m_mutex;
    double m_rate;
    size_t m_capacity;
    double m_water;
    std::chrono::steady_clock::time_point m_lastLeak;
};

} // namespace galay::utils

#endif // GALAY_UTILS_RATE_LIMITER_HPP
