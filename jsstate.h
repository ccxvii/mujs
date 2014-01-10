#ifndef js_state_h
#define js_state_h

typedef struct js_Ast js_Ast;

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
