#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

#if WREN_DEBUG_GC_STRESS
  // Since collecting calls this function to free things, make sure we don't
  // recurse.
  if (newSize > 0)
    wrenCollectGarbage(vm);
#else
  if (new_size > 0 && vm->bytes_allocated > vm->next_gc)
    ls_collect_garbage(vm);
#endif

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

static void ls_obj_init(LsVM *vm, LsObj *obj, LsObjType type) {
  assert(vm != NULL);
  assert(obj != NULL);

  obj->type = type;
  obj->is_dark = false;
  obj->next = vm->first_obj;
  vm->first_obj = obj;
}

static LsObjString *ls_allocate_string(LsVM *vm, size_t length) {
  LsObjString *str = ls_allocate_flex(vm, LsObjString, char, length + 1);
  // TODO: handle oom.
  ls_obj_init(vm, &str->obj, LS_OBJ_STRING);

  str->length = length;
  // str->value[length] = '\0';

  return str;
}

LsValue ls_new_string_length(LsVM *vm, const char *text, size_t length) {
  LsObjString *str = ls_allocate_string(vm, length);

  if (length > 0 && text != NULL)
    memcpy(str->value, text, length);

  return ls_obj2val(&str->obj);
}

LsValue ls_new_string(LsVM *vm, const char *text) {
  return ls_new_string_length(vm, text, strlen(text));
}
