#ifndef GALAY_UTILS_PARSER_MANAGER_HPP
#define GALAY_UTILS_PARSER_MANAGER_HPP

#include "galay-utils/parser/config.hpp"
#include "galay-utils/parser/env.hpp"
#include "galay-utils/parser/ini.hpp"
#include "galay-utils/parser/toml.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace galay::utils {

/**
 * @brief Registry and factory for all parser types.
 *
 * The manager maps file extensions to parser factories. It registers `.conf`,
 * `.ini`, `.env`, and `.toml` by default. The singleton registry is initialized
 * on first use; concurrent mutation through registerParser() must be externally
 * synchronized by callers.
 */
class ParserManager {
public:
    using Creator = std::function<std::unique_ptr<ParserBase>()>;

    static ParserManager& instance() {
        static ParserManager instance;
        return instance;
    }

    /**
     * @brief Register or replace a parser factory for an extension.
     * @param extension Extension including the leading dot, such as `.toml`.
     * @param creator Factory that returns a new parser instance.
     */
    void registerParser(const std::string& extension, Creator creator) {
        m_creators[extension] = std::move(creator);
    }

    /**
     * @brief Create a parser based on the extension of a path.
     * @param path File path or file name.
     * @return Parser instance, or nullptr when the extension is unknown.
     */
    std::unique_ptr<ParserBase> createParser(const std::string& path) {
        auto dot_pos = path.rfind('.');
        if (dot_pos == std::string::npos) {
            return nullptr;
        }

        std::string extension = path.substr(dot_pos);
        auto iter = m_creators.find(extension);
        if (iter != m_creators.end()) {
            return iter->second();
        }

        return nullptr;
    }

private:
    ParserManager() {
        registerParser(".conf", []() { return std::make_unique<ConfigParser>(); });
        registerParser(".ini", []() { return std::make_unique<IniParser>(); });
        registerParser(".env", []() { return std::make_unique<EnvParser>(); });
        registerParser(".toml", []() { return std::make_unique<TomlParser>(); });
    }

    std::unordered_map<std::string, Creator> m_creators;
};

} // namespace galay::utils

#endif // GALAY_UTILS_PARSER_MANAGER_HPP
