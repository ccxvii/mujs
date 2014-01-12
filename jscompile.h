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

	OP_LOADVAR,
	OP_LOADINDEX,
	OP_LOADMEMBER,
	OP_AVAR,
	OP_AINDEX,
	OP_AMEMBER,

	OP_LOAD,
	OP_DUP_LOAD,
	OP_STORE,

	OP_CALL,
	OP_TCALL,
	OP_NEW,

	OP_DELETE,
	OP_VOID,
	OP_TYPEOF,
	OP_PREINC,
	OP_POSTINC,
	OP_PREDEC,
	OP_POSTDEC,
	OP_POS,
	OP_NEG,
	OP_BITNOT,
	OP_LOGNOT,

	OP_LOGOR,
	OP_LOGAND,
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