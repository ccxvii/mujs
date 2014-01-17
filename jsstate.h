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

};

#endif
