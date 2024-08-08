#ifndef LIGHT_VM_H_INCLUDE
#define LIGHT_VM_H_INCLUDE

#include "lightscript.h"

#define ls_allocate(vm, type) ls_reallocate((vm), NULL, 0, sizeof(type))
#define ls_free(vm, ptr) ls_reallocate((vm), ptr, sizeof(*ptr), 0)

struct LsVM {
  LsConfiguration config;

  // The number of bytes that are known to be currently allocated. Includes all
  // memory that was proven live after the last GC, as well as any new bytes
  // that were allocated since then. Does *not* include bytes for objects that
  // were freed since the last GC.
  size_t bytes_allocated;

  // The number of total allocated bytes that will trigger the next GC.
  size_t next_gc;
};

// A generic allocation function that handles all explicit memory management.
// It's used like so:
//
// - To allocate new memory, [memory] is NULL and [old_size] is zero. It should
//   return the allocated memory or NULL on failure.
//
// - To attempt to grow an existing allocation, [memory] is the memory,
//   [old_size] is its previous size, and [new_size] is the desired size.
//   It should return [memory] if it was able to grow it in place, or a new
//   pointer if it had to move it.
//
// - To shrink memory, [memory], [old_size], and [new_size] are the same as
//   above but it will always return [memory].
//
// - To free memory, [memory] will be the memory to free and [new_size] will be
//   zero. It should return NULL.
void *ls_reallocate(struct LsVM *vm, void *memory, size_t old_size,
                    size_t new_size);

#endif
