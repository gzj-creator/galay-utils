#ifndef GALAY_UTILS_TYPENAME_HPP
#define GALAY_UTILS_TYPENAME_HPP

#include <string>
#include <typeinfo>
#include <cstdlib>

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

namespace galay::utils {

/**
 * @brief Get demangled type name for a given type
 */
template<typename T>
inline std::string getTypeName() {
    const char* name = typeid(T).name();

#if defined(__GNUC__) || defined(__clang__)
    int status = 0;
    char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if (status == 0 && demangled) {
        std::string result(demangled);
        std::free(demangled);
        return result;
    }
#endif

    return std::string(name);
}

/**
 * @brief Get demangled type name for a given object
 */
template<typename T>
inline std::string getTypeName(const T& obj) {
    const char* name = typeid(obj).name();

#if defined(__GNUC__) || defined(__clang__)
    int status = 0;
    char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if (status == 0 && demangled) {
        std::string result(demangled);
        std::free(demangled);
        return result;
    }
#endif

    return std::string(name);
}

/**
 * @brief Demangle a symbol name
 */
inline std::string demangleSymbol(const char* mangledName) {
    if (!mangledName) {
        return "";
    }

#if defined(__GNUC__) || defined(__clang__)
    int status = 0;
    char* demangled = abi::__cxa_demangle(mangledName, nullptr, nullptr, &status);
    if (status == 0 && demangled) {
        std::string result(demangled);
        std::free(demangled);
        return result;
    }
#endif

    return std::string(mangledName);
}

} // namespace galay::utils

#endif // GALAY_UTILS_TYPENAME_HPP
