#ifndef GALAY_UTILS_RANDOM_HPP
#define GALAY_UTILS_RANDOM_HPP

#include "../common/Defn.hpp"
#include <random>
#include <string>
#include <mutex>

namespace galay::utils {

/**
 * @brief Thread-safe random number generator
 */
class Randomizer {
public:
    static Randomizer& instance() {
        static Randomizer instance;
        return instance;
    }

    int randomInt(int min, int max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<int> dist(min, max);
        return dist(m_engine);
    }

    uint32_t randomUint32(uint32_t min, uint32_t max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<uint32_t> dist(min, max);
        return dist(m_engine);
    }

    uint64_t randomUint64(uint64_t min, uint64_t max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<uint64_t> dist(min, max);
        return dist(m_engine);
    }

    double randomDouble(double min, double max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_real_distribution<double> dist(min, max);
        return dist(m_engine);
    }

    float randomFloat(float min, float max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_real_distribution<float> dist(min, max);
        return dist(m_engine);
    }

    bool randomBool(double probability = 0.5) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::bernoulli_distribution dist(probability);
        return dist(m_engine);
    }

    std::string randomString(size_t length,
                             const std::string& charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") {
        if (charset.empty() || length == 0) return "";

        std::string result;
        result.resize(length); // 预分配内存

        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<size_t> dist(0, charset.length() - 1);

        for (size_t i = 0; i < length; ++i) {
            result[i] = charset[dist(m_engine)]; // 直接赋值，避免+=操作
        }

        return result;
    }

    std::string randomHex(size_t length, bool uppercase = false) {
        if (length == 0) return "";

        const char* hexChars = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

        std::string result;
        result.resize(length); // 预分配内存

        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<int> dist(0, 15);

        for (size_t i = 0; i < length; ++i) {
            result[i] = hexChars[dist(m_engine)]; // 直接赋值
        }

        return result;
    }

    void randomBytes(uint8_t* buffer, size_t length) {
        if (length == 0) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<int> dist(0, 255);

        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<uint8_t>(dist(m_engine));
        }
    }

    std::string uuid() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<int> dist(0, 15);
        std::uniform_int_distribution<int> dist2(8, 11);

        const char* hexChars = "0123456789abcdef";
        std::string uuid(36, '-');

        for (int i = 0; i < 36; ++i) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                continue;
            } else if (i == 14) {
                uuid[i] = '4';
            } else if (i == 19) {
                uuid[i] = hexChars[dist2(m_engine)];
            } else {
                uuid[i] = hexChars[dist(m_engine)];
            }
        }

        return uuid;
    }

    void seed(uint64_t seed) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_engine.seed(seed);
    }

    void reseed() {
        std::random_device rd;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_engine.seed(rd());
    }

private:
    Randomizer() {
        std::random_device rd;
        m_engine.seed(rd());
    }
    ~Randomizer() = default;

    Randomizer(const Randomizer&) = delete;
    Randomizer& operator=(const Randomizer&) = delete;

    std::mt19937_64 m_engine;
    std::mutex m_mutex;
};

} // namespace galay::utils

#endif // GALAY_UTILS_RANDOM_HPP
