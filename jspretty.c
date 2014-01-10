#include "js.h"
#include "jsparse.h"

static void printast(js_Ast *n, int level);

static const char *strast(int type)
{
	switch (type) {
	case AST_LIST: return "LIST";
	case AST_IDENTIFIER: return "IDENTIFIER";
	case AST_NUMBER: return "NUMBER";
	case AST_STRING: return "STRING";
	case AST_REGEXP: return "REGEXP";
	case EXP_VAR: return "VAR";
	case EXP_NULL: return "NULL";
	case EXP_TRUE: return "TRUE";
	case EXP_FALSE: return "FALSE";
	case EXP_THIS: return "THIS";
	case EXP_ARRAY: return "ARRAY";
	case EXP_OBJECT: return "OBJECT";
	case EXP_PROP_VAL: return "PROP_VAL";
	case EXP_PROP_GET: return "PROP_GET";
	case EXP_PROP_SET: return "PROP_SET";
	case EXP_INDEX: return "INDEX";
	case EXP_MEMBER: return "MEMBER";
	case EXP_NEW: return "new";
	case EXP_CALL: return "CALL";
	case EXP_FUNC: return "function";
	case EXP_COND: return "?:";
	case EXP_COMMA: return ",";
	case EXP_DELETE: return "delete";
	case EXP_VOID: return "void";
	case EXP_TYPEOF: return "typeof";
	case EXP_PREINC: return "PRE++";
	case EXP_PREDEC: return "PRE--";
	case EXP_POSTINC: return "POST++";
	case EXP_POSTDEC: return "POST--";
	case EXP_POS: return "+";
	case EXP_NEG: return "-";
	case EXP_BITNOT: return "~";
	case EXP_LOGNOT: return "!";
	case EXP_LOGOR: return "||";
	case EXP_LOGAND: return "&&";
	case EXP_BITOR: return "|";
	case EXP_BITXOR: return "^";
	case EXP_BITAND: return "&";
	case EXP_EQ: return "==";
	case EXP_NE: return "!=";
	case EXP_EQ3: return "===";
	case EXP_NE3: return "!==";
	case EXP_LT: return "<";
	case EXP_GT: return ">";
	case EXP_LE: return "<=";
	case EXP_GE: return ">=";
	case EXP_INSTANCEOF: return "instanceof";
	case EXP_IN: return "in";
	case EXP_SHL: return "<<";
	case EXP_SHR: return ">>";
	case EXP_USHR: return ">>>";
	case EXP_ADD: return "+";
	case EXP_SUB: return "-";
	case EXP_MUL: return "*";
	case EXP_DIV: return "/";
	case EXP_MOD: return "%";
	case EXP_ASS: return "=";
	case EXP_ASS_MUL: return "*=";
	case EXP_ASS_DIV: return "/=";
	case EXP_ASS_MOD: return "%=";
	case EXP_ASS_ADD: return "+=";
	case EXP_ASS_SUB: return "-=";
	case EXP_ASS_SHL: return "<<=";
	case EXP_ASS_SHR: return ">>=";
	case EXP_ASS_USHR: return ">>>=";
	case EXP_ASS_BITAND: return "&=";
	case EXP_ASS_BITXOR: return "^=";
	case EXP_ASS_BITOR: return "|=";
	case STM_BLOCK: return "BLOCK";
	case STM_FUNC: return "function-decl";
	case STM_NOP: return "NOP";
	case STM_VAR: return "var";
	case STM_IF: return "if";
	case STM_DO: return "do-while";
	case STM_WHILE: return "while";
	case STM_FOR: return "for";
	case STM_FOR_VAR: return "for_var";
	case STM_FOR_IN: return "for_in";
	case STM_FOR_IN_VAR: return "for_in_var";
	case STM_CONTINUE: return "continue";
	case STM_BREAK: return "break";
	case STM_RETURN: return "return";
	case STM_WITH: return "with";
	case STM_SWITCH: return "switch";
	case STM_THROW: return "throw";
	case STM_TRY: return "try";
	case STM_LABEL: return "label";
	case STM_CASE: return "case";
	case STM_DEFAULT: return "default";
	case STM_DEBUGGER: return "debugger";
	default: return "(unknown)";
	}
}

