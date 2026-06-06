/**
 * @file rate_limiter.hpp
 * @brief 限流器集合
 * @author galay-utils
 * @version 1.0.0
 *
 * @details 提供四种限流器实现：计数信号量、令牌桶、滑动窗口和漏桶。
 *          所有限流器均为非阻塞 API，不适合在协程中直接使用。
 */

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
 * @note 本模块不提供协程支持。限流器仅暴露同步非阻塞的 tryAcquire() API。
 *       不要在期望挂起/唤醒行为的协程调度器中直接调用这些类型，
 *       应在上层实现重试、等待或超时策略。
 */

/**
 * @brief 无锁计数信号量（非阻塞获取）
 * @details 使用原子计数器实现许可的获取和释放，不阻塞调用线程。
 */
class CountingSemaphore {
public:
    explicit CountingSemaphore(size_t initial = 0)
        : m_count(initial) {}

    /**
     * @brief 非阻塞尝试获取许可
     * @param count 请求的许可数量
     * @return 成功获取返回 true，否则返回 false
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
     * @brief 释放许可
     * @param count 释放的许可数量
     */
    void release(size_t count = 1) {
        m_count.fetch_add(count, std::memory_order_release);
    }

    /**
     * @brief 获取当前可用许可数量
     * @return 当前许可数量
     */
    size_t available() const {
        return m_count.load(std::memory_order_acquire);
    }

private:
    std::atomic<size_t> m_count;
};

/**
 * @brief 非阻塞令牌桶限流器
 * @details 按令牌/秒速率填充令牌，直到容量上限。线程安全，不阻塞调用者。
 */
class TokenBucketLimiter {
public:
    TokenBucketLimiter(double rate, size_t capacity)
        : m_rate(rate)
        , m_capacity(capacity)
        , m_tokens(static_cast<int64_t>(capacity) * kPrecision)
        , m_last_refill_time(nowTicks()) {}

    /**
     * @brief 非阻塞尝试消费令牌
     * @param tokens 请求的令牌数量
     * @return 令牌充足返回 true，否则返回 false
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
     * @brief 获取当前可用令牌数
     * @return 令牌数量（浮点值）
     */
    double availableTokens() const {
        return static_cast<double>(m_tokens.load(std::memory_order_acquire)) / kPrecision;
    }

    /**
     * @brief 设置令牌填充速率
     * @param rate 每秒填充的令牌数
     */
    void setRate(double rate) {
        refill();
        m_rate = rate;
    }

    /**
     * @brief 设置桶容量并裁剪当前令牌数
     * @param capacity 最大令牌数量
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
     * @brief 获取配置的填充速率
     * @return 每秒令牌数
     */
    double rate() const { return m_rate; }

    /**
     * @brief 获取配置的桶容量
     * @return 最大令牌数量
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
 * @brief 线程安全滑动窗口限流器
 * @details 每次成功获取记录时间戳，超过窗口内最大请求数时返回 false。
 *          使用内部互斥锁保护请求窗口，不适合作为协程等待对象。
 */
class SlidingWindowLimiter {
public:
    SlidingWindowLimiter(size_t maxRequests, std::chrono::milliseconds windowSize)
        : m_max_requests(maxRequests)
        , m_window_size(windowSize) {}

    /**
     * @brief 非阻塞尝试记录一次请求
     * @return 允许请求返回 true，被限流返回 false
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
     * @brief 获取配置的请求限制
     * @return 窗口内最大请求数
     */
    size_t maxRequests() const { return m_max_requests; }

    /**
     * @brief 获取配置的窗口大小
     * @return 滑动窗口时长
     */
    std::chrono::milliseconds windowSize() const { return m_window_size; }

private:
    size_t m_max_requests;
    std::chrono::milliseconds m_window_size;
    std::deque<std::chrono::steady_clock::time_point> m_requests;
    std::mutex m_mutex;
};

/**
 * @brief 非阻塞漏桶限流器
 * @details 桶按配置速率漏水，仅在不超过容量时接受新水。线程安全，不阻塞。
 */
class LeakyBucketLimiter {
public:
    LeakyBucketLimiter(double rate, size_t capacity)
        : m_rate(rate)
        , m_capacity(capacity)
        , m_water(0)
        , m_last_leak_time(nowTicks()) {}

    /**
     * @brief 非阻塞尝试添加水量
     * @param amount 请求的水量
     * @return 容量允许返回 true，否则返回 false
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
     * @brief 获取当前桶内水量
     * @return 水量（浮点值）
     */
    double currentWater() const {
        return static_cast<double>(m_water.load(std::memory_order_acquire)) / kPrecision;
    }

    /**
     * @brief 获取配置的漏水速率
     * @return 每秒漏水单位数
     */
    double rate() const { return m_rate; }

    /**
     * @brief 获取配置的桶容量
     * @return 最大水量
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
