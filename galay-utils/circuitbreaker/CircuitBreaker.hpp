#ifndef GALAY_UTILS_CIRCUIT_BREAKER_HPP
#define GALAY_UTILS_CIRCUIT_BREAKER_HPP

#include "../common/Defn.hpp"
#include <mutex>
#include <chrono>
#include <atomic>
#include <functional>
#include <deque>
#include <stdexcept>

namespace galay::utils {

enum class CircuitState { Closed, Open, HalfOpen };

struct CircuitBreakerConfig {
    size_t failureThreshold = 5;
    size_t successThreshold = 3;
    std::chrono::seconds resetTimeout{30};
    double slowCallThreshold = 5.0;
    size_t slidingWindowSize = 10;
    double slowCallRateThreshold = 0.5;
};

class CircuitBreaker {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit CircuitBreaker(CircuitBreakerConfig config = {})
        : m_config(std::move(config))
        , m_state(CircuitState::Closed)
        , m_failureCount(0)
        , m_successCount(0)
        , m_lastFailureTime(TimePoint::min()) {}

    bool allowRequest() {
        std::lock_guard<std::mutex> lock(m_mutex);

        switch (m_state) {
            case CircuitState::Closed:
                return true;

            case CircuitState::Open: {
                auto now = Clock::now();
                if (now - m_lastFailureTime >= m_config.resetTimeout) {
                    m_state = CircuitState::HalfOpen;
                    m_successCount = 0;
                    m_failureCount = 0;
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
        std::lock_guard<std::mutex> lock(m_mutex);

        m_metrics.push_back({true, 0.0, Clock::now()});
        trimMetrics();

        switch (m_state) {
            case CircuitState::Closed:
                m_failureCount = 0;
                break;

            case CircuitState::HalfOpen:
                ++m_successCount;
                if (m_successCount >= m_config.successThreshold) {
                    m_state = CircuitState::Closed;
                    m_failureCount = 0;
                    m_successCount = 0;
                }
                break;

            case CircuitState::Open:
                break;
        }
    }

    void onFailure() {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_metrics.push_back({false, 0.0, Clock::now()});
        trimMetrics();
        m_lastFailureTime = Clock::now();

        switch (m_state) {
            case CircuitState::Closed:
                ++m_failureCount;
                if (m_failureCount >= m_config.failureThreshold) {
                    m_state = CircuitState::Open;
                }
                break;

            case CircuitState::HalfOpen:
                m_state = CircuitState::Open;
                m_failureCount = 0;
                m_successCount = 0;
                break;

            case CircuitState::Open:
                break;
        }
    }

    void onSlowCall(double duration) {
        std::lock_guard<std::mutex> lock(m_mutex);

        bool isSlow = duration >= m_config.slowCallThreshold;
        m_metrics.push_back({true, duration, Clock::now()});
        trimMetrics();

        if (m_state == CircuitState::Closed && isSlow) {
            size_t slowCount = 0;
            for (const auto& m : m_metrics) {
                if (m.duration >= m_config.slowCallThreshold) {
                    ++slowCount;
                }
            }

            double slowRate = static_cast<double>(slowCount) / m_metrics.size();
            if (slowRate >= m_config.slowCallRateThreshold) {
                m_state = CircuitState::Open;
                m_lastFailureTime = Clock::now();
            }
        }
    }

    template<typename F>
    auto execute(F&& f) -> decltype(f()) {
        if (!allowRequest()) {
            throw std::runtime_error("Circuit breaker is open");
        }

        auto start = Clock::now();
        try {
            auto result = f();
            auto duration = std::chrono::duration<double>(Clock::now() - start).count();

            if (duration >= m_config.slowCallThreshold) {
                onSlowCall(duration);
            } else {
                onSuccess();
            }

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

        auto start = Clock::now();
        try {
            auto result = f();
            auto duration = std::chrono::duration<double>(Clock::now() - start).count();

            if (duration >= m_config.slowCallThreshold) {
                onSlowCall(duration);
            } else {
                onSuccess();
            }

            return result;
        } catch (...) {
            onFailure();
            return fallback();
        }
    }

    CircuitState state() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state;
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
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_failureCount;
    }

    size_t successCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_successCount;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state = CircuitState::Closed;
        m_failureCount = 0;
        m_successCount = 0;
        m_metrics.clear();
    }

    void forceOpen() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state = CircuitState::Open;
        m_lastFailureTime = Clock::now();
    }

    const CircuitBreakerConfig& config() const { return m_config; }

private:
    struct Metric {
        bool success;
        double duration;
        TimePoint timestamp;
    };

    void trimMetrics() {
        while (m_metrics.size() > m_config.slidingWindowSize) {
            m_metrics.pop_front();
        }
    }

    CircuitBreakerConfig m_config;
    mutable std::mutex m_mutex;
    CircuitState m_state;
    size_t m_failureCount;
    size_t m_successCount;
    TimePoint m_lastFailureTime;
    std::deque<Metric> m_metrics;
};

} // namespace galay::utils

#endif // GALAY_UTILS_CIRCUIT_BREAKER_HPP