static void indent(int level)
{
	while (level--)
		putchar('\t');
}

static void printlist(js_Ast *n, int level, const char *sep)
{
	while (n) {
		printast(n->a, level);
		n = n->b;
		if (n)
			fputs(sep, stdout);
	}
}

static void printblock(js_Ast *n, int level)
{
	while (n) {
		indent(level);
		printast(n->a, level);
		if (n->a->type < STM_BLOCK) // expression
			putchar(';');
		n = n->b;
		if (n)
			putchar('\n');
	}
}

static void printstm(js_Ast *n, int level)
{
	if (n->type == STM_BLOCK) {
		printf(" {\n");
		printblock(n->a, level + 1);
		putchar('\n');
		indent(level);
		printf("}");
	} else {
		putchar('\n');
		indent(level + 1);
		printast(n, level + 1);
		if (n->type < STM_BLOCK) // expression
			putchar(';');
	}
}

static void printunary(int level, js_Ast *n, const char *pre, const char *suf)
{
	printf(pre);
	printast(n, level);
	printf(suf);
}

static void printbinary(int level, js_Ast *a, js_Ast *b, const char *op)
{
	printf("(");
	printast(a, level);
	printf(" %s ", op);
	printast(b, level);
	printf(")");
}

static void printast(js_Ast *n, int level)
{
	switch (n->type) {
	case AST_IDENTIFIER: printf("%s", n->s); return;
	case AST_NUMBER: printf("%g", n->n); return;
	case AST_STRING: printf("'%s'", n->s); return;
	case AST_REGEXP: printf("/%s/", n->s); return;
	case AST_LIST:
		putchar('[');
		printlist(n, level, " ");
		putchar(']');
		break;

	case STM_BLOCK:
		putchar('{');
		putchar('\n');
		printblock(n->a, level + 1);
		putchar('\n');
		indent(level);
		putchar('}');
		break;

	case STM_FOR:
		printf("for (");
		printast(n->a, level); printf("; ");
		printast(n->b, level); printf("; ");
		printast(n->c, level); printf(")");
		printstm(n->d, level);
		break;
	case STM_FOR_VAR:
		printf("for (var ");
		printlist(n->a, level, ", "); printf("; ");
		printast(n->b, level); printf("; ");
		printast(n->c, level); printf(")");
		printstm(n->d, level);
		break;
	case STM_FOR_IN:
		printf("for (");
		printast(n->a, level); printf(" in ");
		printast(n->b, level); printf(")");
		printstm(n->c, level);
		break;
	case STM_FOR_IN_VAR:
		printf("for (var ");
		printlist(n->a, level, ", "); printf(" in ");
		printast(n->b, level); printf(")");
		printstm(n->c, level);
		break;

	case STM_NOP:
		putchar(';');
		break;

	case STM_VAR:
		printf("var ");
		printlist(n->a, level, ", ");
		putchar(';');
		break;

	case EXP_VAR:
		printast(n->a, level);
		if (n->b) {
			printf(" = ");
			printast(n->b, level);
		}
		break;

	case STM_IF:
		printf("if (");
		printast(n->a, level);
		printf(")");
		printstm(n->b, level);
		if (n->c) {
			putchar('\n');
			indent(level);
			printf("else");
			printstm(n->c, level);
		}
		break;

	case STM_DO:
		printf("do");
		printstm(n->a, level);
		if (n->a->type == STM_BLOCK) {
			putchar(' ');
		} else {
			putchar('\n');
			indent(level);
		}
		printf("while (");
		printast(n->b, level);
		printf(");");
		break;

	case STM_WHILE:
		printf("while (");
		printast(n->a, level);
		printf(")");
		printstm(n->b, level);
		break;

	case STM_CONTINUE:
		if (n->a) {
			printf("continue ");
			printast(n->a, level);
			printf(";");
		} else {
			printf("continue;");
		}
		break;

	case STM_BREAK:
		if (n->a) {
			printf("break ");
			printast(n->a, level);
			printf(";");
		} else {
			printf("break;");
		}
		break;

	case STM_RETURN:
		if (n->a) {
			printf("return ");
			printast(n->a, level);
			printf(";");
		} else {
			printf("return;");
		}
		break;

	case STM_THROW:
		printf("throw ");
		printast(n->a, level);
		printf(";");
		break;

	case STM_SWITCH:
		printf("switch (");
		printast(n->a, level);
		printf(") {\n");
		printblock(n->b, level);
		putchar('\n');
		indent(level);
		printf("}");
		break;

	case STM_CASE:
		printf("case ");
		printast(n->a, level);
		printf(":");
		if (n->b) {
			printf("\n");
			printblock(n->b, level + 1);
		}
		break;

	case STM_DEFAULT:
		printf("default:");
		if (n->a) {
			printf("\n");
			printblock(n->a, level + 1);
		}
		break;

	case STM_LABEL:
		printast(n->a, level);
		printf(":");
		printstm(n->b, level - 1);
		break;

	case STM_WITH:
		printf("with (");
		printast(n->a, level);
		printf(")");
		printstm(n->b, level);
		break;

	case STM_TRY:
		printf("try");
		printstm(n->a, level);
		if (n->b && n->c) {
			printf(" catch (");
			printast(n->b, level);
			printf(")");
			printstm(n->c, level);
		}
		if (n->d) {
			printf(" finally");
			printstm(n->d, level);
		}
		break;

	case STM_DEBUGGER:
		printf("debugger");
		break;

	case STM_FUNC:
		printf("function ");
		printast(n->a, level);
		printf("(");
		printlist(n->b, level, ", ");
		printf(")\n");
		indent(level);
		printf("{\n");
		printblock(n->c, level + 1);
		printf("\n");
		indent(level);
		printf("}");
		break;

	case EXP_FUNC:
		printf("(function ");
		if (n->a)
			printast(n->a, level);
		printf("(");
		printlist(n->b, level, ", ");
		printf(") {\n");
		printblock(n->c, level + 1);
		printf("\n");
		indent(level);
		printf("})");
		break;

	case EXP_OBJECT:
		printf("{ ");
		printlist(n->a, level, ", ");
		printf(" }");
		break;

	case EXP_PROP_VAL:
		printast(n->a, level);
		printf(": ");
		printast(n->b, level);
		break;

	case EXP_ARRAY:
		printf("[ ");
		printlist(n->a, level, ", ");
		printf(" ]");
		break;

	case EXP_NEW:
		printf("(new ");
		printast(n->a, level);
		printf("(");
		printlist(n->b, level, ", ");
		printf("))");
		break;

	case EXP_CALL:
		printf("(");
		printast(n->a, level);
		printf("(");
		printlist(n->b, level, ", ");
		printf("))");
		break;

	case EXP_MEMBER:
		printf("(");
		printast(n->a, level);
		printf(".");
		printast(n->b, level);
		printf(")");
		break;

	case EXP_INDEX:
		printf("(");
		printast(n->a, level);
		printf("[");
		printast(n->b, level);
		printf("])");
		break;

	case EXP_COND:
		printf("(");
		printast(n->a, level);
		printf(" ? ");
		printast(n->b, level);
		printf(" : ");
		printast(n->c, level);
		printf(")");
		break;

	case EXP_NULL: printf("null"); break;
	case EXP_TRUE: printf("true"); break;
	case EXP_FALSE: printf("false"); break;
	case EXP_THIS: printf("this"); break;

	case EXP_DELETE:	printunary(level, n->a, "(delete ", ")"); break;
	case EXP_VOID:		printunary(level, n->a, "(void ", ")"); break;
	case EXP_TYPEOF:	printunary(level, n->a, "(typeof ", ")"); break;
	case EXP_PREINC:	printunary(level, n->a, "(++", ")"); break;
	case EXP_PREDEC:	printunary(level, n->a, "(--", ")"); break;
	case EXP_POSTINC:	printunary(level, n->a, "(", "++)"); break;
	case EXP_POSTDEC:	printunary(level, n->a, "(", "--)"); break;
	case EXP_POS:		printunary(level, n->a, "(+", ")"); break;
	case EXP_NEG:		printunary(level, n->a, "(-", ")"); break;
	case EXP_BITNOT:	printunary(level, n->a, "(~", ")"); break;
	case EXP_LOGNOT:	printunary(level, n->a, "(!", ")"); break;

	case EXP_COMMA:		printbinary(level, n->a, n->b, ","); break;
	case EXP_LOGOR:		printbinary(level, n->a, n->b, "||"); break;
	case EXP_LOGAND:	printbinary(level, n->a, n->b, "&&"); break;
	case EXP_BITOR:		printbinary(level, n->a, n->b, "|"); break;
	case EXP_BITXOR:	printbinary(level, n->a, n->b, "^"); break;
	case EXP_BITAND:	printbinary(level, n->a, n->b, "&"); break;
	case EXP_EQ:		printbinary(level, n->a, n->b, "=="); break;
	case EXP_NE:		printbinary(level, n->a, n->b, "!="); break;
	case EXP_EQ3:		printbinary(level, n->a, n->b, "==="); break;
	case EXP_NE3:		printbinary(level, n->a, n->b, "!=="); break;
	case EXP_LT:		printbinary(level, n->a, n->b, "<"); break;
	case EXP_GT:		printbinary(level, n->a, n->b, ">"); break;
	case EXP_LE:		printbinary(level, n->a, n->b, "<="); break;
	case EXP_GE:		printbinary(level, n->a, n->b, ">="); break;
	case EXP_INSTANCEOF:	printbinary(level, n->a, n->b, "instanceof"); break;
	case EXP_IN:		printbinary(level, n->a, n->b, "in"); break;
	case EXP_SHL:		printbinary(level, n->a, n->b, "<<"); break;
	case EXP_SHR:		printbinary(level, n->a, n->b, ">>"); break;
	case EXP_USHR:		printbinary(level, n->a, n->b, ">>>"); break;
	case EXP_ADD:		printbinary(level, n->a, n->b, "+"); break;
	case EXP_SUB:		printbinary(level, n->a, n->b, "-"); break;
	case EXP_MUL:		printbinary(level, n->a, n->b, "*"); break;
	case EXP_DIV:		printbinary(level, n->a, n->b, "/"); break;
	case EXP_MOD:		printbinary(level, n->a, n->b, "%"); break;
	case EXP_ASS:		printbinary(level, n->a, n->b, "="); break;
	case EXP_ASS_MUL:	printbinary(level, n->a, n->b, "*="); break;
	case EXP_ASS_DIV:	printbinary(level, n->a, n->b, "/="); break;
	case EXP_ASS_MOD:	printbinary(level, n->a, n->b, "%="); break;
	case EXP_ASS_ADD:	printbinary(level, n->a, n->b, "+="); break;
	case EXP_ASS_SUB:	printbinary(level, n->a, n->b, "-="); break;
	case EXP_ASS_SHL:	printbinary(level, n->a, n->b, "<<="); break;
	case EXP_ASS_SHR:	printbinary(level, n->a, n->b, ">>="); break;
	case EXP_ASS_USHR:	printbinary(level, n->a, n->b, ">>>="); break;
	case EXP_ASS_BITAND:	printbinary(level, n->a, n->b, "&="); break;
	case EXP_ASS_BITXOR:	printbinary(level, n->a, n->b, "^="); break;
	case EXP_ASS_BITOR:	printbinary(level, n->a, n->b, "|="); break;

	default:
		printf("(%s", strast(n->type));
		if (n->a) { putchar(' '); printast(n->a, level); }
		if (n->b) { putchar(' '); printast(n->b, level); }
		if (n->c) { putchar(' '); printast(n->c, level); }
		if (n->d) { putchar(' '); printast(n->d, level); }
		putchar(')');
		break;
	}
}

void jsP_pretty(js_State *J, js_Ast *prog)
{
	printblock(prog, 0);
	putchar('\n');
}
