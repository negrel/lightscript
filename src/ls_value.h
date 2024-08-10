#ifndef LS_VALUE_H_INCLUDE
#define LS_VALUE_H_INCLUDE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ls_buffer.h"

// A mask that selects the sign bit.
#define SIGN_BIT ((uint64_t)1 << 63)

// The bits that must be set to indicate a quiet NaN.
#define QNAN ((uint64_t)0x7ffc000000000000)

// Masks out the tag bits used to identify the singleton value.
#define MASK_TAG (7)

// Tag values for the different singleton values.
#define LS_TAG_NAN (0)
#define LS_TAG_NULL (1)
#define LS_TAG_FALSE (2)
#define LS_TAG_TRUE (3)

#define LS_NULL ((LsValue)(uint64_t)(QNAN | LS_TAG_NULL))
#define LS_FALSE ((LsValue)(uint64_t)(QNAN | LS_TAG_FALSE))
#define LS_TRUE ((LsValue)(uint64_t)(QNAN | LS_TAG_TRUE))

// An object pointer is a NaN with a set sign bit.
#define ls_is_obj(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#define ls_is_str(value)                                                       \
  (ls_is_obj(value) && ls_val2obj(value)->type == LS_OBJ_STRING)

// Identifies which specific type a heap-allocated object is.
typedef enum {
  LS_OBJ_STRING,
  LS_OBJ_ARRAY,
  LS_OBJ_MAP,
  LS_OBJ_TYPE_COUNT, // Must be last.
} LsObjType;

// Base struct for all heap allocated objects.
typedef struct ls_obj {
  LsObjType type;

  // Marked as dark by GC.
  bool is_dark;

  // The next object in the linked list of all currently allocated objects.
  struct ls_obj *next;
} LsObj;

// Releases all memory owned by [obj], including [obj] itself.
void ls_free_obj(LsVM *vm, LsObj *obj);

// NaN boxed value.
// An IEEE 754 double-precision float is a 64-bit value with bits laid out
// like:
//
// 1 Sign bit
// | 11 Exponent bits
// | |          52 Mantissa (i.e. fraction) bits
// | |          |
// S[Exponent-][Mantissa------------------------------------------]
//
// The details of how these are used to represent numbers aren't really
// relevant here as long we don't interfere with them. The important bit is NaN.
//
// An IEEE double can represent a few magical values like NaN ("not a number"),
// Infinity, and -Infinity. A NaN is any value where all exponent bits are set:
//
//  v--NaN bits
// -11111111111----------------------------------------------------
//
// Here, "-" means "doesn't matter". Any bit sequence that matches the above is
// a NaN. With all of those "-", it obvious there are a *lot* of different
// bit patterns that all mean the same thing. NaN tagging takes advantage of
// this. We'll use those available bit patterns to represent things other than
// numbers without giving up any valid numeric values.
//
// NaN values come in two flavors: "signalling" and "quiet". The former are
// intended to halt execution, while the latter just flow through arithmetic
// operations silently. We want the latter. Quiet NaNs are indicated by setting
// the highest mantissa bit:
//
//             v--Highest mantissa bit
// -[NaN      ]1---------------------------------------------------
//
// If all of the NaN bits are set, it's not a number. Otherwise, it is.
// That leaves all of the remaining bits as available for us to play with. We
// stuff a few different kinds of things here: special singleton values like
// "true", "false", and "null", and pointers to objects allocated on the heap.
// We'll use the sign bit to distinguish singleton values from pointers. If
// it's set, it's a pointer.
//
// v--Pointer or singleton?
// S[NaN      ]1---------------------------------------------------
//
// For singleton values, we just enumerate the different values. We'll use the
// low bits of the mantissa for that, and only need a few:
//
//                                                 3 Type bits--v
// 0[NaN      ]1------------------------------------------------[T]
//
// For pointers, we are left with 51 bits of mantissa to store an address.
// That's more than enough room for a 32-bit address. Even 64-bit machines
// only actually use 48 bits for addresses, so we've got plenty. We just stuff
// the address right into the mantissa.
//
// Ta-da, double precision numbers, pointers, and a bunch of singleton values,
// all stuffed into a single 64-bit sequence. Even better, we don't have to
// do any masking or work to extract number values: they are unmodified. This
// means math on numbers is fast.
typedef uint64_t LsValue;

// Convert an object into a value.
LsValue ls_obj2val(LsObj *obj);

// Convert a value into an object.
// You must check that value is an object before calling this function.
LsObj *ls_val2obj(LsValue value);

// Returns true if [a] and [b] are strictly the same value. This is identity
// for object values, and value equality for unboxed values.
#define ls_val_same(a, b) ((a) == (b))

// Returns true if [a] and [b] are equivalent. Immutable values (null, bools,
// numbers, ranges, and strings) are equal if they have the same data. All
// other values are equal if they are identical objects (e.g. ls_val_same).
bool ls_val_eq(LsValue a, LsValue b);

DECLARE_BUFFER(Value, value, LsValue);

// A heap-allocated string object.
typedef struct ls_obj_string {
  LsObj obj;

  // Number of bytes in the string, not including the null terminator.
  size_t length;

  // Inline array of the string's bytes followed by a null terminator.
  char value[];
} LsObjString;

typedef struct ls_obj_array {
  LsObj obj;

  ValueBuffer elements;
} LsObjArray;

typedef struct {
  LsValue key;
  LsValue value;
} MapEntry;

typedef struct ls_obj_map {
  LsObj obj;

  size_t capacity;
  size_t count;

  MapEntry *entries;
} LsObjMap;

// Creates a new string object and copies [text] into it.
//
// [text] must be non-NULL.
LsValue ls_new_string(LsVM *vm, const char *text);

// Creates a new string object of [length] and copies [text] into it.
//
// [text] may be NULL if [length] is zero.
LsValue ls_new_string_length(LsVM *vm, const char *text, size_t length);

// Creates a new array with [initial_length] LS_NULL elements.
LsValue ls_new_array(LsVM *vm, size_t initial_length);

LsValue ls_array_index(LsValue val);

// Creates a new empty map.
LsValue ls_new_map(LsVM *vm);

// Converts [num] to an [LsValue].
LsValue ls_num2val(double num);

// Interprets [val] as a [double].
double ls_val2num(LsValue val);

#endif
