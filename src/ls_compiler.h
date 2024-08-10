#ifndef LS_COMPILER_H_INCLUDE
#define LS_COMPILER_H_INCLUDE

#include "ls_vm.h"

typedef struct ls_compiler LsCompiler;

LsObj *ls_compile(LsVM *vm, const char *source);

#endif
