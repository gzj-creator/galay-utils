/**
 * @file breaker.hpp
 * @brief 无锁熔断器实现
 * @author galay-utils
 * @version 1.0.0
 *
 * @details 使用原子变量实现熔断器状态管理，适合高并发场景和协程环境。
 *          支持 Closed -> Open -> HalfOpen 状态转换和带降级的执行模式。
 */

#ifndef GALAY_UTILS_CIRCUIT_BREAKER_HPP
#define GALAY_UTILS_CIRCUIT_BREAKER_HPP

#include "galay-utils/common/defn.hpp"
#include <chrono>
#include <atomic>
#include <stdexcept>

namespace galay::utils {

/// 熔断器状态枚举
enum class CircuitState : int { Closed = 0, Open = 1, HalfOpen = 2 };

/**
 * @brief 熔断器配置结构体
 * @details 定义熔断器的失败阈值、成功阈值和重置超时时间。
 */
struct CircuitBreakerConfig {
    size_t failureThreshold = 5; ///< 触发熔断的连续失败次数阈值
    size_t successThreshold = 3; ///< 半开状态下恢复的连续成功次数阈值
    std::chrono::seconds resetTimeout{30}; ///< 熔断后到半开状态的超时时间
};

/**
 * @brief 无锁熔断器实现
 *
 * 使用原子变量实现状态管理，适合高并发场景和协程环境。
 *
 * 状态转换：
 * - Closed -> Open: 连续失败次数达到阈值
 * - Open -> HalfOpen: 超时后自动转换
 * - HalfOpen -> Closed: 连续成功次数达到阈值
 * - HalfOpen -> Open: 发生失败
 */
class CircuitBreaker {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit CircuitBreaker(CircuitBreakerConfig config = {})
        : m_config(std::move(config))
        , m_state(static_cast<int>(CircuitState::Closed))
        , m_failureCount(0)
        , m_successCount(0)
        , m_lastFailureTime(0) {}

    /**
     * @brief 判断是否允许请求通过
     * @return 允许请求返回 true，熔断中返回 false
     */
    bool allowRequest() {
        auto currentState = static_cast<CircuitState>(m_state.load(std::memory_order_acquire));

        switch (currentState) {
            case CircuitState::Closed:
                return true;

            case CircuitState::Open: {
                auto now = Clock::now().time_since_epoch().count();
                auto lastFailure = m_lastFailureTime.load(std::memory_order_acquire);
                auto timeoutNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    m_config.resetTimeout).count();

                if (now - lastFailure >= timeoutNs) {
                    // 尝试转换到 HalfOpen
                    int expected = static_cast<int>(CircuitState::Open);
                    if (m_state.compare_exchange_strong(expected,
                            static_cast<int>(CircuitState::HalfOpen),
                            std::memory_order_acq_rel)) {
                        m_successCount.store(0, std::memory_order_release);
                        m_failureCount.store(0, std::memory_order_release);
                    }
                    return true;
                }
                return false;
            }

            case CircuitState::HalfOpen:
                return true;
        }

        return false;
    }

    /**
     * @brief 记录一次成功调用
     */
    void onSuccess() {
        auto currentState = static_cast<CircuitState>(m_state.load(std::memory_order_acquire));

        switch (currentState) {
            case CircuitState::Closed:
                m_failureCount.store(0, std::memory_order_release);
                break;

            case CircuitState::HalfOpen: {
                auto count = m_successCount.fetch_add(1, std::memory_order_acq_rel) + 1;
                if (count >= m_config.successThreshold) {
                    // 尝试转换到 Closed
                    int expected = static_cast<int>(CircuitState::HalfOpen);
                    if (m_state.compare_exchange_strong(expected,
                            static_cast<int>(CircuitState::Closed),
                            std::memory_order_acq_rel)) {
                        m_failureCount.store(0, std::memory_order_release);
                        m_successCount.store(0, std::memory_order_release);
                    }
                }
                break;
            }

            case CircuitState::Open:
                break;
        }
    }

