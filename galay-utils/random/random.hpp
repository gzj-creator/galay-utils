/**
 * @file random.hpp
 * @brief 线程安全随机数生成器
 * @author galay-utils
 * @version 1.0.0
 *
 * @details 提供线程安全的随机数生成功能，支持整数、浮点数、布尔值、
 *          字符串、十六进制、字节和 UUID 生成。单例模式，使用互斥锁保证线程安全。
 */

#ifndef GALAY_UTILS_RANDOM_HPP
#define GALAY_UTILS_RANDOM_HPP

#include "galay-utils/common/defn.hpp"
#include <random>
#include <string>
#include <mutex>

namespace galay::utils {

/**
 * @brief 线程安全随机数生成器
 * @details 单例模式，使用 Mersenne Twister 19937_64 引擎和互斥锁保证线程安全。
 */
class Randomizer {
public:
    /**
     * @brief 获取单例实例
     * @return 随机数生成器实例引用
     */
    static Randomizer& instance() {
        static Randomizer instance;
        return instance;
    }

    /**
     * @brief 生成随机整数
     * @param min 最小值（含）
     * @param max 最大值（含）
     * @return 随机整数
     */
    int randomInt(int min, int max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<int> dist(min, max);
        return dist(m_engine);
    }

    /**
     * @brief 生成随机 32 位无符号整数
     * @param min 最小值（含）
     * @param max 最大值（含）
     * @return 随机 32 位无符号整数
     */
    uint32_t randomUint32(uint32_t min, uint32_t max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<uint32_t> dist(min, max);
        return dist(m_engine);
    }

    /**
     * @brief 生成随机 64 位无符号整数
     * @param min 最小值（含）
     * @param max 最大值（含）
     * @return 随机 64 位无符号整数
     */
    uint64_t randomUint64(uint64_t min, uint64_t max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<uint64_t> dist(min, max);
        return dist(m_engine);
    }

    /**
     * @brief 生成随机双精度浮点数
     * @param min 最小值（含）
     * @param max 最大值（不含）
     * @return 随机双精度浮点数
     */
    double randomDouble(double min, double max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_real_distribution<double> dist(min, max);
        return dist(m_engine);
    }

    /**
     * @brief 生成随机单精度浮点数
     * @param min 最小值（含）
     * @param max 最大值（不含）
     * @return 随机单精度浮点数
     */
    float randomFloat(float min, float max) {
        if (min >= max) return min;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_real_distribution<float> dist(min, max);
        return dist(m_engine);
    }

    /**
     * @brief 生成随机布尔值
     * @param probability 为 true 的概率（默认 0.5）
     * @return 随机布尔值
     */
    bool randomBool(double probability = 0.5) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::bernoulli_distribution dist(probability);
        return dist(m_engine);
    }

    /**
     * @brief 生成随机字符串
     * @param length 字符串长度
     * @param charset 字符集（默认为字母数字）
     * @return 随机字符串
     */
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

    /**
     * @brief 生成随机十六进制字符串
     * @param length 字符串长度
     * @param uppercase 是否使用大写字母
     * @return 随机十六进制字符串
     */
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

    /**
     * @brief 生成随机字节序列
     * @param buffer 输出缓冲区
     * @param length 字节数量
     */
    void randomBytes(uint8_t* buffer, size_t length) {
        if (length == 0) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        std::uniform_int_distribution<int> dist(0, 255);

        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<uint8_t>(dist(m_engine));
        }
    }

    /**
     * @brief 生成 UUID（版本 4）
     * @return 36 字符的 UUID 字符串
     */
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
