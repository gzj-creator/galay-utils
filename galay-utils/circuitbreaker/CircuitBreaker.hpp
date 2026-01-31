#ifndef GALAY_UTILS_CIRCUIT_BREAKER_HPP
#define GALAY_UTILS_CIRCUIT_BREAKER_HPP

#include "../common/Defn.hpp"
#include <chrono>
#include <atomic>
#include <stdexcept>

namespace galay::utils {

enum class CircuitState : int { Closed = 0, Open = 1, HalfOpen = 2 };

struct CircuitBreakerConfig {
    size_t failureThreshold = 5;
    size_t successThreshold = 3;
    std::chrono::seconds resetTimeout{30};
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

    CircuitState state() const {
        return static_cast<CircuitState>(m_state.load(std::memory_order_acquire));
    }

    const char* stateString() const {
        switch (state()) {
            case CircuitState::Closed: return "CLOSED";
            case CircuitState::Open: return "OPEN";
            case CircuitState::HalfOpen: return "HALF_OPEN";
        }
        return "UNKNOWN";
    }

    size_t failureCount() const {
        return m_failureCount.load(std::memory_order_acquire);
    }

    size_t successCount() const {
        return m_successCount.load(std::memory_order_acquire);
    }

    void reset() {
        m_state.store(static_cast<int>(CircuitState::Closed), std::memory_order_release);
        m_failureCount.store(0, std::memory_order_release);
        m_successCount.store(0, std::memory_order_release);
    }

    void forceOpen() {
        m_state.store(static_cast<int>(CircuitState::Open), std::memory_order_release);
        m_lastFailureTime.store(Clock::now().time_since_epoch().count(), std::memory_order_release);
    }

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
