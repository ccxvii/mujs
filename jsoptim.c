#include "js.h"
#include "jsparse.h"

static inline int i32(double d)
{
	double two32 = 4294967296.0;
	double two31 = 2147483648.0;

	if (!isfinite(d) || d == 0)
		return 0;

	d = fmod(d, two32);
	d = d >= 0 ? floor(d) : ceil(d) + two32;
	if (d >= two31)
		return d - two32;
	else
		return d;
}

static inline unsigned int u32(double d)
{
	return i32(d);
}

static int N(js_Ast *node, double *r, double x)
{
	node->type = AST_NUMBER;
	node->number = x;
	node->a = node->b = node->c = node->d = NULL;
	*r = x;
	return 1;
}

static int foldnumber(js_Ast *node, double *r)
{
	double x, y, z, w;
	int a, b;

	a = node->a ? foldnumber(node->a, &x) : 0;
	b = node->b ? foldnumber(node->b, &y) : 0;
	if (node->c) foldnumber(node->c, &z);
	if (node->d) foldnumber(node->d, &w);

	if (node->type == AST_NUMBER) {
		*r = node->number;
		return 1;
	}

	/* not an expression */
	if (node->c || node->d)
		return 0;

	/* binary */
	if (a && b) {
		switch (node->type) {
		case EXP_MUL: return N(node, r, x * y);
		case EXP_DIV: return N(node, r, x / y);
		case EXP_MOD: return N(node, r, fmod(x, y));
		case EXP_ADD: return N(node, r, x + y);
		case EXP_SUB: return N(node, r, x - y);
		case EXP_SHL: return N(node, r, i32(x) << (u32(y) & 0x1F));
		case EXP_SHR: return N(node, r, i32(x) >> (u32(y) & 0x1F));
		case EXP_USHR: return N(node, r, u32(x) >> (u32(y) & 0x1F));
		case EXP_BITAND: return N(node, r, i32(x) & i32(y));
		case EXP_BITXOR: return N(node, r, i32(x) ^ i32(y));
		case EXP_BITOR: return N(node, r, i32(x) | i32(y));
		}
	}

	/* unary */
	else if (a) {
		switch (node->type) {
		case EXP_NEG: return N(node, r, -x);
		case EXP_POS: return N(node, r, x);
		case EXP_BITNOT: return N(node, r, ~i32(x));
		}
	}

	return 0;
}

void jsP_optimize(js_State *J, js_Ast *prog)
{
	double x;
	foldnumber(prog, &x);
}
