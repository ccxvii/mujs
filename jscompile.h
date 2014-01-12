#ifndef js_compile_h
#define js_compile_h

enum
{
	OP_POP,
	OP_DUP,

	OP_CONST,
	OP_UNDEF,
	OP_NULL,
	OP_TRUE,
	OP_FALSE,
	OP_THIS,

	OP_ARRAY,
	OP_ARRAYPUT,
	OP_OBJECT,
	OP_OBJECTPUT,

	OP_VARDEC,
	OP_FUNDEC,
	OP_FUNEXP,

	OP_LOADVAR,	/* -(name)- <value> */
	OP_LOADINDEX,	/* <obj> <idx> -- <value> */
	OP_LOADMEMBER,	/* <obj> -(name)- <value> */

	OP_AVAR,	/* -(name)- <addr> */
	OP_AINDEX,	/* <obj> <idx> -- <addr> */
	OP_AMEMBER,	/* <obj> -(name)- <addr> */

	OP_LOAD,	/* <addr> -- <addr> <value> */
	OP_STORE,	/* <addr> <value> -- <value> */

	OP_CALL,	/* <fun> <args...> -(numargs)- <return value> */
	OP_TCALL,	/* <obj> <fun> <args...> -(numargs)- <return value> */
	OP_NEW,

	OP_DELETE,	/* <addr> -- <success> */
	OP_PREINC,	/* <addr> -- <value+1> */
	OP_PREDEC,	/* <addr> -- <value-1> */
	OP_POSTINC,	/* <addr> -- <value> */
	OP_POSTDEC,	/* <addr> -- <value> */

	OP_VOID,
	OP_TYPEOF,
	OP_POS,
	OP_NEG,
	OP_BITNOT,
	OP_LOGNOT,

	OP_BITOR,
	OP_BITXOR,
	OP_BITAND,
	OP_EQ,
	OP_NE,
	OP_EQ3,
	OP_NE3,
	OP_LT,
	OP_GT,
	OP_LE,
	OP_GE,
	OP_INSTANCEOF,
	OP_IN,
	OP_SHL,
	OP_SHR,
	OP_USHR,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,

	OP_TRY,
	OP_THROW,
	OP_RETURN,
	OP_DEBUGGER,

	OP_WITH,
	OP_ENDWITH,

	OP_JUMP,
	OP_JTRUE,
	OP_JFALSE,
};

struct js_Function
{
	unsigned char *code;
	int cap, len;

	js_Value *klist;
	int kcap, klen;

	js_Function *next;
};

js_Function *jsC_compile(js_State *J, js_Ast *prog);
void jsC_freecompile(js_State *J);
int jsC_error(js_State *J, js_Ast *node, const char *fmt, ...);
void jsC_dumpvalue(js_State *J, js_Value v);
void jsC_dumpfunction(js_State *J, js_Function *fun);

#endif
