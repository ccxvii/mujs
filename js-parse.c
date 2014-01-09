#include "js.h"
#include "js-parse.h"
#include "js-ast.h"

#define A1(x,a)		jsP_newnode(J, x, a, 0, 0, 0)
#define A2(x,a,b)	jsP_newnode(J, x, a, b, 0, 0)
#define A3(x,a,b,c)	jsP_newnode(J, x, a, b, c, 0)

#define LIST(h,t)	jsP_newnode(J, AST_LIST, h, t, 0, 0);
#define HEAD(h)		jsP_newnode(J, AST_LIST, h, 0, 0, 0);
#define TAIL(t)		jsP_newnode(J, AST_LIST, 0, t, 0, 0);

#define ID(s)		jsP_newsnode(J, AST_IDENTIFIER, s)

#define EXP0(x)		jsP_newnode(J, EXP_ ## x, 0, 0, 0, 0)
#define EXP1(x,a)	jsP_newnode(J, EXP_ ## x, a, 0, 0, 0)
#define EXP2(x,a,b)	jsP_newnode(J, EXP_ ## x, a, b, 0, 0)
#define EXP3(x,a,b,c)	jsP_newnode(J, EXP_ ## x, a, b, c, 0)

#define STM1(x,a)	jsP_newnode(J, STM_ ## x, a, 0, 0, 0)
#define STM2(x,a,b)	jsP_newnode(J, STM_ ## x, a, b, 0, 0)
#define STM3(x,a,b,c)	jsP_newnode(J, STM_ ## x, a, b, c, 0)
#define STM4(x,a,b,c,d)	jsP_newnode(J, STM_ ## x, a, b, c, d)

static js_Ast *expression(js_State *J, int notin);
static js_Ast *assignment(js_State *J, int notin);
static js_Ast *memberexp(js_State *J);
static js_Ast *block(js_State *J);

static void next(js_State *J)
{
	J->lookahead = jsP_lex(J);
}

static int accept(js_State *J, int t)
{
	if (J->lookahead == t) {
		next(J);
		return 1;
	}
	return 0;
}

static void expect(js_State *J, int t)
{
	if (accept(J, t))
		return;
	jsP_error(J, "unexpected token %d (expected %d)", J->lookahead, t);
}

static void semicolon(js_State *J)
{
	if (J->lookahead == ';') {
		next(J);
		return;
	}
	if (J->newline || J->lookahead == '}' || J->lookahead == 0)
		return;
	jsP_error(J, "unexpected token %d (expected semicolon)", J->lookahead);
}

static js_Ast *identifier(js_State *J)
{
	if (J->lookahead == TK_IDENTIFIER) {
		js_Ast *a = ID(J->yytext);
		next(J);
		return a;
	}
	jsP_error(J, "unexpected token %d (expected identifier)", J->lookahead);
	return NULL;
}

static js_Ast *identifiername(js_State *J)
{
	if (J->lookahead == TK_IDENTIFIER || J->lookahead >= TK_BREAK) {
		js_Ast *a = ID(J->yytext);
		next(J);
		return a;
	}
	jsP_error(J, "unexpected token %d (expected identifier or keyword)", J->lookahead);
	return NULL;
}

static js_Ast *arguments(js_State *J)
{
	js_Ast *head, *node;

	if (J->lookahead == ')')
		return NULL;

	node = assignment(J, 0);
	head = HEAD(node);
	while (accept(J, ',')) {
		node = assignment(J, 0);
		head = LIST(head, node);
	}
	return head;
}

static js_Ast *arrayliteral(js_State *J)
{
	js_Ast *head, *node;

	while (J->lookahead == ',')
		next(J);

	if (J->lookahead == ']')
		return NULL;

	node = assignment(J, 0);
	head = HEAD(node);
	while (accept(J, ',')) {
		while (J->lookahead == ',')
			next(J);
		if (J->lookahead == ']')
			break;
		node = assignment(J, 0);
		head = LIST(head, node);
	}
	return head;
}

