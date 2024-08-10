#include <stdlib.h>
#include <string.h>

#include "ls_value.h"
#include "ls_vm.h"

// The behavior of realloc() when the size is 0 is implementation defined. It
// may return a non-NULL pointer which must not be dereferenced but nevertheless
// should be freed. To prevent that, we avoid calling realloc() with a zero
// size.
static void *default_reallocate(void *ptr, size_t new_size) {
  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  return realloc(ptr, new_size);
}

void ls_collect_garbage(LsVM *vm) { (void)vm; }

void *ls_reallocate(LsVM *vm, void *memory, size_t old_size, size_t new_size) {
  // If new bytes are being allocated, add them to the total count. If objects
  // are being completely deallocated, we don't track that (since we don't
  // track the original size). Instead, that will be handled while marking
  // during the next GC.
  vm->bytes_allocated += new_size - old_size;

  if (new_size > 0 && vm->bytes_allocated > vm->next_gc)
    ls_collect_garbage(vm);

  return vm->config.reallocate(memory, new_size);
}

LsVM *ls_new_vm(LsConfiguration *config) {
  LsReallocateFn reallocate = default_reallocate;

  if (config != NULL) {
    if (config->reallocate != NULL)
      reallocate = config->reallocate;
  }

  LsVM *vm = reallocate(NULL, sizeof(LsVM));
  memset(vm, 0, sizeof(LsVM));

  if (config != NULL) {
    memcpy(&vm->config, config, sizeof(*config));
  } else {
    vm->config.write = NULL;
    vm->config.on_error = NULL;
    vm->config.initial_heap_size = 10 * 1024 * 1024;
    vm->config.min_heap_size = 1024 * 1024;
    vm->config.heap_growth_percent = 50;
  }
  vm->config.reallocate = reallocate;

  return vm;
}

void ls_free_vm(LsVM *vm) { ls_reallocate(vm, vm, 0, 0); }
