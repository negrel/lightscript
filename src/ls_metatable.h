#ifndef LS_METAMETHODS_H_INCLUDE
#define LS_METAMETHODS_H_INCLUDE

#include "ls_value.h"

typedef struct {
  LsValue (*add)(LsValue, LsValue);
  LsValue (*sub)(LsValue, LsValue);
  LsValue (*mul)(LsValue, LsValue);
  LsValue (*div)(LsValue, LsValue);
  LsValue (*unm)(LsValue, LsValue);
  LsValue (*mod)(LsValue, LsValue);
  LsValue (*pow)(LsValue, LsValue);
  LsValue (*band)(LsValue, LsValue);
  LsValue (*bor)(LsValue, LsValue);
  LsValue (*bnot)(LsValue, LsValue);
  LsValue (*shl)(LsValue, LsValue);
  LsValue (*shr)(LsValue, LsValue);
  LsValue (*eq)(LsValue, LsValue);
  LsValue (*lt)(LsValue, LsValue);
  LsValue (*le)(LsValue, LsValue);
  LsValue (*index)(LsValue, LsValue);
  LsValue (*newindex)(LsValue, LsValue);
  LsValue (*call)(LsValue, LsValue);
  LsValue (*close)(LsValue, LsValue);

  LsObjMap *extra;
} LsMetatable;

size_t foo = sizeof(LsMetatable);

#endif
