#ifndef js_state_h
#define js_state_h

#include "jsobject.h" /* for js_Value */

#define JS_STACKSIZE 256
#define JS_GCLIMIT 10000 /* run gc cycle every N allocations */

struct js_State
{
	jmp_buf jb; /* setjmp buffer for error handling in parser */

	js_StringNode *strings;

	/* input */
	const char *filename;
	const char *source;
	int line;

	/* lexer */
	struct { char *text; size_t len, cap; } buf;
	int lasttoken;
	int newline;

	/* parser */
	int lookahead;
	const char *text;
	double number;
	js_Ast *gcast; /* list of allocated nodes to free after parsing */

	/* compiler */
	js_Function *fun; /* list of allocated functions to free on errors */

	int strict;

	/* runtime */
	js_Object *Object_prototype;
	js_Object *Array_prototype;
	js_Object *Function_prototype;
	js_Object *Boolean_prototype;
	js_Object *Number_prototype;
	js_Object *String_prototype;

	js_Object *G;
	js_Environment *E;

	/* garbage collector list */
	int gcmark;
	int gccounter;
	js_Environment *gcenv;
	js_Function *gcfun;
	js_Object *gcobj;

	/* execution stack */
	int top, bot;
	js_Value stack[JS_STACKSIZE];
};

#endif
