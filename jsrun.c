#include "jsi.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsrun.h"

static void jsR_run(js_State *J, js_Function *F);

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

/* Push values on stack */

#define STACK (J->stack)
#define TOP (J->top)
#define BOT (J->bot)

void js_pushvalue(js_State *J, js_Value v)
{
	STACK[TOP] = v;
	++TOP;
}

void js_pushundefined(js_State *J)
{
	STACK[TOP].type = JS_TUNDEFINED;
	++TOP;
}

void js_pushnull(js_State *J)
{
	STACK[TOP].type = JS_TNULL;
	++TOP;
}

void js_pushboolean(js_State *J, int v)
{
	STACK[TOP].type = JS_TBOOLEAN;
	STACK[TOP].u.boolean = !!v;
	++TOP;
}

void js_pushnumber(js_State *J, double v)
{
	STACK[TOP].type = JS_TNUMBER;
	STACK[TOP].u.number = v;
	++TOP;
}

void js_pushstring(js_State *J, const char *v)
{
	STACK[TOP].type = JS_TSTRING;
	STACK[TOP].u.string = js_intern(J, v);
	++TOP;
}

void js_pushliteral(js_State *J, const char *v)
{
	STACK[TOP].type = JS_TSTRING;
	STACK[TOP].u.string = v;
	++TOP;
}

void js_pushobject(js_State *J, js_Object *v)
{
	STACK[TOP].type = JS_TOBJECT;
	STACK[TOP].u.object = v;
	++TOP;
}

void js_pushglobal(js_State *J)
{
	js_pushobject(J, J->G);
}

/* Read values from stack */

static const js_Value *stackidx(js_State *J, int idx)
{
	static js_Value undefined = { JS_TUNDEFINED, { 0 } };
	idx = idx < 0 ? TOP + idx : BOT + idx;
	if (idx < 0 || idx >= TOP)
		return &undefined;
	return STACK + idx;
}

int js_isundefined(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TUNDEFINED; }
int js_isnull(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TNULL; }
int js_isboolean(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TBOOLEAN; }
int js_isnumber(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TNUMBER; }
int js_isstring(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TSTRING; }
int js_isprimitive(js_State *J, int idx) { return stackidx(J, idx)->type != JS_TOBJECT; }
int js_isobject(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TOBJECT; }

int js_iscallable(js_State *J, int idx)
{
	const js_Value *v = stackidx(J, idx);
	if (v->type == JS_TOBJECT)
		return v->u.object->type == JS_CFUNCTION || v->u.object->type == JS_CCFUNCTION;
	return 0;
}

static const char *js_typeof(js_State *J, int idx)
{
	switch (stackidx(J, idx)->type) {
	case JS_TUNDEFINED: return "undefined";
	case JS_TNULL: return "object";
	case JS_TBOOLEAN: return "boolean";
	case JS_TNUMBER: return "number";
	case JS_TSTRING: return "string";
	case JS_TOBJECT: return "object";
	}
	return "object";
}

js_Value js_tovalue(js_State *J, int idx)
{
	return *stackidx(J, idx);
}

int js_toboolean(js_State *J, int idx)
{
	return jsV_toboolean(J, stackidx(J, idx));
}

double js_tonumber(js_State *J, int idx)
{
	return jsV_tonumber(J, stackidx(J, idx));
}

double js_tointeger(js_State *J, int idx)
{
	return tointeger(jsV_tonumber(J, stackidx(J, idx)));
}

int js_toint32(js_State *J, int idx)
{
	return toint32(jsV_tonumber(J, stackidx(J, idx)));
}

unsigned int js_touint32(js_State *J, int idx)
{
	return touint32(jsV_tonumber(J, stackidx(J, idx)));
}

const char *js_tostring(js_State *J, int idx)
{
	return jsV_tostring(J, stackidx(J, idx));
}

js_Object *js_toobject(js_State *J, int idx)
{
	return jsV_toobject(J, stackidx(J, idx));
}

js_Value js_toprimitive(js_State *J, int idx, int hint)
{
	return jsV_toprimitive(J, stackidx(J, idx), hint);
}

/* Stack manipulation */

void js_pop(js_State *J, int n)
{
	TOP -= n;
	if (TOP < BOT) {
		TOP = BOT;
		js_error(J, "stack underflow!");
	}
}

void js_copy(js_State *J, int idx)
{
	STACK[TOP] = *stackidx(J, idx);
	++TOP;
}

void js_dup(js_State *J)
{
	STACK[TOP] = STACK[TOP-1];
	++TOP;
}

void js_dup2(js_State *J)
{
	STACK[TOP] = STACK[TOP-2];
	STACK[TOP+1] = STACK[TOP-1];
	TOP += 2;
}