static js_Ast *propname(js_State *J)
{
	js_Ast *name;
	if (J->lookahead == TK_NUMBER) {
		name = jsP_newnnode(J, AST_NUMBER, J->yynumber);
		next(J);
	} else if (J->lookahead == TK_STRING) {
		name = jsP_newsnode(J, AST_NUMBER, J->yytext);
		next(J);
	} else {
		name = identifiername(J);
	}
	return name;
}

static js_Ast *propassign(js_State *J)
{
	js_Ast *name, *value, *arg, *body;

	if (J->lookahead == TK_IDENTIFIER && !strcmp(J->yytext, "get")) {
		next(J);
		name = propname(J);
		expect(J, '(');
		expect(J, ')');
		body = block(J);
		return EXP2(PROP_GET, name, body);
	}

	if (J->lookahead == TK_IDENTIFIER && !strcmp(J->yytext, "set")) {
		next(J);
		name = propname(J);
		expect(J, '(');
		arg = identifier(J);
		expect(J, ')');
		body = block(J);
		return EXP3(PROP_SET, name, arg, body);
	}

	name = propname(J);
	expect(J, ':');
	value = assignment(J, 0);
	return EXP2(PROP_VAL, name, value);
}

static js_Ast *objectliteral(js_State *J)
{
	js_Ast *head, *node;

	if (J->lookahead == '}')
		return NULL;

	node = propassign(J);
	head = HEAD(node);
	while (accept(J, ',')) {
		if (J->lookahead == '}')
			break;
		node = propassign(J);
		head = LIST(head, node);
	}
	return head;
}

static js_Ast *primary(js_State *J)
{
	js_Ast *a;
	if (J->lookahead == TK_IDENTIFIER) {
		a = jsP_newsnode(J, AST_IDENTIFIER, J->yytext);
		next(J);
		return a;
	}
	if (J->lookahead == TK_STRING) {
		a = jsP_newsnode(J, AST_STRING, J->yytext);
		next(J);
		return a;
	}
	if (J->lookahead == TK_NUMBER) {
		a = jsP_newnnode(J, AST_NUMBER, J->yynumber);
		next(J);
		return a;
	}
	if (accept(J, TK_THIS)) return EXP0(THIS);
	if (accept(J, TK_NULL)) return EXP0(NULL);
	if (accept(J, TK_TRUE)) return EXP0(TRUE);
	if (accept(J, TK_FALSE)) return EXP0(FALSE);
	if (accept(J, '{')) { a = EXP1(OBJECT, objectliteral(J)); expect(J, '}'); return a; }
	if (accept(J, '[')) { a = EXP1(ARRAY, arrayliteral(J)); expect(J, ']'); return a; }
	if (accept(J, '(')) { a = expression(J, 0); expect(J, ')'); return a; }
	jsP_error(J, "unexpected token in primary expression: %d", J->lookahead);
	return NULL;
}

static js_Ast *newexp(js_State *J)
{
	js_Ast *a, *b;
	if (accept(J, TK_NEW)) {
		a = memberexp(J);
		if (accept(J, '(')) {
			b = arguments(J);
			expect(J, ')');
			return EXP2(NEW, a, b);
		}
		return EXP1(NEW, a);
	}
	return primary(J);
}

static js_Ast *memberexp(js_State *J)
{
	js_Ast *a = newexp(J);
loop:
	if (accept(J, '.')) { a = EXP2(MEMBER, a, identifiername(J)); goto loop; }
	if (accept(J, '[')) { a = EXP2(INDEX, a, expression(J, 0)); expect(J, ']'); goto loop; }
	return a;
}

static js_Ast *callexp(js_State *J)
{
	js_Ast *a = newexp(J);
loop:
	if (accept(J, '.')) { a = EXP2(MEMBER, a, identifiername(J)); goto loop; }
	if (accept(J, '[')) { a = EXP2(INDEX, a, expression(J, 0)); expect(J, ']'); goto loop; }
	if (accept(J, '(')) { a = EXP2(CALL, a, arguments(J)); expect(J, ')'); goto loop; }
	return a;
}

