#ifndef LS_VM_H_INCLUDE
#define LS_VM_H_INCLUDE

#include "ls_value.h"

struct ls_vm {
  LsConfiguration config;

  // The number of bytes that are known to be currently allocated. Includes all
  // memory that was proven live after the last GC, as well as any new bytes
  // that were allocated since then. Does *not* include bytes for objects that
  // were freed since the last GC.
  size_t bytes_allocated;

  // The number of total allocated bytes that will trigger the next GC.
  size_t next_gc;

  // The first object in the linked list of all currently allocated objects.
  // Objects are prepended on allocation.
  LsObj *first_obj;
};

#endif
