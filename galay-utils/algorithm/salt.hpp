#ifndef GALAY_UTILS_SALT_H
#define GALAY_UTILS_SALT_H

#include <string>
#include <vector>
#include <random>
#include <array>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace galay::utils
{
    class SaltGenerator
    {
    public:
        // Generate salt as hex string
        static std::string generateHex(size_t length = 32);

        // Generate salt as base64 string
        static std::string generateBase64(size_t length = 32);

        // Generate salt as raw bytes
        static std::vector<uint8_t> generateBytes(size_t length = 32);

        // Generate salt with custom charset
        static std::string generateCustom(size_t length, const std::string& charset);

        // Generate cryptographically secure random salt (hex)
        static std::string generateSecureHex(size_t length = 32);

        // Generate cryptographically secure random salt (base64)
        static std::string generateSecureBase64(size_t length = 32);

        // Generate cryptographically secure random bytes
        static std::vector<uint8_t> generateSecureBytes(size_t length = 32);

        // Generate salt for password hashing (bcrypt-style, 22 chars base64)
        static std::string generateBcryptSalt();

        // Generate salt with timestamp prefix (for time-based uniqueness)
        static std::string generateTimestamped(size_t length = 32);

        // Validate salt format
        static bool isValidHex(const std::string& salt);
        static bool isValidBase64(const std::string& salt);

    private:
        // Base64 encoding for salt
        static std::string toBase64(const uint8_t* data, size_t length);

        // Hex encoding
        static std::string toHex(const uint8_t* data, size_t length);

        // Get cryptographically secure random bytes
        static void getSecureRandomBytes(uint8_t* buffer, size_t length);

        // Base64 charset for bcrypt
        static constexpr const char* BCRYPT_BASE64 =
            "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    };

    // Implementation

    inline std::string SaltGenerator::toHex(const uint8_t* data, size_t length)
    {
        static const char hexChars[] = "0123456789abcdef";
        std::string result;
        result.reserve(length * 2);

        for (size_t i = 0; i < length; ++i)
        {
            result.push_back(hexChars[data[i] >> 4]);
            result.push_back(hexChars[data[i] & 0x0F]);
        }

        return result;
    }

    inline std::string SaltGenerator::toBase64(const uint8_t* data, size_t length)
    {
        static const char base64Chars[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string result;
        result.reserve(((length + 2) / 3) * 4);

        for (size_t i = 0; i < length; i += 3)
        {
            uint32_t n = static_cast<uint32_t>(data[i]) << 16;
            if (i + 1 < length) n |= static_cast<uint32_t>(data[i + 1]) << 8;
            if (i + 2 < length) n |= static_cast<uint32_t>(data[i + 2]);

            result.push_back(base64Chars[(n >> 18) & 0x3F]);
            result.push_back(base64Chars[(n >> 12) & 0x3F]);
            result.push_back(i + 1 < length ? base64Chars[(n >> 6) & 0x3F] : '=');
            result.push_back(i + 2 < length ? base64Chars[n & 0x3F] : '=');
        }

        return result;
    }

    inline void SaltGenerator::getSecureRandomBytes(uint8_t* buffer, size_t length)
    {
        std::random_device rd;

        // Fill buffer with random bytes
        for (size_t i = 0; i < length; ++i)
        {
            buffer[i] = static_cast<uint8_t>(rd() & 0xFF);
        }
    }

    inline std::vector<uint8_t> SaltGenerator::generateBytes(size_t length)
    {
        std::vector<uint8_t> salt(length);
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<int> dis(0, 255);

        for (size_t i = 0; i < length; ++i)
        {
            salt[i] = static_cast<uint8_t>(dis(gen));
        }

        return salt;
    }

    inline std::vector<uint8_t> SaltGenerator::generateSecureBytes(size_t length)
    {
        std::vector<uint8_t> salt(length);
        getSecureRandomBytes(salt.data(), length);
        return salt;
    }

    inline std::string SaltGenerator::generateHex(size_t length)
    {
        auto bytes = generateBytes(length);
        return toHex(bytes.data(), bytes.size());
    }

    inline std::string SaltGenerator::generateSecureHex(size_t length)
    {
        auto bytes = generateSecureBytes(length);
        return toHex(bytes.data(), bytes.size());
    }

    inline std::string SaltGenerator::generateBase64(size_t length)
    {
        auto bytes = generateBytes(length);
        return toBase64(bytes.data(), bytes.size());
    }

    inline std::string SaltGenerator::generateSecureBase64(size_t length)
    {
        auto bytes = generateSecureBytes(length);
        return toBase64(bytes.data(), bytes.size());
    }

    inline std::string SaltGenerator::generateCustom(size_t length, const std::string& charset)
    {
        if (charset.empty() || length == 0)
            return "";

        std::string result;
        result.reserve(length);

        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<size_t> dis(0, charset.length() - 1);

        for (size_t i = 0; i < length; ++i)
        {
            result.push_back(charset[dis(gen)]);
        }

        return result;
    }

    inline std::string SaltGenerator::generateBcryptSalt()
    {
        // Bcrypt uses 22 characters of base64-encoded salt
        std::vector<uint8_t> bytes = generateSecureBytes(16);

        std::string result;
        result.reserve(22);

        // Use bcrypt's custom base64 encoding
        for (size_t i = 0; i < 16; i += 3)
        {
            uint32_t n = static_cast<uint32_t>(bytes[i]) << 16;
            if (i + 1 < 16) n |= static_cast<uint32_t>(bytes[i + 1]) << 8;
            if (i + 2 < 16) n |= static_cast<uint32_t>(bytes[i + 2]);

            result.push_back(BCRYPT_BASE64[(n >> 18) & 0x3F]);
            result.push_back(BCRYPT_BASE64[(n >> 12) & 0x3F]);
            if (i + 1 < 16) result.push_back(BCRYPT_BASE64[(n >> 6) & 0x3F]);
            if (i + 2 < 16) result.push_back(BCRYPT_BASE64[n & 0x3F]);
        }

        // Truncate to 22 characters
        if (result.length() > 22)
            result.resize(22);

        return result;
    }

    inline std::string SaltGenerator::generateTimestamped(size_t length)
    {
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        // Convert timestamp to hex
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(16) << timestamp;
        std::string timestampHex = ss.str();

        // Generate random part
        size_t randomLength = (length > 16) ? (length - 16) / 2 : 0;
        std::string randomPart = generateSecureHex(randomLength);

        return timestampHex + randomPart;
    }

    inline bool SaltGenerator::isValidHex(const std::string& salt)
    {
        if (salt.empty())
            return false;

        for (char c : salt)
        {
            if (!((c >= '0' && c <= '9') ||
                  (c >= 'a' && c <= 'f') ||
                  (c >= 'A' && c <= 'F')))
            {
                return false;
            }
        }

        return true;
    }

    inline bool SaltGenerator::isValidBase64(const std::string& salt)
    {
        if (salt.empty())
            return false;

        for (char c : salt)
        {
            if (!((c >= 'A' && c <= 'Z') ||
                  (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') ||
                  c == '+' || c == '/' || c == '='))
            {
                return false;
            }
        }

        return true;
    }

} // namespace galay::utils

#endif /* GALAY_UTILS_SALT_H */
