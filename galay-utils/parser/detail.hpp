#ifndef GALAY_UTILS_PARSER_DETAIL_HPP
#define GALAY_UTILS_PARSER_DETAIL_HPP

#include <cctype>
#include <string>
#include <vector>

namespace galay::utils::parser_detail {

inline std::string trim(const std::string& text) {
    size_t start = 0;
    size_t end = text.length();
    while (start < end && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

inline std::string processEscapes(const std::string& text) {
    std::string result;
    result.reserve(text.length());

    for (size_t index = 0; index < text.length(); ++index) {
        if (text[index] == '\\' && index + 1 < text.length()) {
            switch (text[index + 1]) {
                case 'n': result += '\n'; ++index; break;
                case 't': result += '\t'; ++index; break;
                case 'r': result += '\r'; ++index; break;
                case '\\': result += '\\'; ++index; break;
                case '"': result += '"'; ++index; break;
                case '\'': result += '\''; ++index; break;
                default: result += text[index]; break;
            }
        } else {
            result += text[index];
        }
    }

    return result;
}

inline bool isQuoted(const std::string& text) {
    return text.length() >= 2 &&
        ((text.front() == '"' && text.back() == '"') ||
         (text.front() == '\'' && text.back() == '\''));
}

inline std::string unquote(const std::string& text) {
    if (!isQuoted(text)) {
        return text;
    }
    return processEscapes(text.substr(1, text.length() - 2));
}

inline std::vector<std::string> splitCommaSeparated(const std::string& text) {
    if (trim(text).empty()) {
        return {};
    }

    std::vector<std::string> result;
    std::string item;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;

    for (char character : text) {
        if (escaped) {
            item += character;
            escaped = false;
            continue;
        }

        if (character == '\\') {
            item += character;
            escaped = true;
            continue;
        }

        if (character == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            item += character;
            continue;
        }

        if (character == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
            item += character;
            continue;
        }

        if (character == ',' && !in_single_quote && !in_double_quote) {
            result.push_back(trim(item));
            item.clear();
            continue;
        }

        item += character;
    }

    result.push_back(trim(item));
    return result;
}

inline std::string stripInlineComment(const std::string& text, char comment_character) {
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;

    for (size_t index = 0; index < text.length(); ++index) {
        char character = text[index];

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

        if (character == comment_character && !in_single_quote && !in_double_quote) {
            return trim(text.substr(0, index));
        }
    }

    return trim(text);
}

} // namespace galay::utils::parser_detail

#endif // GALAY_UTILS_PARSER_DETAIL_HPP
