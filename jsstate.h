#ifndef js_state_h
#define js_state_h

#include "jsobject.h" /* for js_Value */

#define JS_STACKSIZE 256
#define JS_TRYLIMIT 64
#define JS_GCLIMIT 10000 /* run gc cycle every N allocations */

void js_savetry(js_State *J, short *pc);

#define js_trypc(J, PC) \
	(js_savetry(J, PC), setjmp(J->trybuf[J->trylen++].buf))

#define js_try(J) \
	(js_savetry(J, NULL), setjmp(J->trybuf[J->trylen++].buf))

#define js_endtry(J) \
	(--J->trylen)

typedef struct js_Jumpbuf js_Jumpbuf;

struct js_Jumpbuf
{
	jmp_buf buf;
	js_Environment *E;
	int top, bot;
	short *pc;
};

struct js_State
{
	js_StringNode *strings;

	/* input */
	const char *filename;
	const char *source;
	int line;

	/* lexer */
	struct { char *text; size_t len, cap; } buf;
	int lexline;
	int lexchar;
	int lasttoken;
	int newline;

	/* parser */
	int lookahead;
	const char *text;
	double number;
	js_Ast *gcast; /* list of allocated nodes to free after parsing */

	/* compiler */
	int strict;

	/* runtime environment */
	js_Object *Object_prototype;
	js_Object *Array_prototype;
	js_Object *Function_prototype;
	js_Object *Boolean_prototype;
	js_Object *Number_prototype;
	js_Object *String_prototype;

	js_Object *Error_prototype;
	js_Object *EvalError_prototype;
	js_Object *RangeError_prototype;
	js_Object *ReferenceError_prototype;
	js_Object *SyntaxError_prototype;
	js_Object *TypeError_prototype;
	js_Object *URIError_prototype;

	js_Object *G;
	js_Environment *E;

	/* garbage collector list */
	int gcmark;
	int gccounter;
	js_Environment *gcenv;
	js_Function *gcfun;
	js_Object *gcobj;

	/* exception stack */
	int trylen;
	js_Jumpbuf trybuf[JS_TRYLIMIT];

	/* execution stack */
	int top, bot;
	js_Value stack[JS_STACKSIZE];
};

#endif
