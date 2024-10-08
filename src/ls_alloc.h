#include <stddef.h>

#include "lightscript.h"

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
void *ls_reallocate(LsVM *vm, void *memory, size_t old_size, size_t new_size);

// Use the VM's allocator to allocate an object of [type].
#define ls_allocate(vm, type) ls_reallocate((vm), NULL, 0, sizeof(type))
// Use the VM's allocator to allocate an object of [main_type] containing a
// flexible array of [count] objects of [array_type].
#define ls_allocate_flex(vm, main_type, array_type, count)                     \
  ((main_type *)ls_reallocate(                                                 \
      (vm), NULL, 0, sizeof(main_type) + sizeof(array_type) * (count)))

// Use the VM's allocator to allocate an array of [count] elements of [type].
#define ls_allocate_array(vm, type, count)                                     \
  ((type *)ls_reallocate(vm, NULL, 0, sizeof(type) * (count)))

// Free ptr previously allocated using VM's allocator.
#define ls_free(vm, ptr) ls_reallocate((vm), ptr, sizeof(*ptr), 0)
