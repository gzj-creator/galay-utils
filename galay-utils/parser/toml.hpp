#ifndef GALAY_UTILS_PARSER_TOML_HPP
#define GALAY_UTILS_PARSER_TOML_HPP

#include "galay-utils/parser/detail.hpp"
#include "galay-utils/parser/parser_base.hpp"
#include <cctype>
#include <unordered_map>
#include <unordered_set>

namespace galay::utils {

/**
 * @brief Lightweight TOML parser for common configuration files.
 *
 * Supports key-value pairs, `[section]` headers, dotted keys, strings, booleans,
 * numbers, and single-line arrays. Parsed values are normalized into strings;
 * arrays are stored as comma-separated normalized values and can be retrieved
 * with getArray(). This parser intentionally does not implement TOML tables of
 * arrays, multiline strings, dates, or full TOML validation.
 */
class TomlParser : public ParserBase {
public:
    TomlParser() = default;

    bool parseFile(const std::string& path) override {
        return parseFileContent(path);
    }

    bool parseString(const std::string& content) override {
        m_values.clear();
        m_sections.clear();
        m_last_error.clear();
        std::string current_section;

        std::istringstream input(content);
        std::string line;
        int line_num = 0;

        while (std::getline(input, line)) {
            ++line_num;
            line = parser_detail::stripInlineComment(line, '#');

            if (line.empty()) {
                continue;
            }

            if (line.front() == '[' && line.back() == ']') {
                if (line.length() < 3 || line[1] == '[' || line[line.length() - 2] == ']') {
                    m_last_error = "Unsupported TOML table line " + std::to_string(line_num) + ": " + line;
                    return false;
                }
                current_section = parser_detail::trim(line.substr(1, line.length() - 2));
                if (!isValidKey(current_section)) {
                    m_last_error = "Invalid TOML section at line " + std::to_string(line_num);
                    return false;
                }
                if (m_sections.find(current_section) != m_sections.end()) {
                    m_last_error = "Duplicate TOML section at line " + std::to_string(line_num) + ": " + current_section;
                    return false;
                }
                if (hasKeyConflict(current_section)) {
                    m_last_error = "TOML section conflicts with key at line " + std::to_string(line_num) + ": " + current_section;
                    return false;
                }
                m_sections.insert(current_section);
                continue;
            }

            auto equal_pos = line.find('=');
            if (equal_pos == std::string::npos) {
                m_last_error = "Invalid TOML line " + std::to_string(line_num) + ": " + line;
                return false;
            }

            std::string key = parser_detail::trim(line.substr(0, equal_pos));
            std::string value = normalizeValue(parser_detail::trim(line.substr(equal_pos + 1)), line_num);
            if (!m_last_error.empty()) {
                return false;
            }

            if (key.empty()) {
                m_last_error = "Empty TOML key at line " + std::to_string(line_num);
                return false;
            }
            if (!isValidKey(key)) {
                m_last_error = "Invalid TOML key at line " + std::to_string(line_num) + ": " + key;
                return false;
            }

            std::string full_key = current_section.empty() ? key : current_section + "." + key;
            if (m_values.find(full_key) != m_values.end()) {
                m_last_error = "Duplicate TOML key at line " + std::to_string(line_num) + ": " + full_key;
                return false;
            }
            if (hasKeyConflict(full_key)) {
                m_last_error = "TOML key conflicts with existing key at line " + std::to_string(line_num) + ": " + full_key;
                return false;
            }
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

    std::vector<std::string> getArray(const std::string& key) const {
        auto value = getValue(key);
        if (!value) {
            return {};
        }
        return parser_detail::splitCommaSeparated(*value);
    }

private:
    std::string normalizeValue(const std::string& raw_value, int line_num) {
        if (raw_value.empty()) {
            m_last_error = "Empty TOML value at line " + std::to_string(line_num);
            return {};
        }

        if (raw_value.front() == '[') {
            if (raw_value.back() != ']') {
                m_last_error = "Invalid TOML array at line " + std::to_string(line_num);
                return {};
            }
            std::string array_body = raw_value.substr(1, raw_value.length() - 2);
            if (parser_detail::trim(array_body).empty()) {
                return {};
            }
            if (hasNestedArray(array_body)) {
                m_last_error = "Unsupported nested TOML array at line " + std::to_string(line_num);
                return {};
            }
            auto items = parser_detail::splitCommaSeparated(array_body);
            ScalarKind array_kind = ScalarKind::Unknown;
            std::string result;
            for (size_t index = 0; index < items.size(); ++index) {
                ScalarKind item_kind = scalarKind(items[index]);
                if (item_kind == ScalarKind::Invalid) {
                    m_last_error = "Invalid TOML array item at line " + std::to_string(line_num);
                    return {};
                }
                if (array_kind == ScalarKind::Unknown) {
                    array_kind = item_kind;
                } else if (array_kind != item_kind) {
                    m_last_error = "Mixed TOML array item types at line " + std::to_string(line_num);
                    return {};
                }
                if (index > 0) {
                    result += ",";
                }
                result += normalizeScalar(items[index], line_num);
                if (!m_last_error.empty()) {
                    return {};
                }
            }
            return result;
        }

        return normalizeScalar(raw_value, line_num);
    }

    std::string normalizeScalar(const std::string& raw_value, int line_num) {
        std::string value = parser_detail::trim(raw_value);
        if (value.empty()) {
            m_last_error = "Empty TOML scalar at line " + std::to_string(line_num);
            return {};
        }
        if (value.front() == '"' || value.front() == '\'') {
            if (!parser_detail::isQuoted(value)) {
                m_last_error = "Unterminated TOML string at line " + std::to_string(line_num);
                return {};
            }
            if (value.front() == '"' && !hasValidEscapes(value)) {
                m_last_error = "Invalid TOML escape at line " + std::to_string(line_num);
                return {};
            }
            return parser_detail::unquote(value);
        }
        if (parser_detail::isQuoted(value)) {
            return parser_detail::unquote(value);
        }
        if (value.front() == '{' || value.back() == '}') {
            m_last_error = "Unsupported TOML inline table at line " + std::to_string(line_num);
            return {};
        }
        if (value.find('"') != std::string::npos || value.find('\'') != std::string::npos) {
            m_last_error = "Invalid TOML string at line " + std::to_string(line_num);
            return {};
        }
        if (value == "true" || value == "false" || isNumber(value)) {
            return value;
        }
        m_last_error = "Unsupported TOML scalar at line " + std::to_string(line_num) + ": " + value;
        return {};
    }

    enum class ScalarKind {
        Unknown,
        String,
        Bool,
        Number,
        Invalid
    };

    ScalarKind scalarKind(const std::string& raw_value) const {
        std::string value = parser_detail::trim(raw_value);
        if (value.empty() || value.front() == '[' || value.front() == '{' || value.back() == '}') {
            return ScalarKind::Invalid;
        }
        if (value.front() == '"' || value.front() == '\'') {
            if (!parser_detail::isQuoted(value)) {
                return ScalarKind::Invalid;
            }
            if (value.front() == '"' && !hasValidEscapes(value)) {
                return ScalarKind::Invalid;
            }
            return ScalarKind::String;
        }
        if (value.find('"') != std::string::npos || value.find('\'') != std::string::npos) {
            return ScalarKind::Invalid;
        }
        if (value == "true" || value == "false") {
            return ScalarKind::Bool;
        }
        if (isNumber(value)) {
            return ScalarKind::Number;
        }
        return ScalarKind::Invalid;
    }

    static bool isValidKey(const std::string& key) {
        if (key.empty() || key.front() == '.' || key.back() == '.') {
            return false;
        }

        bool previous_dot = false;
        for (char character : key) {
            if (character == '.') {
                if (previous_dot) {
                    return false;
                }
                previous_dot = true;
                continue;
            }

            previous_dot = false;
            unsigned char byte = static_cast<unsigned char>(character);
            if (!std::isalnum(byte) && character != '_' && character != '-') {
                return false;
            }
        }
        return true;
    }

    static bool isNumber(const std::string& value) {
        size_t index = 0;
        if (value[index] == '+' || value[index] == '-') {
            ++index;
        }
        if (index >= value.length()) {
            return false;
        }
        if (index + 1 < value.length() && value[index] == '0' && std::isdigit(static_cast<unsigned char>(value[index + 1]))) {
            return false;
        }

        bool has_digit = false;
        while (index < value.length() && std::isdigit(static_cast<unsigned char>(value[index]))) {
            has_digit = true;
            ++index;
        }

        if (index < value.length() && value[index] == '.') {
            ++index;
            bool has_fraction_digit = false;
            while (index < value.length() && std::isdigit(static_cast<unsigned char>(value[index]))) {
                has_fraction_digit = true;
                ++index;
            }
            if (!has_fraction_digit) {
                return false;
            }
        }

        return has_digit && index == value.length();
    }

    static bool hasValidEscapes(const std::string& value) {
        for (size_t index = 1; index + 1 < value.length(); ++index) {
            if (value[index] != '\\') {
                continue;
            }

            char escaped = value[++index];
            switch (escaped) {
                case 'b':
                case 't':
                case 'n':
                case 'f':
                case 'r':
                case '"':
                case '\\':
                    break;
                case 'u':
                    if (!hasHexDigits(value, index + 1, 4)) {
                        return false;
                    }
                    index += 4;
                    break;
                case 'U':
                    if (!hasHexDigits(value, index + 1, 8)) {
                        return false;
                    }
                    index += 8;
                    break;
                default:
                    return false;
            }
        }
        return true;
    }

    static bool hasHexDigits(const std::string& value, size_t start, size_t count) {
        if (start + count > value.length() - 1) {
            return false;
        }
        for (size_t index = start; index < start + count; ++index) {
            if (!std::isxdigit(static_cast<unsigned char>(value[index]))) {
                return false;
            }
        }
        return true;
    }

    bool hasKeyConflict(const std::string& key) const {
        for (const auto& entry : m_values) {
            const std::string& existing_key = entry.first;
            if (existing_key == key) {
                return true;
            }
            if (existing_key.length() > key.length() &&
                existing_key.compare(0, key.length(), key) == 0 &&
                existing_key[key.length()] == '.') {
                return true;
            }
            if (key.length() > existing_key.length() &&
                key.compare(0, existing_key.length(), existing_key) == 0 &&
                key[existing_key.length()] == '.') {
                return true;
            }
        }
        return false;
    }

    static bool hasNestedArray(const std::string& value) {
        bool in_single_quote = false;
        bool in_double_quote = false;
        bool escaped = false;
        for (char character : value) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (character == '\\') {
                escaped = true;
                continue;
            }
            if (character == '\'' && !in_double_quote) {
                in_single_quote = !in_single_quote;
                continue;
            }
            if (character == '"' && !in_single_quote) {
                in_double_quote = !in_double_quote;
                continue;
            }
            if ((character == '[' || character == ']') && !in_single_quote && !in_double_quote) {
                return true;
            }
        }
        if (in_single_quote || in_double_quote) {
            return true;
        }
        return false;
    }

    std::unordered_map<std::string, std::string> m_values;
    std::unordered_set<std::string> m_sections;
};

} // namespace galay::utils

#endif // GALAY_UTILS_PARSER_TOML_HPP
