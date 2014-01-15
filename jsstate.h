#ifndef js_state_h
#define js_state_h

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
	js_Ast *ast; /* list of allocated nodes to free after parsing */

	/* compiler */
	js_Function *fun; /* list of allocated functions to free on errors */

	/* runtime */
	js_Environment *E;
	js_Object *global;

	int strict;
};

#endif
