#ifndef js_ast_h
#define js_ast_h

typedef struct js_Ast js_Ast;

struct js_Ast
{
	int type;
	int op;
	js_Ast *a, *b, *c, *d;
	double n;
	const char *s;
};

enum
{
	AST_LIST,

	AST_IDENTIFIER,
	AST_NUMBER,
	AST_STRING,
	AST_REGEXP,

	/* literals */
	EXP_NULL,
	EXP_TRUE,
	EXP_FALSE,
	EXP_THIS,

	EXP_ARRAY,
	EXP_OBJECT,
	EXP_PROP_VAL,
	EXP_PROP_GET,
	EXP_PROP_SET,

	/* expressions */
	EXP_INDEX,
	EXP_MEMBER,
	EXP_NEW,
	EXP_CALL,
	EXP_FUNC,
	EXP_COND,
	EXP_COMMA,

	EXP_DELETE,
	EXP_VOID,
	EXP_TYPEOF,
	EXP_PREINC,
	EXP_PREDEC,
	EXP_POSTINC,
	EXP_POSTDEC,
	EXP_POS,
	EXP_NEG,
	EXP_BITNOT,
	EXP_LOGNOT,

	EXP_LOGOR,
	EXP_LOGAND,
	EXP_BITOR,
	EXP_BITXOR,
	EXP_BITAND,
	EXP_EQ,
	EXP_NE,
	EXP_EQ3,
	EXP_NE3,
	EXP_LT,
	EXP_GT,
	EXP_LE,
	EXP_GE,
	EXP_INSTANCEOF,
	EXP_IN,
	EXP_SHL,
	EXP_SHR,
	EXP_USHR,
	EXP_ADD,
	EXP_SUB,
	EXP_MUL,
	EXP_DIV,
	EXP_MOD,

	EXP_ASS,
	EXP_ASS_MUL,
	EXP_ASS_DIV,
	EXP_ASS_MOD,
	EXP_ASS_ADD,
	EXP_ASS_SUB,
	EXP_ASS_SHL,
	EXP_ASS_SHR,
	EXP_ASS_USHR,
	EXP_ASS_BITAND,
	EXP_ASS_BITXOR,
	EXP_ASS_BITOR,

	/* statements */
	STM_NOP,
	STM_EXP,
	STM_VAR,
	STM_IF,
	STM_DO,
	STM_WHILE,
	STM_FOR_EXP,
	STM_FOR_VAR_EXP,
	STM_FOR_IN,
	STM_FOR_VAR_IN,
	STM_CONTINUE,
	STM_BREAK,
	STM_RETURN,
	STM_WITH,
	STM_SWITCH,
	STM_THROW,
	STM_TRY,
	STM_LABEL,
	STM_CASE,
	STM_DEFAULT,
	STM_DEBUGGER,
};

js_Ast *jsP_newnode(js_State *J, int type, js_Ast *a, js_Ast *b, js_Ast *c, js_Ast *d);
js_Ast *jsP_newsnode(js_State *J, int type, const char *s);
js_Ast *jsP_newnnode(js_State *J, int type, double n);

#endif
