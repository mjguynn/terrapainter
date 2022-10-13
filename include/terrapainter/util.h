#pragma once

#include <cstdlib>
#include <cstdio>
#include <cassert>

#define _MACRO_TO_STRING(x) #x
#define MACRO_TO_STRING(x) _MACRO_TO_STRING(x)

#if defined(_MSC_VER) // MSVC
    #define UNCHECKED_UNREACHABLE() __assume(false)
    #define DIAG_PUSH_MSVC() __pragma(warning(push))
    #define DIAG_IGNORE_MSVC(__diag) __pragma(warning(disable:__diag))
    #define DIAG_POP_MSVC() __pragma(warning(pop))
    #define DIAG_PUSH_GCC() 
    #define DIAG_IGNORE_GCC(__diag) 
    #define DIAG_POP_GCC()
#elif defined(__GNUC__) // GCC, Clang
    #define UNCHECKED_UNREACHABLE() __builtin_unreachable()
    #define DIAG_PUSH_MSVC()
    #define DIAG_IGNORE_MSVC(__diag)
    #define DIAG_POP_MSVC()
    #define DIAG_PUSH_GCC() _Pragma("GCC diagnostic push")
    #define DIAG_IGNORE_GCC(__diag) _Pragma(MACRO_TO_STRING(GCC diagnostic ignored "-W" __diag )) 
    #define DIAG_POP_GCC() _Pragma("GCC diagnostic pop")
#endif

#define DIAG_PUSHIGNORE_MSVC(__diag) DIAG_PUSH_MSVC(); DIAG_IGNORE_MSVC(__diag)
#define DIAG_PUSHIGNORE_GCC(__diag) DIAG_PUSH_GCC(); DIAG_IGNORE_GCC(__diag)

#ifndef NDEBUG
    #define UNREACHABLE() do {\
        fputs(__FILE__ "(" MACRO_TO_STRING(__LINE__) "): Unreachable code\n", stderr); \
        assert(!"Unreachable code"); \
        abort(); \
    } while(0)
#else 
    #define UNREACHABLE() UNCHECKED_UNREACHABLE()
#endif

#define TODO() do { \
    fputs(__FILE__ "(" MACRO_TO_STRING(__LINE__) "): TODO\n", stderr); \
    assert(!"TODO"); \
    abort(); \
} while(0)


#define error(...) do { fprintf(stderr, "[error] " __VA_ARGS__); exit(-1); } while(0)
#define info(...) fprintf(stderr, "[info] " __VA_ARGS__)