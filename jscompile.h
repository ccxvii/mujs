#ifndef js_compile_h
#define js_compile_h

enum
{
	OP_POP,		/* A -- */
	OP_DUP,		/* A -- A A */
	OP_DUP2,	/* A B -- A B A B */
	OP_ROT2,	/* A B -- B A */
	OP_ROT3,	/* A B C -- C A B */
	OP_DUP1ROT4,	/* A B C -- C A B C */

	OP_NUMBER_0,	/* -- 0 */
	OP_NUMBER_1,	/* -- 1 */
	OP_NUMBER_X,	/* -K- K */

	OP_NUMBER,	/* -N- <number> */
	OP_STRING,	/* -S- <string> */
	OP_CLOSURE,	/* -F- <closure> */

	OP_NEWARRAY,
	OP_NEWOBJECT,

	OP_UNDEF,
	OP_NULL,
	OP_TRUE,
	OP_FALSE,
	OP_THIS,
	OP_GLOBAL,

	OP_FUNDEC,	/* <closure> -S- */
	OP_VARDEC,	/* -S- */

	OP_GETVAR,	/* -S- <value> */
	OP_SETVAR,	/* <value> -S- <value> */
	OP_DELVAR,	/* -S- <success> */

	OP_IN,		/* <name> <obj> -- <exists?> */
	OP_GETPROP,	/* <obj> <name> -- <value> */
	OP_SETPROP,	/* <obj> <name> <value> -- <value> */
	OP_DELPROP,	/* <obj> <name> -- <success> */
	OP_NEXTPROP,	/* <obj> <name> -- <obj> <name+1> true || false */

	OP_CALL,	/* <closure> <this> <args...> -(numargs)- <returnvalue> */
	OP_NEW,		/* <closure> <args...> -(numargs)- <returnvalue> */

	OP_TYPEOF,
	OP_POS,
	OP_NEG,
	OP_BITNOT,
	OP_LOGNOT,

	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_ADD,
	OP_SUB,
	OP_SHL,
	OP_SHR,
	OP_USHR,
	OP_LT,
	OP_GT,
	OP_LE,
	OP_GE,
	OP_EQ,
	OP_NE,
	OP_STRICTEQ,
	OP_STRICTNE,
	OP_BITAND,
	OP_BITXOR,
	OP_BITOR,

	OP_INSTANCEOF,

	OP_THROW,
	OP_TRY,
	OP_CATCH,
	OP_ENDCATCH,
	OP_WITH,
	OP_ENDWITH,

	OP_DEBUGGER,
	OP_JUMP,
	OP_JTRUE,
	OP_JFALSE,
	OP_RETURN,
};

struct js_Function
{
	const char *name;

	int numparams;
	const char **params;

	short *code;
	int codecap, codelen;

	js_Function **funtab;
	int funcap, funlen;

	double *numtab;
	int numcap, numlen;

	const char **strtab;
	int strcap, strlen;

	const char *filename;
	int line;

	js_Function *gcnext;
	int gcmark;
};

js_Function *jsC_compile(js_State *J, js_Ast *prog);

void jsC_dumpfunction(js_State *J, js_Function *fun);

#endif
