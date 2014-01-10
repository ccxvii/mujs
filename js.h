#ifndef js_h
#define js_h

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <math.h>

typedef struct js_State js_State;
typedef struct js_StringNode js_StringNode;

typedef int (*js_CFunction)(js_State *J);

js_State *js_newstate(void);
void js_close(js_State *J);

int js_error(js_State *J, const char *fmt, ...);

int js_loadstring(js_State *J, const char *s);
int js_loadfile(js_State *J, const char *filename);

const char *js_intern(js_State *J, const char *s);

/* private */

typedef struct js_Ast js_Ast;

void jsP_initlex(js_State *J, const char *filename, const char *source);
int jsP_lex(js_State *J);
const char *jsP_tokenstring(int token);
int jsP_parse(js_State *J, const char *filename, const char *source);
int jsP_error(js_State *J, const char *fmt, ...);

void js_printstringtree(js_State *J);

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
	struct { char g, i, m; } flags;
	js_Ast *ast; /* list of allocated nodes to free after parsing */

	int strict;
};

#endif