void js_rot2(js_State *J)
{
	/* A B -> B A */
	js_Value tmp = STACK[TOP-1];	/* A B (B) */
	STACK[TOP-1] = STACK[TOP-2];	/* A A */
	STACK[TOP-2] = tmp;		/* B A */
}

void js_rot3(js_State *J)
{
	/* A B C -> C A B */
	js_Value tmp = STACK[TOP-1];	/* A B C (C) */
	STACK[TOP-1] = STACK[TOP-2];	/* A B B */
	STACK[TOP-2] = STACK[TOP-3];	/* A A B */
	STACK[TOP-3] = tmp;		/* C A B */
}

void js_rot3pop2(js_State *J)
{
	/* A B C -> C */
	STACK[TOP-3] = STACK[TOP-1];
	TOP -= 2;
}

void js_dup1rot4(js_State *J)
{
	/* A B C -> C A B C */
	STACK[TOP] = STACK[TOP-1];	/* A B C C */
	STACK[TOP-1] = STACK[TOP-2];	/* A B B C */
	STACK[TOP-2] = STACK[TOP-3];	/* A A B C */
	STACK[TOP-3] = STACK[TOP];	/* C A B C */
	++TOP;
}

void js_rot(js_State *J, int n)
{
	int i;
	js_Value tmp = STACK[TOP-1];
	for (i = 1; i <= n; i++)
		STACK[TOP-i] = STACK[TOP-i-1];
	STACK[TOP-i] = tmp;
}

/* Global and object property accessors */

void js_getglobal(js_State *J, const char *name)
{
	js_Property *ref = jsV_getproperty(J, J->G, name);
	if (ref)
		js_pushvalue(J, ref->value);
	else
		js_pushundefined(J);
}

void js_setglobal(js_State *J, const char *name)
{
	js_Property *ref = jsV_setproperty(J, J->G, name);
	if (ref)
		ref->value = js_tovalue(J, -1);
	js_pop(J, 1);
}

void js_getownproperty(js_State *J, int idx, const char *name)
{
	js_Object *obj = js_toobject(J, idx);
	js_Property *ref = jsV_getownproperty(J, obj, name);
	if (ref)
		js_pushvalue(J, ref->value);
	else
		js_pushundefined(J);
}

void js_getproperty(js_State *J, int idx, const char *name)
{
	js_Object *obj = js_toobject(J, idx);
	js_Property *ref = jsV_getproperty(J, obj, name);
	if (ref)
		js_pushvalue(J, ref->value);
	else
		js_pushundefined(J);
}

void js_setproperty(js_State *J, int idx, const char *name)
{
	js_Object *obj = js_toobject(J, idx);
	js_Property *ref = jsV_setproperty(J, obj, name);
	if (ref)
		ref->value = js_tovalue(J, -1);
	js_pop(J, 1);
}

int js_nextproperty(js_State *J, int idx)
{
	js_Object *obj = js_toobject(J, idx);
	js_Property *ref = jsV_nextproperty(J, obj, js_tostring(J, -1));
	js_pop(J, 1);
	if (ref) {
		js_pushliteral(J, ref->name);
		js_pushvalue(J, ref->value);
		return 1;
	}
	return 0;
}

/* Environment records */

js_Environment *jsR_newenvironment(js_State *J, js_Object *vars, js_Environment *outer)
{
	js_Environment *E = malloc(sizeof *E);
	E->gcmark = 0;
	E->gcnext = J->gcenv;
	J->gcenv = E;
	++J->gccounter;

	E->outer = outer;
	E->variables = vars;
	return E;
}

static js_Property *js_decvar(js_State *J, const char *name)
{
	return jsV_setproperty(J, J->E->variables, name);
}

static js_Property *js_getvar(js_State *J, const char *name)
{
	js_Environment *E = J->E;
	do {
		js_Property *ref = jsV_getproperty(J, E->variables, name);
		if (ref)
			return ref;
		E = E->outer;
	} while (E);
	return NULL;
}

static js_Property *js_setvar(js_State *J, const char *name)
{
	js_Environment *E = J->E;
	do {
		js_Property *ref = jsV_getproperty(J, E->variables, name);
		if (ref)
			return ref;
		E = E->outer;
	} while (E);
	return jsV_setproperty(J, J->G, name);
}

/* Function calls */

static void jsR_callfunction(js_State *J, int n, js_Function *F, js_Environment *scope)
{
	js_Environment *saveE;
	int i;

	saveE = J->E;

	J->E = jsR_newenvironment(J, jsV_newobject(J, JS_COBJECT, NULL), scope);
	for (i = 0; i < n; i++) {
		js_Property *ref = js_decvar(J, F->params[i]);
		if (i < n)
			ref->value = js_tovalue(J, i + 1);
	}
	js_pop(J, n);

	jsR_run(J, F);
	js_rot3pop2(J);

	J->E = saveE;
}