static js_Ast *postfix(js_State *J)
{
	js_Ast *a = callexp(J);
	if (!J->newline && accept(J, TK_INC)) return EXP1(POSTINC, a);
	if (!J->newline && accept(J, TK_DEC)) return EXP1(POSTDEC, a);
	return a;
}

static js_Ast *unary(js_State *J)
{
	if (accept(J, TK_DELETE)) return EXP1(DELETE, unary(J));
	if (accept(J, TK_VOID)) return EXP1(VOID, unary(J));
	if (accept(J, TK_TYPEOF)) return EXP1(TYPEOF, unary(J));
	if (accept(J, TK_INC)) return EXP1(PREINC, unary(J));
	if (accept(J, TK_DEC)) return EXP1(PREDEC, unary(J));
	if (accept(J, '+')) return EXP1(POS, unary(J));
	if (accept(J, '-')) return EXP1(NEG, unary(J));
	if (accept(J, '~')) return EXP1(BITNOT, unary(J));
	if (accept(J, '!')) return EXP1(LOGNOT, unary(J));
	return postfix(J);
}

static js_Ast *multiplicative(js_State *J)
{
	js_Ast *a = unary(J);
loop:
	if (accept(J, '*')) { a = EXP2(MUL, a, unary(J)); goto loop; }
	if (accept(J, '/')) { a = EXP2(DIV, a, unary(J)); goto loop; }
	if (accept(J, '%')) { a = EXP2(MOD, a, unary(J)); goto loop; }
	return a;
}

static js_Ast *additive(js_State *J)
{
	js_Ast *a = multiplicative(J);
loop:
	if (accept(J, '+')) { a = EXP2(ADD, a, multiplicative(J)); goto loop; }
	if (accept(J, '-')) { a = EXP2(SUB, a, multiplicative(J)); goto loop; }
	return a;
}

static js_Ast *shift(js_State *J)
{
	js_Ast *a = additive(J);
loop:
	if (accept(J, TK_SHL)) { a = EXP2(SHL, a, additive(J)); goto loop; }
	if (accept(J, TK_SHR)) { a = EXP2(SHR, a, additive(J)); goto loop; }
	if (accept(J, TK_USHR)) { a = EXP2(USHR, a, additive(J)); goto loop; }
	return a;
}

static js_Ast *relational(js_State *J, int notin)
{
	js_Ast *a = shift(J);
loop:
	if (accept(J, '<')) { a = EXP2(LT, a, shift(J)); goto loop; }
	if (accept(J, '>')) { a = EXP2(GT, a, shift(J)); goto loop; }
	if (accept(J, TK_LE)) { a = EXP2(LE, a, shift(J)); goto loop; }
	if (accept(J, TK_GE)) { a = EXP2(GE, a, shift(J)); goto loop; }
	if (accept(J, TK_INSTANCEOF)) { a = EXP2(INSTANCEOF, a, shift(J)); goto loop; }
	if (!notin && accept(J, TK_IN)) { a = EXP2(IN, a, shift(J)); goto loop; }
	return a;
}

static js_Ast *equality(js_State *J, int notin)
{
	js_Ast *a = relational(J, notin);
loop:
	if (accept(J, TK_EQ)) { a = EXP2(EQ, a, relational(J, notin)); goto loop; }
	if (accept(J, TK_NE)) { a = EXP2(NE, a, relational(J, notin)); goto loop; }
	if (accept(J, TK_EQ3)) { a = EXP2(EQ3, a, relational(J, notin)); goto loop; }
	if (accept(J, TK_NE3)) { a = EXP2(NE3, a, relational(J, notin)); goto loop; }
	return a;
}

