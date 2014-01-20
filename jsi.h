#ifndef jsi_h
#define jsi_h

#include "js.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <math.h>
#include <float.h>

enum { JS_REGEXP_G = 1, JS_REGEXP_I = 2, JS_REGEXP_M = 4 }; /* RegExp flags */

enum { JS_HNONE, JS_HNUMBER, JS_HSTRING }; /* Hint to ToPrimitive() */

typedef struct js_Value js_Value;
typedef struct js_Object js_Object;

typedef struct js_Ast js_Ast;
typedef struct js_Function js_Function;
typedef struct js_Environment js_Environment;

typedef struct js_StringNode js_StringNode;
void jsS_dumpstrings(js_State *J);
void jsS_freestrings(js_State *J);

JS_NORETURN void jsR_throwError(js_State *J, const char *message);
JS_NORETURN void jsR_throwEvalError(js_State *J, const char *message);
JS_NORETURN void jsR_throwRangeError(js_State *J, const char *message);
JS_NORETURN void jsR_throwReferenceError(js_State *J, const char *message);
JS_NORETURN void jsR_throwSyntaxError(js_State *J, const char *message);
JS_NORETURN void jsR_throwTypeError(js_State *J, const char *message);
JS_NORETURN void jsR_throwURIError(js_State *J, const char *message);

js_Object *jsR_newcconstructor(js_State *J, js_CFunction cfunction, js_CFunction cconstructor);

const char *jsR_stringfromnumber(js_State *J, double number);
double jsR_numberfromstring(js_State *J, const char *string);

void js_newfunction(js_State *J, js_Function *function, js_Environment *scope);
void js_newscript(js_State *J, js_Function *function);
void js_dup(js_State *J);
void js_rot(js_State *J, int n);
void js_rot2(js_State *J);
void js_rot3(js_State *J);

/* Exception handling */

typedef struct js_Jumpbuf js_Jumpbuf;

struct js_Jumpbuf
{
	jmp_buf buf;
	js_Environment *E;
	int top, bot;
	short *pc;
};

void js_savetry(js_State *J, short *pc);

#define js_trypc(J, PC) \
	(js_savetry(J, PC), setjmp(J->trybuf[J->trylen++].buf))

#define js_try(J) \
	(js_savetry(J, NULL), setjmp(J->trybuf[J->trylen++].buf))

#define js_endtry(J) \
	(--J->trylen)

/* State struct */

struct js_State
{
	js_StringNode *strings;

	/* parser input source */
	const char *filename;
	const char *source;
	int line;

	/* lexer state */
	struct { char *text; size_t len, cap; } lexbuf;
	int lexline;
	int lexchar;
	int lasttoken;
	int newline;

	/* parser state */
	int lookahead;
	const char *text;
	double number;
	js_Ast *gcast; /* list of allocated nodes to free after parsing */

	/* compiler state */
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

	/* execution stack */
	int top, bot;
	js_Value *stack;

	/* garbage collector list */
	int gcmark;
	int gccounter;
	js_Environment *gcenv;
	js_Function *gcfun;
	js_Object *gcobj;

	/* exception stack */
	int trylen;
	js_Jumpbuf trybuf[JS_TRYLIMIT];
};

#endif