static void jsR_callscript(js_State *J, int n, js_Function *F)
{
	js_pop(J, n);
	jsR_run(J, F);
	js_rot3pop2(J);
}

static void jsR_callcfunction(js_State *J, int n, js_CFunction F)
{
	int rv = F(J, n);
	if (rv) {
		js_Value v = js_tovalue(J, -1);
		TOP = --BOT; /* pop down to below function */
		js_pushvalue(J, v);
	} else {
		TOP = --BOT; /* pop down to below function */
		js_pushundefined(J);
	}
}

void js_call(js_State *J, int n)
{
	js_Object *obj = js_toobject(J, -n - 2);
	int savebot = BOT;
	BOT = TOP - n - 1;
	if (obj->type == JS_CFUNCTION)
		jsR_callfunction(J, n, obj->u.f.function, obj->u.f.scope);
	else if (obj->type == JS_CSCRIPT)
		jsR_callscript(J, n, obj->u.f.function);
	else if (obj->type == JS_CCFUNCTION)
		jsR_callcfunction(J, n, obj->u.c.function);
	else
		js_typeerror(J, "not a function");
	BOT = savebot;
}

void js_construct(js_State *J, int n)
{
	js_Object *obj = js_toobject(J, -n - 1);
	js_Object *prototype;
	js_Object *newobj;

	/* built-in constructors create their own objects, give them a 'null' this */
	if (obj->type == JS_CCFUNCTION && obj->u.c.constructor) {
		int savebot = BOT;
		js_pushnull(J);
		if (n > 0)
			js_rot(J, n + 1);
		BOT = TOP - n - 1;
		jsR_callcfunction(J, n, obj->u.c.constructor);
		BOT = savebot;
		return;
	}

	/* extract the function object's prototype property */
	js_getproperty(J, -n - 1, "prototype");
	if (js_isobject(J, -1))
		prototype = js_toobject(J, -1);
	else
		prototype = J->Object_prototype;
	js_pop(J, 1);

	/* create a new object with above prototype, and shift it into the 'this' slot */
	newobj = jsV_newobject(J, JS_COBJECT, prototype);
	js_pushobject(J, newobj);
	if (n > 0)
		js_rot(J, n + 1);

	/* call the function */
	js_call(J, n);

	/* if result is not an object, return the original object we created */
	if (!js_isobject(J, -1)) {
		js_pop(J, 1);
		js_pushobject(J, newobj);
	}
}

/* Exceptions */

void js_savetry(js_State *J, short *pc)
{
	if (J->trylen == JS_TRYLIMIT)
		js_error(J, "exception stack overflow");
	J->trybuf[J->trylen].E = J->E;
	J->trybuf[J->trylen].top = J->top;
	J->trybuf[J->trylen].bot = J->bot;
	J->trybuf[J->trylen].pc = pc;
}

void js_throw(js_State *J)
{
	if (J->trylen > 0) {
		js_Value v = js_tovalue(J, -1);
		--J->trylen;
		J->E = J->trybuf[J->trylen].E;
		J->top = J->trybuf[J->trylen].top;
		J->bot = J->trybuf[J->trylen].bot;
		js_pushvalue(J, v);
		longjmp(J->trybuf[J->trylen].buf, 1);
	}
	fprintf(stderr, "libjs: uncaught exception!\n");
	abort();
}

/* Main interpreter loop */

void jsR_dumpstack(js_State *J)
{
	int i;
	printf("stack {\n");
	for (i = 0; i < TOP; ++i) {
		putchar(i == BOT ? '>' : ' ');
		printf("% 4d: ", i);
		js_dumpvalue(J, STACK[i]);
		putchar('\n');
	}
	printf("}\n");
}

void jsR_dumpenvironment(js_State *J, js_Environment *E, int d)
{
	printf("scope %d ", d);
	js_dumpobject(J, E->variables);
	if (E->outer)
		jsR_dumpenvironment(J, E->outer, d+1);
}

void js_trap(js_State *J, int pc)
{
	js_Function *F = STACK[BOT-1].u.object->u.f.function;
	printf("trap at %d in ", pc);
	jsC_dumpfunction(J, F);
	jsR_dumpstack(J);
	jsR_dumpenvironment(J, J->E, 0);
}

