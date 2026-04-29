#ifndef GALAY_UTILS_RATE_LIMITER_HPP
#define GALAY_UTILS_RATE_LIMITER_HPP

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>

namespace galay::utils {

/**
 * @note Coroutine support is intentionally disabled for this module.
 *
 * Rate limiters only expose synchronous, non-blocking tryAcquire() APIs. The
 * previous coroutine acquire()/awaitable path was removed because it pulled in
 * galay-kernel and because some limiter implementations use mutex-protected
 * state. Do not call these types from a coroutine scheduler expecting suspend /
 * wake behavior; adapt retry, waiting, or timeout policy in the upper layer.
 */

/**
 * @brief Lock-free counting semaphore with non-blocking acquisition.
 *
 * CountingSemaphore stores an atomic permit count. tryAcquire() never blocks
 * and returns false when there are not enough permits. release() increases the
 * available count. This type has no coroutine or scheduler dependency.
 */
class CountingSemaphore {
public:
    explicit CountingSemaphore(size_t initial = 0)
        : m_count(initial) {}

    /**
     * @brief Try to acquire permits without blocking.
     * @param count Number of permits requested.
     * @return true when the permits were acquired; false otherwise.
     */
    bool tryAcquire(size_t count = 1) {
        size_t current = m_count.load(std::memory_order_acquire);
        while (current >= count) {
            if (m_count.compare_exchange_weak(current, current - count,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Release permits back to the semaphore.
     * @param count Number of permits to release.
     */
    void release(size_t count = 1) {
        m_count.fetch_add(count, std::memory_order_release);
    }

    /**
     * @brief Return the current number of available permits.
     * @return Current permit count.
     */
    size_t available() const {
        return m_count.load(std::memory_order_acquire);
    }

private:
    std::atomic<size_t> m_count;
};

/**
 * @brief Non-blocking token bucket rate limiter.
 *
 * The bucket refills according to a token-per-second rate up to capacity.
 * tryAcquire() consumes tokens when enough are available and returns false
 * otherwise. This type is thread-safe and does not block callers.
 */
class TokenBucketLimiter {
public:
    TokenBucketLimiter(double rate, size_t capacity)
        : m_rate(rate)
        , m_capacity(capacity)
        , m_tokens(static_cast<int64_t>(capacity) * kPrecision)
        , m_last_refill_time(nowTicks()) {}

    /**
     * @brief Try to consume tokens without blocking.
     * @param tokens Number of tokens requested.
     * @return true when enough tokens were available; false otherwise.
     */
    bool tryAcquire(size_t tokens = 1) {
        refill();

        int64_t needed = static_cast<int64_t>(tokens) * kPrecision;
        int64_t current = m_tokens.load(std::memory_order_acquire);

        while (current >= needed) {
            if (m_tokens.compare_exchange_weak(current, current - needed,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Return currently available tokens.
     * @return Token count as a floating-point value.
     */
    double availableTokens() const {
        return static_cast<double>(m_tokens.load(std::memory_order_acquire)) / kPrecision;
    }

    /**
     * @brief Update token refill rate.
     * @param rate Tokens added per second.
     */
    void setRate(double rate) {
        refill();
        m_rate = rate;
    }

    /**
     * @brief Update bucket capacity and clamp current tokens if needed.
     * @param capacity Maximum token count.
     */
    void setCapacity(size_t capacity) {
        m_capacity = capacity;
        int64_t max_tokens = static_cast<int64_t>(capacity) * kPrecision;
        int64_t current = m_tokens.load(std::memory_order_acquire);
        while (current > max_tokens) {
            if (m_tokens.compare_exchange_weak(current, max_tokens,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                break;
            }
        }
    }

    /**
     * @brief Return configured refill rate.
     * @return Tokens per second.
     */
    double rate() const { return m_rate; }

    /**
     * @brief Return configured bucket capacity.
     * @return Maximum token count.
     */
    size_t capacity() const { return m_capacity; }

private:
    static constexpr int64_t kPrecision = 1000000;

    static int64_t nowTicks() {
        return std::chrono::steady_clock::now().time_since_epoch().count();
    }

    void refill() {
        int64_t now = nowTicks();
        int64_t last_time = m_last_refill_time.load(std::memory_order_acquire);

        while (now > last_time) {
            double elapsed_seconds = std::chrono::duration<double>(
                std::chrono::steady_clock::duration(now - last_time)).count();
            int64_t tokens_to_add = static_cast<int64_t>(elapsed_seconds * m_rate * kPrecision);
            if (tokens_to_add <= 0) {
                return;
            }

            if (m_last_refill_time.compare_exchange_weak(last_time, now,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                int64_t max_tokens = static_cast<int64_t>(m_capacity) * kPrecision;
                int64_t current = m_tokens.load(std::memory_order_acquire);
                int64_t desired;
                do {
                    desired = std::min(max_tokens, current + tokens_to_add);
                } while (!m_tokens.compare_exchange_weak(current, desired,
                    std::memory_order_acq_rel, std::memory_order_acquire));
                return;
            }
        }
    }

    double m_rate;
    size_t m_capacity;
    std::atomic<int64_t> m_tokens;
    std::atomic<int64_t> m_last_refill_time;
};

/**
 * @brief Thread-safe sliding-window limiter.
 *
 * Each successful acquisition records a timestamp. Calls beyond maxRequests
 * inside the active window return false. Old timestamps are removed during
 * tryAcquire(). This type uses an internal mutex to protect the request window,
 * so it is intentionally not exposed as a coroutine awaitable.
 */
class SlidingWindowLimiter {
public:
    SlidingWindowLimiter(size_t maxRequests, std::chrono::milliseconds windowSize)
        : m_max_requests(maxRequests)
        , m_window_size(windowSize) {}

    /**
     * @brief Try to record one request in the current window.
     * @return true when the request is allowed; false when rate limited.
     */
    bool tryAcquire() {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(m_mutex);

        auto cutoff = now - m_window_size;
        while (!m_requests.empty() && m_requests.front() < cutoff) {
            m_requests.pop_front();
        }

        if (m_requests.size() < m_max_requests) {
            m_requests.push_back(now);
            return true;
        }
        return false;
    }

    /**
     * @brief Return configured request limit.
     * @return Maximum requests in the window.
     */
    size_t maxRequests() const { return m_max_requests; }

    /**
     * @brief Return configured window size.
     * @return Sliding window duration.
     */
    std::chrono::milliseconds windowSize() const { return m_window_size; }

private:
    size_t m_max_requests;
    std::chrono::milliseconds m_window_size;
    std::deque<std::chrono::steady_clock::time_point> m_requests;
    std::mutex m_mutex;
};

/**
 * @brief Non-blocking leaky bucket limiter.
 *
 * The bucket leaks at a configured rate and accepts new water only when the
 * capacity would not be exceeded. tryAcquire() is thread-safe and non-blocking.
 */
class LeakyBucketLimiter {
public:
    LeakyBucketLimiter(double rate, size_t capacity)
        : m_rate(rate)
        , m_capacity(capacity)
        , m_water(0)
        , m_last_leak_time(nowTicks()) {}

    /**
     * @brief Try to add water to the bucket.
     * @param amount Requested amount.
     * @return true when capacity allows the amount; false otherwise.
     */
    bool tryAcquire(size_t amount = 1) {
        leak();

        int64_t requested = static_cast<int64_t>(amount) * kPrecision;
        int64_t max_water = static_cast<int64_t>(m_capacity) * kPrecision;
        int64_t current = m_water.load(std::memory_order_acquire);

        while (current + requested <= max_water) {
            if (m_water.compare_exchange_weak(current, current + requested,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Return current bucket water amount.
     * @return Water amount as floating-point value.
     */
    double currentWater() const {
        return static_cast<double>(m_water.load(std::memory_order_acquire)) / kPrecision;
    }

    /**
     * @brief Return configured leak rate.
     * @return Units leaked per second.
     */
    double rate() const { return m_rate; }

    /**
     * @brief Return configured bucket capacity.
     * @return Maximum water amount.
     */
    size_t capacity() const { return m_capacity; }

private:
    static constexpr int64_t kPrecision = 1000000;

    static int64_t nowTicks() {
        return std::chrono::steady_clock::now().time_since_epoch().count();
    }

    void leak() {
        int64_t now = nowTicks();
        int64_t last_time = m_last_leak_time.load(std::memory_order_acquire);

        while (now > last_time) {
            double elapsed_seconds = std::chrono::duration<double>(
                std::chrono::steady_clock::duration(now - last_time)).count();
            int64_t leaked = static_cast<int64_t>(elapsed_seconds * m_rate * kPrecision);
            if (leaked <= 0) {
                return;
            }

            if (m_last_leak_time.compare_exchange_weak(last_time, now,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                int64_t current = m_water.load(std::memory_order_acquire);
                int64_t desired;
                do {
                    desired = std::max<int64_t>(0, current - leaked);
                } while (!m_water.compare_exchange_weak(current, desired,
                    std::memory_order_acq_rel, std::memory_order_acquire));
                return;
            }
        }
    }

    double m_rate;
    size_t m_capacity;
    std::atomic<int64_t> m_water;
    std::atomic<int64_t> m_last_leak_time;
};

} // namespace galay::utils

#endif // GALAY_UTILS_RATE_LIMITER_HPP
