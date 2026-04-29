#ifndef GALAY_UTILS_PARSER_CONFIG_HPP
#define GALAY_UTILS_PARSER_CONFIG_HPP

#include "galay-utils/parser/detail.hpp"
#include "galay-utils/parser/parser_base.hpp"
#include <unordered_map>

namespace galay::utils {

/**
 * @brief Parser for simple `.conf` key-value files with optional sections.
 *
 * Lines use `key = value`; `[section]` prefixes following keys as
 * `section.key`. `#` and `;` begin full-line comments. Parsed values are stored
 * as strings, with matching single or double quotes removed.
 */
class ConfigParser : public ParserBase {
public:
    ConfigParser() = default;

    bool parseFile(const std::string& path) override {
        return parseFileContent(path);
    }

    bool parseString(const std::string& content) override {
        m_values.clear();
        m_last_error.clear();
        std::string current_section;

        std::istringstream input(content);
        std::string line;
        int line_num = 0;

        while (std::getline(input, line)) {
            ++line_num;
            line = parser_detail::trim(line);

            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            if (line[0] == '[' && line.back() == ']') {
                current_section = parser_detail::trim(line.substr(1, line.length() - 2));
                continue;
            }

            auto equal_pos = line.find('=');
            if (equal_pos == std::string::npos) {
                m_last_error = "Invalid line " + std::to_string(line_num) + ": " + line;
                return false;
            }

            std::string key = parser_detail::trim(line.substr(0, equal_pos));
            std::string value = parser_detail::trim(line.substr(equal_pos + 1));
            value = parser_detail::unquote(value);

            std::string full_key = current_section.empty() ? key : current_section + "." + key;
            m_values[full_key] = value;
        }

        return true;
    }

    std::optional<std::string> getValue(const std::string& key) const override {
        auto iter = m_values.find(key);
        if (iter != m_values.end()) {
            return iter->second;
        }
        return std::nullopt;
    }

    bool hasKey(const std::string& key) const override {
        return m_values.find(key) != m_values.end();
    }

    std::vector<std::string> getKeys() const override {
        std::vector<std::string> keys;
        keys.reserve(m_values.size());
        for (const auto& entry : m_values) {
            keys.push_back(entry.first);
        }
        return keys;
    }

    std::vector<std::string> getKeysInSection(const std::string& section) const {
        std::vector<std::string> keys;
        std::string prefix = section + ".";
        for (const auto& entry : m_values) {
            if (entry.first.substr(0, prefix.length()) == prefix) {
                keys.push_back(entry.first.substr(prefix.length()));
            }
        }
        return keys;
    }

    std::vector<std::string> getArray(const std::string& key) const {
        auto value = getValue(key);
        if (!value) {
            return {};
        }
        return parser_detail::splitCommaSeparated(*value);
    }

protected:
    std::unordered_map<std::string, std::string> m_values;
};

} // namespace galay::utils

#endif // GALAY_UTILS_PARSER_CONFIG_HPP
