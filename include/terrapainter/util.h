#pragma once

#include <cstdlib>
#include <cstdio>
#include <cassert>

#define _MACRO_TO_STRING(x) #x
#define MACRO_TO_STRING(x) _MACRO_TO_STRING(x)

namespace util {
    /// Declares that a code fragment is unreachable.
    /// Adapted from https://en.cppreference.com/w/cpp/utility/unreachable
    template<typename T = void>
    [[noreturn]]
    inline T unreachable() {
        // Try to catch this on debug builds
        assert(!"Unreachable code");
    #if defined(__GNUC__) // GCC, Clang, ICC
        __builtin_unreachable();
    #elif defined(_MSC_VER) // MSVC
        __assume(false);
    #endif
    
        // On debug builds, we "cleanly" abort the program
    #ifndef NDEBUG
        abort();
    #endif
    }
}

#define TODO() do { \
    fputs(__FILE__ "(" MACRO_TO_STRING(__LINE__) "): TODO\n", stderr); \
    assert(!"TODO"); \
    abort(); \
} while(0)
