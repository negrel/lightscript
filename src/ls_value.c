#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "ls_value.h"
#include "ls_vm.h"
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

  case LS_OBJ_ARRAY: {
    LsObjArray *arrA = (LsObjArray *)objA;
    LsObjArray *arrB = (LsObjArray *)objB;
    return arrA->elements.length == arrB->elements.length &&
           memcmp(arrA->elements.data, arrB->elements.data,
                  arrA->elements.length) == 0;
  }

  default:
    // All other types are only equal if they are same, which they aren't if
    // we get here.
    return false;
  }
}

static void ls_obj_init(LsVM *vm, LsObj *obj, LsObjType type) {
  assert(vm != NULL);
  assert(obj != NULL);

  obj->type = type;
  obj->is_dark = false;
  obj->next = vm->first_obj;
  vm->first_obj = obj;
}

void ls_free_obj(LsVM *vm, LsObj *obj) {
  assert(vm != NULL);
  assert(obj != NULL);

  switch (obj->type) {
  case LS_OBJ_ARRAY: {
    LsObjArray *arr = (LsObjArray *)obj;
    ls_value_buffer_clear(vm, &arr->elements);
    break;
  }

  default:
    break;
  }

  ls_free(vm, obj);
}

static LsObjString *ls_allocate_string(LsVM *vm, size_t length) {
  LsObjString *str = ls_allocate_flex(vm, LsObjString, char, length + 1);
  // TODO: handle oom.
  ls_obj_init(vm, &str->obj, LS_OBJ_STRING);

  str->length = length;
  str->value[length] = '\0';

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

LsValue ls_new_array(LsVM *vm, size_t initial_length) {
  LsObjArray *arr = ls_allocate(vm, LsObjArray);
  ls_obj_init(vm, &arr->obj, LS_OBJ_ARRAY);
  ls_value_buffer_init(&arr->elements);
  ls_value_buffer_fill(vm, &arr->elements, LS_NULL, initial_length);

  return ls_obj2val(&arr->obj);
}