static js_Ast *bitand(js_State *J, int notin)
{
	js_Ast *a = equality(J, notin);
	while (accept(J, '&'))
		a = EXP2(BITAND, a, equality(J, notin));
	return a;
}

static js_Ast *bitxor(js_State *J, int notin)
{
	js_Ast *a = bitand(J, notin);
	while (accept(J, '^'))
		a = EXP2(BITXOR, a, bitand(J, notin));
	return a;
}

static js_Ast *bitor(js_State *J, int notin)
{
	js_Ast *a = bitxor(J, notin);
	while (accept(J, '|'))
		a = EXP2(BITOR, a, bitxor(J, notin));
	return a;
}

static js_Ast *logand(js_State *J, int notin)
{
	js_Ast *a = bitor(J, notin);
	while (accept(J, TK_AND))
		a = EXP2(LOGAND, a, bitor(J, notin));
	return a;
}

static js_Ast *logor(js_State *J, int notin)
{
	js_Ast *a = logand(J, notin);
	while (accept(J, TK_OR))
		a = EXP2(LOGOR, a, logand(J, notin));
	return a;
}

static js_Ast *conditional(js_State *J, int notin)
{
	js_Ast *a, *b, *c;
	a = logor(J, notin);
	if (accept(J, '?')) {
		b = assignment(J, notin);
		expect(J, ':');
		c = assignment(J, notin);
		return EXP3(COND, a, b, c);
	}
	return a;
}

static js_Ast *assignment(js_State *J, int notin)
{
	js_Ast *a = conditional(J, notin);
	if (accept(J, '=')) return EXP2(ASS, a, assignment(J, notin));
	if (accept(J, TK_MUL_ASS)) return EXP2(ASS_MUL, a, assignment(J, notin));
	if (accept(J, TK_DIV_ASS)) return EXP2(ASS_DIV, a, assignment(J, notin));
	if (accept(J, TK_MOD_ASS)) return EXP2(ASS_MOD, a, assignment(J, notin));
	if (accept(J, TK_ADD_ASS)) return EXP2(ASS_ADD, a, assignment(J, notin));
	if (accept(J, TK_SUB_ASS)) return EXP2(ASS_SUB, a, assignment(J, notin));
	if (accept(J, TK_SHL_ASS)) return EXP2(ASS_SHL, a, assignment(J, notin));
	if (accept(J, TK_SHR_ASS)) return EXP2(ASS_SHR, a, assignment(J, notin));
	if (accept(J, TK_USHR_ASS)) return EXP2(ASS_USHR, a, assignment(J, notin));
	if (accept(J, TK_AND_ASS)) return EXP2(ASS_BITAND, a, assignment(J, notin));
	if (accept(J, TK_XOR_ASS)) return EXP2(ASS_BITXOR, a, assignment(J, notin));
	if (accept(J, TK_OR_ASS)) return EXP2(ASS_BITOR, a, assignment(J, notin));
	return a;
}

static js_Ast *expression(js_State *J, int notin)
{
	js_Ast *a = assignment(J, notin);
	while (accept(J, ','))
		a = EXP2(COMMA, a, assignment(J, notin));
	return a;
}

void statement(js_State *J)
{
	js_Ast *exp = expression(J, 1);
	semicolon(J);
	printast(exp);
	putchar(';');
	putchar('\n');
}

static js_Ast *block(js_State *J)
{
	expect(J, '{');
	expect(J, '}');
	return NULL;
}

void program(js_State *J)
{
	next(J);
	while (J->lookahead != 0)
		statement(J);
}

int jsP_error(js_State *J, const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "syntax error: %s:%d: ", J->yyfilename, J->yyline);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");

	longjmp(J->jb, 1);
	return 0;
}

int jsP_parse(js_State *J)
{
	if (setjmp(J->jb)) {
		jsP_freeast(J);
		return 1;
	}

	program(J);

	// TODO: compile to bytecode

	jsP_freeast(J);
	return 0;
}
