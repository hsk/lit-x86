#ifndef _STD_FUNC_LIT_
#define _STD_FUNC_LIT_

#include "lit.h"
#include "asm.h"
#include "lex.h"
#include "expr.h"
#include "parse.h"

typedef struct {
	char *name, *mod_name;
	int args, addr;
} std_function;

int make_stdfunc(char *, char *); // func name, module namee

#endif
