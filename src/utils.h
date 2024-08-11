#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define log(lvl, ...)                                                          \
  do {                                                                         \
    fprintf(stderr,                                                            \
            "LightScript[" lvl "] %s ("__FILE__                                \
            ":%d) - ",                                                         \
            __func__, __LINE__);                                               \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)

#ifdef DEBUG

#define LS_ASSERT(condition, message)                                          \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "[%s:%d] assertion failed in %s(): %s\n", __FILE__,      \
              __LINE__, __func__, message);                                    \
      abort();                                                                 \
    }                                                                          \
  } while (0)

// Indicates that we know execution should never reach this point in the
// program. In debug mode, we assert this fact because it's a bug to get here.
//
// In release mode, we use compiler-specific built in functions to tell the
// compiler the code can't be reached. This avoids "missing return" warnings
// in some cases and also lets it perform some optimizations by assuming the
// code is never reached.
#define unreachable                                                            \
  do {                                                                         \
    fprintf(stderr, "[%s:%d] This code should not be reached in %s()\n",       \
            __FILE__, __LINE__, __func__);                                     \
    abort();                                                                   \
  } while (0)

#define log_debug(...) log("DEBUG", __VA_ARGS__)

#else

#if defined(_MSC_VER)
#define unreachable __assume(0)
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define unreachable __builtin_unreachable()
#elif defined(__clang__)
#define unreachable __builtin_unreachable()
#else
#define unreachable
#endif

#define log_debug(...)

#endif // DEBUG

#endif // UTILS_H
