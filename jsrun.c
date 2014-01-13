#include "js.h"
#include "jsvalue.h"
#include "jsobject.h"
#include "jscompile.h"
#include "jsrun.h"
#include "jsstate.h"

static js_Value stack[256];
static int top = 0;

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

static inline void push(js_State *J, js_Value v)
{
	stack[top++] = v;
}

static inline js_Value pop(js_State *J)
{
	return stack[--top];
}

static inline js_Value peek(js_State *J)
{
	return stack[top-1];
}

static inline void pushnumber(js_State *J, double number)
{
	js_Value v;
	v.type = JS_TNUMBER;
	v.u.number = number;
	push(J, v);
}

static inline double popnumber(js_State *J)
{
	js_Value v = pop(J);
	if (v.type == JS_TNUMBER)
		return v.u.number;
	if (v.type == JS_TSTRING)
		return strtod(v.u.string, 0);
	return 0;
}

static inline void pushundefined(js_State *J)
{
	js_Value v;
	v.type = JS_TUNDEFINED;
	push(J, v);
}

static inline void pushnull(js_State *J)
{
	js_Value v;
	v.type = JS_TNULL;
	push(J, v);
}

static inline void pushboolean(js_State *J, int boolean)
{
	js_Value v;
	v.type = JS_TBOOLEAN;
	v.u.boolean = boolean;
	push(J, v);
}

static inline int popboolean(js_State *J)
{
	js_Value v = pop(J);
	if (v.type == JS_TBOOLEAN)
		return v.u.boolean;
	if (v.type == JS_TNUMBER)
		return v.u.number != 0;
	return 0;
}

static inline void pushreference(js_State *J, js_Property *reference)
{
	js_Value v;
	v.type = JS_TREFERENCE;
	v.u.reference = reference;
	push(J, v);
}

static inline js_Property *popreference(js_State *J)
{
	js_Value v = pop(J);
	if (v.type == JS_TREFERENCE)
		return v.u.reference;
	return 0;
}

static inline js_Property *peekreference(js_State *J)
{
	js_Value v = peek(J);
	if (v.type == JS_TREFERENCE)
		return v.u.reference;
	return 0;
}

#define UNARY(X) a = popnumber(J); pushnumber(J, X)
#define BINARY(X) b = popnumber(J); a = popnumber(J); pushnumber(J, X)

static void runfun(js_State *J, js_Function *F, js_Object *E)
{
	short *pc = F->code;
	int opcode, addr;
	js_Property *p;
	js_Value v;
	double a, b;
	const char *s;

	while (1) {
		opcode = *pc++;
		switch (opcode) {
		case OP_NUMBER:
			pushnumber(J, F->numlist[*pc++]);
			break;

		case OP_LOADVAR:
			s = F->strlist[*pc++];
			p = js_getproperty(J, E, s);
			if (p)
				push(J, p->value);
			else
				pushundefined(J);
			break;

		case OP_AVAR:
			s = F->strlist[*pc++];
			p = js_setproperty(J, E, s);
			pushreference(J, p);
			break;

		case OP_LOAD:
			p = peekreference(J);
			if (p)
				push(J, p->value);
			else
				pushundefined(J);
			break;

		case OP_STORE:
			v = pop(J);
			p = popreference(J);
			if (p)
				p->value = v;
			push(J, v);
			break;

		case OP_UNDEF: pushundefined(J); break;
		case OP_NULL: pushnull(J); break;
		case OP_TRUE: pushboolean(J, 1); break;
		case OP_FALSE: pushboolean(J, 0); break;

		case OP_POS: UNARY(a); break;
		case OP_NEG: UNARY(-a); break;
		case OP_BITNOT: UNARY(~i32(a)); break;
		case OP_LOGNOT: UNARY(!a); break;

		case OP_ADD: BINARY(a + b); break;
		case OP_SUB: BINARY(a - b); break;
		case OP_MUL: BINARY(a * b); break;
		case OP_DIV: BINARY(a / b); break;
		case OP_MOD: BINARY(fmod(a, b)); break;
		case OP_SHL: BINARY(i32(a) << (u32(b) & 0x1F)); break;
		case OP_SHR: BINARY(i32(a) >> (u32(b) & 0x1F)); break;
		case OP_USHR: BINARY(u32(a) >> (u32(b) & 0x1F)); break;
		case OP_BITAND: BINARY(i32(a) & i32(b)); break;
		case OP_BITXOR: BINARY(i32(a) ^ i32(b)); break;
		case OP_BITOR: BINARY(i32(a) | i32(b)); break;

		case OP_LT: BINARY(a < b); break;
		case OP_GT: BINARY(a > b); break;
		case OP_LE: BINARY(a <= b); break;
		case OP_GE: BINARY(a >= b); break;
		case OP_EQ: BINARY(a == b); break;
		case OP_NE: BINARY(a != b); break;

		case OP_JFALSE:
			addr = *pc++;
			if (!popboolean(J))
				pc = F->code + addr;
			break;

		case OP_JTRUE:
			addr = *pc++;
			if (popboolean(J))
				pc = F->code + addr;
			break;

		case OP_JUMP:
			pc = F->code + *pc;
			break;

		case OP_POP: pop(J); break;
		case OP_RETURN: return;

		default:
			fprintf(stderr, "illegal instruction: %d (pc=%d)\n", opcode, (int)(pc - F->code - 1));
			return;
		}
	}
}

void jsR_runfunction(js_State *J, js_Function *F)
{
	js_Object *varenv = js_newobject(J);
	runfun(J, F, varenv);
	js_dumpobject(J, varenv);
}
