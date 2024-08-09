#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ls_value.h"

inline LsValue ls_obj2val(LsObj *obj) {
  // The triple casting is necessary here to satisfy some compilers:
  // 1. (uintptr_t) Convert the pointer to a number of the right size.
  // 2. (uint64_t)  Pad it up to 64 bits in 32-bit builds.
  // 3. Or in the bits to make a tagged Nan.
  // 4. Cast to a typedef'd value.
  return (LsValue)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj));
}

inline LsObj *ls_val2obj(LsValue value) {
  assert(ls_is_obj(value));
  return ((LsObj *)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)));
}
