#ifndef GALAY_UTILS_SIGNAL_HANDLER_HPP
#define GALAY_UTILS_SIGNAL_HANDLER_HPP

#include "../common/Defn.hpp"
#include <functional>
#include <unordered_map>
#include <mutex>
#include <csignal>

namespace galay::utils {

/**
 * @brief Signal handler utility class
 */
class SignalHandler {
public:
    using Handler = std::function<void(int)>;

    static SignalHandler& instance() {
        static SignalHandler instance;
        return instance;
    }

    bool setHandler(int signal, Handler handler) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_handlers[signal] = std::move(handler);

#if defined(_WIN32)
        auto result = std::signal(signal, &SignalHandler::signalCallback);
        return result != SIG_ERR;
#else
        struct sigaction sa{};
        sa.sa_handler = &SignalHandler::signalCallback;
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sa.sa_mask);
        return sigaction(signal, &sa, nullptr) == 0;
#endif
    }

    template<int... Signals>
    bool setHandler(Handler handler) {
        bool success = true;
        ((success &= setHandler(Signals, handler)), ...);
        return success;
    }

    bool removeHandler(int signal) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_handlers.erase(signal);

#if defined(_WIN32)
        auto result = std::signal(signal, SIG_DFL);
        return result != SIG_ERR;
#else
        struct sigaction sa{};
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        return sigaction(signal, &sa, nullptr) == 0;
#endif
    }

    bool restoreDefault(int signal) {
        return removeHandler(signal);
    }

    bool ignoreSignal(int signal) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_handlers.erase(signal);

#if defined(_WIN32)
        auto result = std::signal(signal, SIG_IGN);
        return result != SIG_ERR;
#else
        struct sigaction sa{};
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        return sigaction(signal, &sa, nullptr) == 0;
#endif
    }

    bool blockSignal(int signal) {
#if defined(_WIN32)
        return false;
#else
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, signal);
        return pthread_sigmask(SIG_BLOCK, &set, nullptr) == 0;
#endif
    }

    bool unblockSignal(int signal) {
#if defined(_WIN32)
        return false;
#else
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, signal);
        return pthread_sigmask(SIG_UNBLOCK, &set, nullptr) == 0;
#endif
    }

    bool hasHandler(int signal) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_handlers.find(signal) != m_handlers.end();
    }

private:
    SignalHandler() = default;
    ~SignalHandler() = default;

    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;

    static void signalCallback(int signal) {
        auto& self = instance();
        std::lock_guard<std::mutex> lock(self.m_mutex);

        auto it = self.m_handlers.find(signal);
        if (it != self.m_handlers.end() && it->second) {
            it->second(signal);
        }
    }

    mutable std::mutex m_mutex;
    std::unordered_map<int, Handler> m_handlers;
};

} // namespace galay::utils

#endif // GALAY_UTILS_SIGNAL_HANDLER_HPP
