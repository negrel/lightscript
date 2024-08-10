#ifndef LS_UTILS_H_INCLUDE
#define LS_UTILS_H_INCLUDE

#include <stdio.h>

#ifdef DEBUG

// Indicates that we know execution should never reach this point in the
// program. In debug mode, we assert this fact because it's a bug to get here.
//
// In release mode, we use compiler-specific built in functions to tell the
// compiler the code can't be reached. This avoids "missing return" warnings
// in some cases and also lets it perform some optimizations by assuming the
// code is never reached.
#define UNREACHABLE()                                                          \
  do {                                                                         \
    fprintf(stderr, "[%s:%d] This code should not be reached in %s()\n",       \
            __FILE__, __LINE__, __func__);                                     \
    abort();                                                                   \
  } while (false)

#else

// Tell the compiler that this part of the code will never be reached.
#if defined(_MSC_VER)
#define UNREACHABLE() __assume(0)
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define UNREACHABLE() __builtin_unreachable()
#elif defined(__clang__)
#define UNREACHABLE() __builtin_unreachable()
#else
#define UNREACHABLE()
#endif

#endif // DEBUG

#endif
