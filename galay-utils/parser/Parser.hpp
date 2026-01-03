#ifndef GALAY_UTILS_PARSER_HPP
#define GALAY_UTILS_PARSER_HPP

#include "../common/Defn.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <sstream>
#include <fstream>
#include <functional>

namespace galay::utils {

class ParserBase {
public:
    virtual ~ParserBase() = default;

    virtual bool parseFile(const std::string& path) = 0;
    virtual bool parseString(const std::string& content) = 0;
    virtual std::optional<std::string> getValue(const std::string& key) const = 0;
    virtual bool hasKey(const std::string& key) const = 0;
    virtual std::vector<std::string> getKeys() const = 0;

    template<typename T>
    T getValueAs(const std::string& key, T defaultValue = T{}) const {
        auto val = getValue(key);
        if (!val) return defaultValue;

        std::istringstream iss(*val);
        T result;
        if (iss >> result) {
            return result;
        }
        return defaultValue;
    }

    const std::string& lastError() const { return m_lastError; }

protected:
    std::string m_lastError;
};

class ConfigParser : public ParserBase {
public:
    ConfigParser() = default;

    bool parseFile(const std::string& path) override {
        std::ifstream file(path);
        if (!file) {
            m_lastError = "Failed to open file: " + path;
            return false;
        }

        std::ostringstream oss;
        oss << file.rdbuf();
        return parseString(oss.str());
    }

    bool parseString(const std::string& content) override {
        m_values.clear();
        std::string currentSection;

        std::istringstream iss(content);
        std::string line;
        int lineNum = 0;

        while (std::getline(iss, line)) {
            ++lineNum;
            line = trim(line);

            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }

            auto eqPos = line.find('=');
            if (eqPos == std::string::npos) {
                m_lastError = "Invalid line " + std::to_string(lineNum) + ": " + line;
                return false;
            }

            std::string key = trim(line.substr(0, eqPos));
            std::string value = trim(line.substr(eqPos + 1));

            if (value.length() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.length() - 2);
            }

            value = processEscapes(value);

            std::string fullKey = currentSection.empty() ? key : currentSection + "." + key;
            m_values[fullKey] = value;
        }

        return true;
    }

    std::optional<std::string> getValue(const std::string& key) const override {
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool hasKey(const std::string& key) const override {
        return m_values.find(key) != m_values.end();
    }

    std::vector<std::string> getKeys() const override {
        std::vector<std::string> keys;
        keys.reserve(m_values.size());
        for (const auto& [k, v] : m_values) {
            keys.push_back(k);
        }
        return keys;
    }

    std::vector<std::string> getKeysInSection(const std::string& section) const {
        std::vector<std::string> keys;
        std::string prefix = section + ".";
        for (const auto& [k, v] : m_values) {
            if (k.substr(0, prefix.length()) == prefix) {
                keys.push_back(k.substr(prefix.length()));
            }
        }
        return keys;
    }

    std::vector<std::string> getArray(const std::string& key) const {
        auto val = getValue(key);
        if (!val) return {};

        std::vector<std::string> result;
        std::istringstream iss(*val);
        std::string item;
        while (std::getline(iss, item, ',')) {
            result.push_back(trim(item));
        }
        return result;
    }

private:
    static std::string trim(const std::string& str) {
        size_t start = 0;
        size_t end = str.length();
        while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) ++start;
        while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) --end;
        return str.substr(start, end - start);
    }

    static std::string processEscapes(const std::string& str) {
        std::string result;
        result.reserve(str.length());

        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '\\' && i + 1 < str.length()) {
                switch (str[i + 1]) {
                    case 'n': result += '\n'; ++i; break;
                    case 't': result += '\t'; ++i; break;
                    case 'r': result += '\r'; ++i; break;
                    case '\\': result += '\\'; ++i; break;
                    case '"': result += '"'; ++i; break;
                    case '\'': result += '\''; ++i; break;
                    default: result += str[i]; break;
                }
            } else {
                result += str[i];
            }
        }

        return result;
    }

    std::unordered_map<std::string, std::string> m_values;
};

class EnvParser : public ParserBase {
public:
    EnvParser() = default;

    bool parseFile(const std::string& path) override {
        std::ifstream file(path);
        if (!file) {
            m_lastError = "Failed to open file: " + path;
            return false;
        }

        std::ostringstream oss;
        oss << file.rdbuf();
        return parseString(oss.str());
    }

    bool parseString(const std::string& content) override {
        m_values.clear();

        std::istringstream iss(content);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            if (line.substr(0, 7) == "export ") {
                line = line.substr(7);
            }

            auto eqPos = line.find('=');
            if (eqPos == std::string::npos) {
                continue;
            }

            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            if (value.length() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.length() - 2);
            }

            m_values[key] = value;
        }

        return true;
    }

    std::optional<std::string> getValue(const std::string& key) const override {
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool hasKey(const std::string& key) const override {
        return m_values.find(key) != m_values.end();
    }

    std::vector<std::string> getKeys() const override {
        std::vector<std::string> keys;
        keys.reserve(m_values.size());
        for (const auto& [k, v] : m_values) {
            keys.push_back(k);
        }
        return keys;
    }

private:
    std::unordered_map<std::string, std::string> m_values;
};

class ParserManager {
public:
    using Creator = std::function<std::unique_ptr<ParserBase>()>;

    static ParserManager& instance() {
        static ParserManager instance;
        return instance;
    }

    void registerParser(const std::string& extension, Creator creator) {
        m_creators[extension] = std::move(creator);
    }

    std::unique_ptr<ParserBase> createParser(const std::string& path) {
        auto dotPos = path.rfind('.');
        if (dotPos == std::string::npos) {
            return nullptr;
        }

        std::string ext = path.substr(dotPos);
        auto it = m_creators.find(ext);
        if (it != m_creators.end()) {
            return it->second();
        }

        return nullptr;
    }

private:
    ParserManager() {
        registerParser(".conf", []() { return std::make_unique<ConfigParser>(); });
        registerParser(".ini", []() { return std::make_unique<ConfigParser>(); });
        registerParser(".env", []() { return std::make_unique<EnvParser>(); });
    }

    std::unordered_map<std::string, Creator> m_creators;
};

} // namespace galay::utils

#endif // GALAY_UTILS_PARSER_HPP
