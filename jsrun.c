#include "js.h"
#include "jsobject.h"
#include "jscompile.h"
#include "jsrun.h"
#include "jsstate.h"

static js_Value stack[256];
static int top = 0;
static int bot = 0;

static void js_call(js_State *J, int argc);

static void js_dumpstack(js_State *J)
{
	int i;
	for (i = 0; i < top; ++i) {
		printf("stack %d: ", i);
		js_dumpvalue(J, stack[i]);
		putchar('\n');
	}
}

static inline double tointeger(double n)
{
	double sign = n < 0 ? -1 : 1;
	if (isnan(n)) return 0;
	if (n == 0 || isinf(n)) return n;
	return sign * floor(abs(n));
}

static inline int toint32(double n)
{
	double two32 = 4294967296.0;
	double two31 = 2147483648.0;

	if (!isfinite(n) || n == 0)
		return 0;

	n = fmod(n, two32);
	n = n >= 0 ? floor(n) : ceil(n) + two32;
	if (n >= two31)
		return n - two32;
	else
		return n;
}

static inline unsigned int touint32(double n)
{
	return toint32(n);
}

static void js_pushvalue(js_State *J, js_Value v)
{
	stack[top] = v;
	++top;
}

void js_pushundefined(js_State *J)
{
	stack[top].type = JS_TUNDEFINED;
	++top;
}

void js_pushnull(js_State *J)
{
	stack[top].type = JS_TNULL;
	++top;
}

void js_pushboolean(js_State *J, int v)
{
	stack[top].type = JS_TBOOLEAN;
	stack[top].u.boolean = !!v;
	++top;
}

void js_pushnumber(js_State *J, double v)
{
	stack[top].type = JS_TNUMBER;
	stack[top].u.number = v;
	++top;
}

void js_pushlstring(js_State *J, const char *v)
{
	stack[top].type = JS_TSTRING;
	stack[top].u.string = v;
	++top;
}

void js_pushstring(js_State *J, const char *v)
{
	stack[top].type = JS_TSTRING;
	stack[top].u.string = js_intern(J, v);
	++top;
}

void js_pushobject(js_State *J, js_Object *v)
{
	stack[top].type = JS_TOBJECT;
	stack[top].u.object = v;
	++top;
}

static int stackidx(js_State *J, int idx)
{
	if (idx < 0)
		return top + idx;
	return bot + idx;
}

int js_isundefined(js_State *J, int idx)
{
	idx = stackidx(J, idx);
	return stack[idx].type == JS_TUNDEFINED;
}

int js_isstring(js_State *J, int idx)
{
	idx = stackidx(J, idx);
	return stack[idx].type == JS_TSTRING;
}

js_Value js_tovalue(js_State *J, int idx)
{
	idx = stackidx(J, idx);
	return stack[idx];
}

int js_toboolean(js_State *J, int idx)
{
	const char *s;
	double n;
	idx = stackidx(J, idx);
	switch (stack[idx].type) {
	case JS_TUNDEFINED: return 0;
	case JS_TNULL: return 0;
	case JS_TBOOLEAN: return stack[idx].u.boolean;
	case JS_TNUMBER: n = stack[idx].u.number; return n != 0 || !isnan(n);
	case JS_TSTRING: s = stack[idx].u.string; return s[0] != 0;
	case JS_TOBJECT: return 0;
	}
	return 0;
}

double js_tonumber(js_State *J, int idx)
{
	idx = stackidx(J, idx);
	switch (stack[idx].type) {
	case JS_TUNDEFINED: return NAN;
	case JS_TNULL: return 0;
	case JS_TBOOLEAN: return stack[idx].u.boolean;
	case JS_TNUMBER: return stack[idx].u.number;
	case JS_TSTRING: return strtod(stack[idx].u.string, NULL);
	case JS_TOBJECT: return 0;
	}
	return 0;
}

double js_tointeger(js_State *J, int idx)
{
	return toint32(js_tonumber(J, idx));
}

const char *js_tostring(js_State *J, int idx)
{
	char buf[20];
	idx = stackidx(J, idx);
	switch (stack[idx].type) {
	case JS_TUNDEFINED: return "undefined";
	case JS_TNULL: return "null";
	case JS_TBOOLEAN: return stack[idx].u.boolean ? "true" : "false";
	case JS_TNUMBER: sprintf(buf, "%.9g", stack[idx].u.number); return js_intern(J, buf);
	case JS_TSTRING: return stack[idx].u.string;
	case JS_TOBJECT: return "<object>";
	}
	return NULL;
}

js_Object *js_toobject(js_State *J, int idx)
{
	idx = stackidx(J, idx);
	switch (stack[idx].type) {
	case JS_TUNDEFINED: jsR_error(J, "TypeError (undefined)");
	case JS_TNULL: jsR_error(J, "TypeError (null)");
	case JS_TBOOLEAN: jsR_error(J, "new Boolean()");
	case JS_TNUMBER: jsR_error(J, "new Number()");
	case JS_TSTRING: jsR_error(J, "new String()");
	case JS_TOBJECT: return stack[idx].u.object;
	}
	return NULL;
}