static void jsR_run(js_State *J, js_Function *F)
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
		if (J->gccounter > JS_GCLIMIT) {
			J->gccounter = 0;
			js_gc(J, 0);
		}

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
		case OP_STRING: js_pushliteral(J, ST[*pc++]); break;

		case OP_CLOSURE: js_newfunction(J, FT[*pc++], J->E); break;
		case OP_NEWOBJECT: js_newobject(J); break;
		case OP_NEWARRAY: js_newarray(J); break;

		case OP_UNDEF: js_pushundefined(J); break;
		case OP_NULL: js_pushnull(J); break;
		case OP_TRUE: js_pushboolean(J, 1); break;
		case OP_FALSE: js_pushboolean(J, 0); break;

		case OP_THIS: js_copy(J, 0); break;
		case OP_GLOBAL: js_pushobject(J, J->G); break;

		case OP_FUNDEC:
			ref = js_decvar(J, ST[*pc++]);
			if (ref)
				ref->value = js_tovalue(J, -1);
			js_pop(J, 1);
			break;

		case OP_VARDEC:
			ref = js_decvar(J, ST[*pc++]);
			break;

		case OP_GETVAR:
			str = ST[*pc++];
			ref = js_getvar(J, str);
			if (ref)
				js_pushvalue(J, ref->value);
			else
				js_referenceerror(J, "%s", str);
			break;

		case OP_SETVAR:
			ref = js_setvar(J, ST[*pc++]);
			if (ref)
				ref->value = js_tovalue(J, -1);
			break;

		// OP_DELVAR

		case OP_IN:
			str = js_tostring(J, -2);
			obj = js_toobject(J, -1);
			ref = jsV_getproperty(J, obj, str);
			js_pop(J, 2);
			js_pushboolean(J, ref != NULL);
			break;

		case OP_GETPROP:
			str = js_tostring(J, -1);
			js_pop(J, 1);
			js_getproperty(J, -1, str);

			obj = js_toobject(J, -2);
			ref = jsV_getproperty(J, obj, str);
			js_pop(J, 2);
			if (ref)
				js_pushvalue(J, ref->value);
			else
				js_pushundefined(J);
			break;

		case OP_SETPROP:
			obj = js_toobject(J, -3);
			str = js_tostring(J, -2);
			ref = jsV_setproperty(J, obj, str);
			if (ref)
				ref->value = js_tovalue(J, -1);
			js_rot3pop2(J);
			break;

		// OP_DELPROP

		case OP_NEXTPROP:
			obj = js_toobject(J, -2);
			if (js_isundefined(J, -1))
				str = NULL;
			else
				str = js_tostring(J, -1);

			ref = jsV_nextproperty(J, obj, str);
			if (!ref && obj->prototype) {
				obj = obj->prototype;
				ref = jsV_nextproperty(J, obj, NULL);
			}

			js_pop(J, 2);
			if (ref) {
				js_pushobject(J, obj);
				js_pushliteral(J, ref->name);
				js_pushboolean(J, 1);
			} else {
				js_pushboolean(J, 0);
			}
			break;

		/* Function calls */

		case OP_CALL:
			js_call(J, *pc++);
			break;

		case OP_NEW:
			js_construct(J, *pc++);
			break;

		/* Unary operators */

		case OP_TYPEOF:
			str = js_typeof(J, -1);
			js_pop(J, 1);
			js_pushliteral(J, str);
			break;

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
			js_pushboolean(J, !b);
			break;

		/* Multiplicative operators */

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

		/* Additive operators */

		case OP_ADD:
			js_concat(J);
			break;

		case OP_SUB:
			x = js_tonumber(J, -2);
			y = js_tonumber(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, x - y);
			break;

		/* Shift operators */

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

		/* Relational operators */

		case OP_LT: b = js_compare(J); js_pushboolean(J, b < 0); break;
		case OP_GT: b = js_compare(J); js_pushboolean(J, b > 0); break;
		case OP_LE: b = js_compare(J); js_pushboolean(J, b <= 0); break;
		case OP_GE: b = js_compare(J); js_pushboolean(J, b >= 0); break;

		// OP_INSTANCEOF

		/* Equality */

		case OP_EQ: b = js_equal(J); js_pushboolean(J, b); break;
		case OP_NE: b = js_equal(J); js_pushboolean(J, !b); break;
		case OP_STRICTEQ: b = js_strictequal(J); js_pushboolean(J, b); break;
		case OP_STRICTNE: b = js_strictequal(J); js_pushboolean(J, !b); break;

		/* Binary bitwise operators */

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

		/* With */

		case OP_WITH:
			obj = js_toobject(J, -1);
			J->E = jsR_newenvironment(J, obj, J->E);
			js_pop(J, 1);
			break;

		case OP_ENDWITH:
			J->E = J->E->outer;
			break;

		/* Branching */

		case OP_DEBUGGER:
			js_trap(J, (int)(pc - pcstart) - 1);
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
			js_error(J, "illegal instruction: %s (pc=%d)", jsC_opcodestring(opcode), (int)(pc - F->code - 1));
		}
	}
}
