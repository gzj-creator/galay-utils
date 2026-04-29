#ifndef GALAY_UTILS_PARSER_INI_HPP
#define GALAY_UTILS_PARSER_INI_HPP

#include "galay-utils/parser/config.hpp"

namespace galay::utils {

/**
 * @brief INI parser alias with a dedicated public type.
 *
 * INI files currently share the same behavior as ConfigParser: section headers
 * flatten keys into `section.key`, and values are stored as strings.
 */
class IniParser : public ConfigParser {
public:
    IniParser() = default;
};

} // namespace galay::utils

#endif // GALAY_UTILS_PARSER_INI_HPP