    /**
     * @brief 记录一次失败调用
     */
    void onFailure() {
        auto now = Clock::now().time_since_epoch().count();
        m_lastFailureTime.store(now, std::memory_order_release);

        auto currentState = static_cast<CircuitState>(m_state.load(std::memory_order_acquire));

        switch (currentState) {
            case CircuitState::Closed: {
                auto count = m_failureCount.fetch_add(1, std::memory_order_acq_rel) + 1;
                if (count >= m_config.failureThreshold) {
                    // 尝试转换到 Open
                    int expected = static_cast<int>(CircuitState::Closed);
                    m_state.compare_exchange_strong(expected,
                        static_cast<int>(CircuitState::Open),
                        std::memory_order_acq_rel);
                }
                break;
            }

            case CircuitState::HalfOpen: {
                // 直接转换到 Open
                int expected = static_cast<int>(CircuitState::HalfOpen);
                if (m_state.compare_exchange_strong(expected,
                        static_cast<int>(CircuitState::Open),
                        std::memory_order_acq_rel)) {
                    m_failureCount.store(0, std::memory_order_release);
                    m_successCount.store(0, std::memory_order_release);
                }
                break;
            }

            case CircuitState::Open:
                break;
        }
    }

    /**
     * @brief 在熔断保护下执行函数
     * @tparam F 可调用对象类型
     * @param f 待执行的函数
     * @return 函数执行结果
     * @throws std::runtime_error 熔断器处于开启状态时抛出异常
     */
    template<typename F>
    auto execute(F&& f) -> decltype(f()) {
        if (!allowRequest()) {
            throw std::runtime_error("Circuit breaker is open");
        }

        try {
            auto result = f();
            onSuccess();
            return result;
        } catch (...) {
            onFailure();
            throw;
        }
    }

    /**
     * @brief 在熔断保护下执行函数，失败时调用降级函数
     * @tparam F 主函数类型
     * @tparam Fallback 降级函数类型
     * @param f 待执行的主函数
     * @param fallback 降级函数
     * @return 主函数或降级函数的执行结果
     */
    template<typename F, typename Fallback>
    auto executeWithFallback(F&& f, Fallback&& fallback) -> decltype(f()) {
        if (!allowRequest()) {
            return fallback();
        }

        try {
            auto result = f();
            onSuccess();
            return result;
        } catch (...) {
            onFailure();
            return fallback();
        }
    }

    /**
     * @brief 获取当前熔断器状态
     * @return 当前状态枚举值
     */
    CircuitState state() const {
        return static_cast<CircuitState>(m_state.load(std::memory_order_acquire));
    }

    /**
     * @brief 获取当前状态的可读字符串
     * @return 状态字符串（"CLOSED"、"OPEN"、"HALF_OPEN"）
     */
    const char* stateString() const {
        switch (state()) {
            case CircuitState::Closed: return "CLOSED";
            case CircuitState::Open: return "OPEN";
            case CircuitState::HalfOpen: return "HALF_OPEN";
        }
        return "UNKNOWN";
    }

    /**
     * @brief 获取失败计数
     * @return 当前失败次数
     */
    size_t failureCount() const {
        return m_failureCount.load(std::memory_order_acquire);
    }

    /**
     * @brief 获取成功计数
     * @return 当前成功次数
     */
    size_t successCount() const {
        return m_successCount.load(std::memory_order_acquire);
    }

    /**
     * @brief 重置熔断器为关闭状态
     */
    void reset() {
        m_state.store(static_cast<int>(CircuitState::Closed), std::memory_order_release);
        m_failureCount.store(0, std::memory_order_release);
        m_successCount.store(0, std::memory_order_release);
    }

    /**
     * @brief 强制将熔断器设置为开启状态
     */
    void forceOpen() {
        m_state.store(static_cast<int>(CircuitState::Open), std::memory_order_release);
        m_lastFailureTime.store(Clock::now().time_since_epoch().count(), std::memory_order_release);
    }

    /**
     * @brief 获取熔断器配置
     * @return 配置的常引用
     */
    const CircuitBreakerConfig& config() const { return m_config; }

private:
    CircuitBreakerConfig m_config;
    std::atomic<int> m_state;
    std::atomic<size_t> m_failureCount;
    std::atomic<size_t> m_successCount;
    std::atomic<int64_t> m_lastFailureTime;
};

} // namespace galay::utils

#endif // GALAY_UTILS_CIRCUIT_BREAKER_HPP
