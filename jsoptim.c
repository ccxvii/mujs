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

static int N(js_Ast *node, double x)
{
	node->type = AST_NUMBER;
	node->number = x;
	node->a = node->b = node->c = node->d = NULL;
	return 1;
}

static int foldnumber(js_Ast *node)
{
	double x, y;
	int a, b;

	if (node->type == AST_NUMBER)
		return 1;

	a = node->a ? foldnumber(node->a) : 0;
	b = node->b ? foldnumber(node->b) : 0;
	if (node->c) foldnumber(node->c);
	if (node->d) foldnumber(node->d);

	if (a) {
		x = node->a->number;
		switch (node->type) {
		case EXP_NEG: return N(node, -x);
		case EXP_POS: return N(node, x);
		case EXP_BITNOT: return N(node, ~i32(x));
		}

		if (b) {
			y = node->b->number;
			switch (node->type) {
			case EXP_MUL: return N(node, x * y);
			case EXP_DIV: return N(node, x / y);
			case EXP_MOD: return N(node, fmod(x, y));
			case EXP_ADD: return N(node, x + y);
			case EXP_SUB: return N(node, x - y);
			case EXP_SHL: return N(node, i32(x) << (u32(y) & 0x1F));
			case EXP_SHR: return N(node, i32(x) >> (u32(y) & 0x1F));
			case EXP_USHR: return N(node, u32(x) >> (u32(y) & 0x1F));
			case EXP_BITAND: return N(node, i32(x) & i32(y));
			case EXP_BITXOR: return N(node, i32(x) ^ i32(y));
			case EXP_BITOR: return N(node, i32(x) | i32(y));
			}
		}
	}

	return 0;
}

void jsP_optimize(js_State *J, js_Ast *prog)
{
	foldnumber(prog);
}
