#ifndef js_h
#define js_h

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

typedef struct js_State js_State;

typedef int (*js_CFunction)(js_State *J);

js_State *js_newstate(void);
void js_close(js_State *J);

int js_error(js_State *J, const char *fmt, ...);

int js_loadstring(js_State *J, const char *s);
int js_loadfile(js_State *J, const char *filename);

/* private */

void jsP_initlex(js_State *J, const char *source);
int jsP_lex(js_State *J);
int jsP_parse(js_State *J);

struct js_State
{
	const char *yysource;
	char *yytext;
	size_t yylen, yycap;
	double yynumber;
	struct { int g, i, m; } yyflags;
	int yyline;
	int lasttoken;
	int newline;
	int strict;
};

#endif
