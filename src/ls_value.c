#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ls_value.h"
#include "string.h"

DEFINE_BUFFER(Value, value, LsValue)

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

bool ls_val_eq(LsValue a, LsValue b) {
  if (ls_val_same(a, b))
    return true;

  // Object are always heap allocated so if one of a or b is not an object
  // they're not equal.
  if (!ls_is_obj(a) || !ls_is_obj(b))
    return false;

  LsObj *objA = ls_val2obj(a);
  LsObj *objB = ls_val2obj(b);

  // Different type of object.
  if (objA->type != objB->type)
    return false;

  switch (objA->type) {
  case LS_OBJ_STRING: {
    LsObjString *strA = (LsObjString *)objA;
    LsObjString *strB = (LsObjString *)objB;
    return strA->length == strB->length &&
           memcmp(strA->value, strB->value, strA->length) == 0;

    break;
  }

  default:
    // All other types are only equal if they are same, which they aren't if
    // we get here.
    return false;
  }
}
