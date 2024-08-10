#ifndef LS_BUFFER_H_INCLUDE
#define LS_BUFFER_H_INCLUDE

#include "ls_alloc.h"

// We need buffers of a few different types. To avoid lots of casting between
// void * and back, we'll use the preprocessor as a poor man's generics and let
// it generate a few type-specific ones.
#define DECLARE_BUFFER(Name, name, type)                                       \
  typedef struct {                                                             \
    size_t length;                                                             \
    size_t capacity;                                                           \
    type *data;                                                                \
  } Name##Buffer;                                                              \
  void ls_##name##_buffer_init(Name##Buffer *buffer);                          \
  void ls_##name##_buffer_clear(LsVM *vm, Name##Buffer *buffer);               \
  void ls_##name##_buffer_fill(LsVM *vm, Name##Buffer *buffer, type data,      \
                               size_t count);                                  \
  void ls_##name##_buffer_write(LsVM *vm, Name##Buffer *buffer, type data)

// This should be used once for each type instantiation, somewhere in a .c file.
#define DEFINE_BUFFER(Name, name, type)                                        \
  void ls_##name##_buffer_init(Name##Buffer *buffer) {                         \
    buffer->data = NULL;                                                       \
    buffer->length = 0;                                                        \
    buffer->capacity = 0;                                                      \
  }                                                                            \
                                                                               \
  void ls_##name##_buffer_clear(LsVM *vm, Name##Buffer *buffer) {              \
    ls_reallocate(vm, buffer->data, 0, 0);                                     \
    ls_##name##_buffer_init(buffer);                                           \
  }                                                                            \
                                                                               \
  void ls_##name##_buffer_fill(LsVM *vm, Name##Buffer *buffer, type data,      \
                               size_t length) {                                \
    size_t capacity = buffer->capacity;                                        \
    while (buffer->capacity < buffer->length + length) {                       \
      buffer->capacity = (buffer->capacity == 0 ? 1 : buffer->capacity) * 2;   \
    }                                                                          \
    buffer->data =                                                             \
        (type *)ls_reallocate(vm, buffer->data, capacity * sizeof(type),       \
                              buffer->capacity * sizeof(type));                \
                                                                               \
    for (size_t i = 0; i < length; i++) {                                      \
      buffer->data[buffer->length++] = data;                                   \
    }                                                                          \
  }                                                                            \
                                                                               \
  void ls_##name##_buffer_write(LsVM *vm, Name##Buffer *buffer, type data) {   \
    ls_##name##_buffer_fill(vm, buffer, data, 1);                              \
  }

#endif