void js_pop(js_State *J, int n)
{
	top -= n;
}

void js_dup(js_State *J)
{
	stack[top] = stack[top-1];
	++top;
}

void js_dup2(js_State *J)
{
	stack[top] = stack[top-2];
	stack[top+1] = stack[top-1];
	top += 2;
}

void js_rot2(js_State *J)
{
	/* A B -> B A */
	js_Value tmp = stack[top-1];	/* A B (B) */
	stack[top-1] = stack[top-2];	/* A A */
	stack[top-2] = tmp;		/* B A */
}

void js_rot3(js_State *J)
{
	/* A B C -> C A B */
	js_Value tmp = stack[top-1];	/* A B C (C) */
	stack[top-1] = stack[top-2];	/* A B B */
	stack[top-2] = stack[top-3];	/* A A B */
	stack[top-3] = tmp;		/* C A B */
}

void js_rot3pop2(js_State *J)
{
	/* A B C -> C */
	stack[top-3] = stack[top-1];
	top -= 2;
}

void js_dup1rot4(js_State *J)
{
	/* A B C -> C A B C */
	stack[top] = stack[top-1];	/* A B C C */
	stack[top-1] = stack[top-2];	/* A B B C */
	stack[top-2] = stack[top-3];	/* A A B C */
	stack[top-3] = stack[top];	/* C A B C */
	++top;
}

void js_trap(js_State *J)
{
	js_dumpstack(J);
}

static void runfun(js_State *J, js_Function *F, js_Environment *E)
{
	js_Function **FT = F->funtab;
	double *NT = F->numtab;
	const char **ST = F->strtab;
	short *pcstart = F->code;
	short *pc = F->code;
	int opcode, offset;

	const char *str;
	js_Object *obj;
	js_Property *ref;
	double x, y;
	int b;

	while (1) {
		opcode = *pc++;
		switch (opcode) {
		case OP_POP: js_pop(J, 1); break;
		case OP_DUP: js_dup(J); break;
		case OP_DUP2: js_dup2(J); break;
		case OP_ROT2: js_rot2(J); break;
		case OP_ROT3: js_rot3(J); break;
		case OP_DUP1ROT4: js_dup1rot4(J); break;

		case OP_NUMBER_0: js_pushnumber(J, 0); break;
		case OP_NUMBER_1: js_pushnumber(J, 1); break;
		case OP_NUMBER_X: js_pushnumber(J, *pc++); break;
		case OP_NUMBER: js_pushnumber(J, NT[*pc++]); break;
		case OP_STRING: js_pushlstring(J, ST[*pc++]); break;

		case OP_CLOSURE: js_pushobject(J, js_newfunction(J, FT[*pc++], E)); break;
		case OP_NEWOBJECT: js_pushobject(J, js_newobject(J, JS_COBJECT)); break;
		case OP_NEWARRAY: js_pushobject(J, js_newobject(J, JS_CARRAY)); break;

		case OP_UNDEF: js_pushundefined(J); break;
		case OP_NULL: js_pushnull(J); break;
		case OP_TRUE: js_pushboolean(J, 1); break;
		case OP_FALSE: js_pushboolean(J, 0); break;

		case OP_THIS: js_pushnull(J); break;

		case OP_FUNDEC:
			ref = js_decvar(J, E, ST[*pc++]);
			if (ref)
				ref->value = js_tovalue(J, -1);
			js_pop(J, 1);
			break;

		case OP_VARDEC:
			ref = js_decvar(J, E, ST[*pc++]);
			break;

		case OP_GETVAR:
			str = ST[*pc++];
			ref = js_getvar(J, E, str);
			if (ref)
				js_pushvalue(J, ref->value);
			else
				jsR_error(J, "ReferenceError (%s)", str);
			break;

		case OP_SETVAR:
			ref = js_setvar(J, E, ST[*pc++]);
			if (ref)
				ref->value = js_tovalue(J, -1);
			break;

		// OP_DELVAR

		case OP_IN:
			str = js_tostring(J, -2);
			obj = js_toobject(J, -1);
			ref = js_getproperty(J, obj, str);
			js_pop(J, 2);
			js_pushboolean(J, ref != NULL);
			break;

		case OP_GETPROP:
			obj = js_toobject(J, -2);
			str = js_tostring(J, -1);
			ref = js_getproperty(J, obj, str);
			js_pop(J, 2);
			if (ref)
				js_pushvalue(J, ref->value);
			else
				js_pushundefined(J);
			break;

		case OP_SETPROP:
			obj = js_toobject(J, -3);
			str = js_tostring(J, -2);
			ref = js_setproperty(J, obj, str);
			if (ref)
				ref->value = js_tovalue(J, -1);
			js_rot3pop2(J);
			break;

		// OP_DELPROP

		case OP_NEXTPROP:
			obj = js_toobject(J, -2);
			if (js_isundefined(J, -1))
				ref = js_firstproperty(J, obj);
			else
				ref = js_nextproperty(J, obj, js_tostring(J, -1));
			if (ref) {
				js_pop(J, 1);
				js_pushlstring(J, ref->name);
				js_pushboolean(J, 1);
			} else {
				js_pop(J, 2);
				js_pushboolean(J, 0);
			}
			break;

		case OP_CALL:
			js_call(J, *pc++);
			break;

		/* Unary expressions */

		case OP_POS:
			x = js_tonumber(J, -1);
			js_pop(J, 1);
			js_pushnumber(J, x);
			break;

		case OP_NEG:
			x = js_tonumber(J, -1);
			js_pop(J, 1);
			js_pushnumber(J, -x);
			break;

		case OP_BITNOT:
			x = js_tonumber(J, -1);
			js_pop(J, 1);
			js_pushnumber(J, ~toint32(x));
			break;

		case OP_LOGNOT:
			b = js_toboolean(J, -1);
			js_pop(J, 1);
			js_pushnumber(J, !b);
			break;

		/* Binary expressions */

		case OP_ADD:
			// TODO: check string concatenation
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, x + y);
			break;

		case OP_SUB:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, x - y);
			break;

		case OP_MUL:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, x * y);
			break;

		case OP_DIV:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, x / y);
			break;

		case OP_MOD:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, fmod(x, y));
			break;

		case OP_SHL:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, toint32(x) << (touint32(y) & 0x1F));
			break;

		case OP_SHR:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, toint32(x) >> (touint32(y) & 0x1F)); break;
			break;

		case OP_USHR:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, touint32(x) >> (touint32(y) & 0x1F)); break;
			break;

		case OP_BITAND:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, toint32(x) & toint32(y));
			break;

		case OP_BITXOR:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, toint32(x) ^ toint32(y));
			break;

		case OP_BITOR:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, toint32(x) | toint32(y));
			break;

		/* Relational expressions */

		/* TODO: string comparisons */
		case OP_LT:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushboolean(J, x < y);
			break;
		case OP_GT:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushboolean(J, x > y);
			break;
		case OP_LE:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushboolean(J, x <= y);
			break;
		case OP_GE:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushboolean(J, x >= y);
			break;

		case OP_EQ:
		case OP_STRICTEQ:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushboolean(J, x == y);
			break;
		case OP_NE:
		case OP_STRICTNE:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushboolean(J, x != y);
			break;

		/* Branching */

		case OP_DEBUGGER:
			js_trap(J);
			break;

		case OP_JUMP:
			pc = pcstart + *pc;
			break;

		case OP_JTRUE:
			offset = *pc++;
			b = js_toboolean(J, -1);
			js_pop(J, 1);
			if (b)
				pc = pcstart + offset;
			break;

		case OP_JFALSE:
			offset = *pc++;
			b = js_toboolean(J, -1);
			js_pop(J, 1);
			if (!b)
				pc = pcstart + offset;
			break;

		case OP_RETURN:
			return;

		default:
			fprintf(stderr, "illegal instruction: %d (pc=%d)\n", opcode, (int)(pc - F->code - 1));
			return;
		}
	}
}

