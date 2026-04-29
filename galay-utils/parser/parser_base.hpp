#ifndef GALAY_UTILS_PARSER_BASE_HPP
#define GALAY_UTILS_PARSER_BASE_HPP

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace galay::utils {

/**
 * @brief Base interface for key-value configuration parsers.
 *
 * Implementations parse file or string content into flattened keys. Section-like
 * formats expose nested values as dot-separated keys, for example
 * `database.host`. Instances are not synchronized; use separate parser objects
 * per thread or protect shared access externally.
 */
class ParserBase {
public:
    virtual ~ParserBase() = default;

    /**
     * @brief Parse content from a file path.
     * @param path File path to read.
     * @return true when the file can be read and parsed; false otherwise.
     */
    virtual bool parseFile(const std::string& path) = 0;

    /**
     * @brief Parse configuration content from memory.
     * @param content Raw configuration text.
     * @return true when parsing succeeds; false otherwise.
     */
    virtual bool parseString(const std::string& content) = 0;

    /**
     * @brief Return the string value for a flattened key.
     * @param key Key name, using dot notation for section values.
     * @return Stored value, or std::nullopt when missing.
     */
    virtual std::optional<std::string> getValue(const std::string& key) const = 0;

    /**
     * @brief Check whether a flattened key exists.
     * @param key Key name to query.
     * @return true when the key exists.
     */
    virtual bool hasKey(const std::string& key) const = 0;

    /**
     * @brief Return all flattened keys currently stored by the parser.
     * @return Keys in unspecified order.
     */
    virtual std::vector<std::string> getKeys() const = 0;

    /**
     * @brief Convert a stored string value with stream extraction.
     * @param key Key name to query.
     * @param defaultValue Value returned when the key is missing or conversion fails.
     * @return Converted value or defaultValue.
     */
    template<typename T>
    T getValueAs(const std::string& key, T defaultValue = T{}) const {
        auto value = getValue(key);
        if (!value) {
            return defaultValue;
        }

        std::istringstream input(*value);
        T result;
        if (input >> result) {
            return result;
        }
        return defaultValue;
    }

    /**
     * @brief Return the last parse or file error.
     * @return Human-readable error text; empty when no error was recorded.
     */
    const std::string& lastError() const { return m_last_error; }

protected:
    bool parseFileContent(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            m_last_error = "Failed to open file: " + path;
            return false;
        }

        std::ostringstream output;
        output << file.rdbuf();
        return parseString(output.str());
    }

    std::string m_last_error;
};

} // namespace galay::utils

#endif // GALAY_UTILS_PARSER_BASE_HPP
