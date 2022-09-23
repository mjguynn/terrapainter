#pragma once

#include <cstdlib>
#include <cassert>

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

    template<typename T = void>
    [[noreturn]]
    inline T todo() {
        assert(!"TODO");
        abort();
    }
}