#ifndef LIGHTSCRIPT_H_INCLUDE
#define LIGHTSCRIPT_H_INCLUDE

#include <stddef.h>

// The LightScript semantic version number components.
#define LS_VERSION_MAJOR 0
#define LS_VERSION_MINOR 1
#define LS_VERSION_PATCH 0

#ifndef LS_API
#if defined(_MSC_VER) && defined(LS_API_DLLEXPORT)
#define LS_API __declspec(dllexport)
#else
#define LS_API
#endif
#endif // LS_API

// A single virtual machine for executing LightScript code.
//
// LightScript has no global state, so all state stored by a running interpreter
// lives here.
typedef struct ls_vm LsVM;

// A generic allocation function that handles all explicit memory management
// used by LightScript. It's used like so:
//
// - To allocate new memory, [memory] is NULL and [new_size] is the desired
//   size. It should return the allocated memory or NULL on failure.
//
// - To attempt to grow an existing allocation, [memory] is the memory, and
//   [new_size] is the desired size. It should return [memory] if it was able to
//   grow it in place, or a new pointer if it had to move it.
//
// - To shrink memory, [memory] and [new_size] are the same as above but it will
//   always return [memory].
//
// - To free memory, [memory] will be the memory to free and [new_size] will be
//   zero. It should return NULL.
typedef void *(*LsReallocateFn)(void *memory, size_t new_size);

// Displays a string of text to the user.
typedef void (*LsWriteFn)(LsVM *vm, const char *text);

typedef enum {
  // A syntax or resolution error detected at compile time.
  LS_ERROR_COMPILE,

  // The error message for a runtime error.
  LS_ERROR_RUNTIME,

  // One entry of a runtime error's stack trace.
  LS_ERROR_STACK_TRACE
} LsErrorType;

// Reports an error to the user.
//
// An error detected during compile time is reported by calling this once with
// [type] `LS_ERROR_COMPILE`, the resolved name of the [module] and [line]
// where the error occurs, and the compiler's error [message].
//
// A runtime error is reported by calling this once with [type]
// `LS_ERROR_RUNTIME`, no [module] or [line], and the runtime error's
// [message]. After that, a series of [type] `LS_ERROR_STACK_TRACE` calls are
// made for each line in the stack trace. Each of those has the resolved
// [module] and [line] where the method or function is defined and [message] is
// the name of the method or function.
typedef void (*LsErrorFn)(LsVM *vm, LsErrorType type, const char *module,
                          int line, const char *message);

typedef struct {
  // The callback LightScript will use to allocate, reallocate, and deallocate
  // memory.
  //
  // If `NULL`, defaults to a built-in function that uses `realloc` and `free`.
  LsReallocateFn reallocate;

  // The callback LightScript uses to display text when `System.print()` or the
  // other related functions are called.
  //
  // If this is `NULL`, LightScript discards any printed text.
  LsWriteFn write;

  // The callback LightScript uses to report errors.
  //
  // When an error occurs, this will be called with the module name, line
  // number, and an error message. If this is `NULL`, LightScript doesn't report
  // any errors.
  LsErrorFn on_error;

  // The number of bytes LightScript will allocate before triggering the first
  // garbage collection.
  //
  // If zero, defaults to 10MB.
  size_t initial_heap_size;

  // After a collection occurs, the threshold for the next collection is
  // determined based on the number of bytes remaining in use. This allows
  // LightScript to shrink its memory usage automatically after reclaiming a
  // large amount of memory.
  //
  // This can be used to ensure that the heap does not get too small, which can
  // in turn lead to a large number of collections afterwards as the heap grows
  // back to a usable size.
  //
  // If zero, defaults to 1MB.
  size_t min_heap_size;

  // LightScript will resize the heap automatically as the number of bytes
  // remaining in use after a collection changes. This number determines the
  // amount of additional memory LightScript will use after a collection, as a
  // percentage of the current heap size.
  //
  // For example, say that this is 50. After a garbage collection, when there
  // are 400 bytes of memory still in use, the next collection will be triggered
  // after a total of 600 bytes are allocated (including the 400 already in
  // use.)
  //
  // Setting this to a smaller number wastes less memory, but triggers more
  // frequent garbage collections.
  //
  // If zero, defaults to 50.
  int heap_growth_percent;

  // User-defined data associated with the VM.
  void *user_data;
} LsConfiguration;

// Creates a new LightScript virtual machine using the given [configuration].
// Wren will copy the configuration data, so the argument passed to this can be
// freed after calling this. If [configuration] is `NULL`, uses a default
// configuration.
LsVM *ls_new_vm(LsConfiguration *config);

// Disposes of all resources is use by [vm], which was previously created by a
// call to [ls_new_vm].
void ls_free_vm(LsVM *vm);

// Immediately run the garbage collector to free unused memory.
void ls_collect_garbage(LsVM *vm);

#endif