static void js_call(js_State *J, int argc)
{
	js_Environment *E;
	js_Property *ref;
	js_Function *F;
	js_Object *obj;
	int i;
	int savebot = bot;

	bot = top - argc;

	obj = js_toobject(J, -argc - 1);

	F = obj->function;
	if (F) {
		E = js_newenvironment(J, obj->scope, js_newobject(J, JS_COBJECT));

		for (i = 0; i < F->numparams; ++i) {
			ref = js_decvar(J, E, F->params[i]);
			if (i + 1 < argc)
				ref->value = js_tovalue(J, i + 1);
		}
		js_pop(J, argc + 1);

		runfun(J, F, E);
	} else {
		js_CFunction CF = obj->cfunction;
		i = CF(J, argc);
		if (i) {
			js_Value v = js_tovalue(J, -1);
			js_pop(J, top - bot + 1);
			js_pushvalue(J, v);
		} else {
			js_pop(J, top - bot + 1);
			js_pushundefined(J);
		}
	}

	bot = savebot;
}

void js_setglobal(js_State *J, const char *name)
{
	js_Property *ref = js_setproperty(J, J->E->variables, name);
	ref->value = js_tovalue(J, -1);
	js_pop(J, 1);
}

void jsR_error(js_State *J, const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "runtime error: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");

	longjmp(J->jb, 1);
}

void jsR_runfunction(js_State *J, js_Function *F)
{
	if (setjmp(J->jb)) {
		js_dumpobject(J, J->E->variables);
		return;
	}

	runfun(J, F, J->E);

	js_dumpobject(J, J->E->variables);
	js_dumpstack(J);
}
