#ifndef GALAY_UTILS_DEFN_HPP
#define GALAY_UTILS_DEFN_HPP

#include <cstdint>
#include <cstddef>
#include <memory>
#include <functional>
#include <string>
#include <string_view>

// Platform detection
#if defined(__APPLE__)
    #define GALAY_PLATFORM_MACOS 1
#elif defined(__linux__)
    #define GALAY_PLATFORM_LINUX 1
#elif defined(_WIN32) || defined(_WIN64)
    #define GALAY_PLATFORM_WINDOWS 1
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define GALAY_ARCH_X64 1
#elif defined(__i386__) || defined(_M_IX86)
    #define GALAY_ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define GALAY_ARCH_ARM64 1
#endif

// Compiler detection
#if defined(__clang__)
    #define GALAY_COMPILER_CLANG 1
#elif defined(__GNUC__)
    #define GALAY_COMPILER_GCC 1
#elif defined(_MSC_VER)
    #define GALAY_COMPILER_MSVC 1
#endif

// Likely/Unlikely hints
#if defined(GALAY_COMPILER_GCC) || defined(GALAY_COMPILER_CLANG)
    #define GALAY_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define GALAY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define GALAY_LIKELY(x)   (x)
    #define GALAY_UNLIKELY(x) (x)
#endif

// Force inline
#if defined(GALAY_COMPILER_MSVC)
    #define GALAY_FORCE_INLINE __forceinline
#else
    #define GALAY_FORCE_INLINE __attribute__((always_inline)) inline
#endif

// Unused variable
#define GALAY_UNUSED(x) (void)(x)

namespace galay::utils {

// Common type aliases
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using usize = size_t;
using isize = ptrdiff_t;

// Smart pointer aliases
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// Function alias
template<typename T>
using Func = std::function<T>;

// String aliases
using String = std::string;
using StringView = std::string_view;

// Non-copyable base class
class NonCopyable {
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

// Non-movable base class
class NonMovable {
protected:
    NonMovable() = default;
    ~NonMovable() = default;

    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

// Singleton base class
template<typename T>
class Singleton : public NonCopyable, public NonMovable {
public:
    static T& instance() {
        static T instance;
        return instance;
    }

protected:
    Singleton() = default;
    ~Singleton() = default;
};

} // namespace galay::utils

#endif // GALAY_UTILS_DEFN_HPP
