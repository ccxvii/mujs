#include "js.h"
#include "jsobject.h"
#include "jsparse.h"
#include "jscompile.h"
#include "jsrun.h"
#include "jsstate.h"

static void jsR_run(js_State *J, js_Function *F);

static js_Value stack[256];
static int top = 0;
static int bot = 0;

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

void js_pushstring(js_State *J, const char *v)
{
	stack[top].type = JS_TSTRING;
	stack[top].u.string = js_intern(J, v);
	++top;
}

void js_pushliteral(js_State *J, const char *v)
{
	stack[top].type = JS_TSTRING;
	stack[top].u.string = v;
	++top;
}

void js_pushobject(js_State *J, js_Object *v)
{
	stack[top].type = JS_TOBJECT;
	stack[top].u.object = v;
	++top;
}

void js_pushglobal(js_State *J)
{
	js_pushobject(J, J->G);
}

void js_newobject(js_State *J)
{
	js_pushobject(J, jsR_newobject(J, JS_COBJECT, J->Object_prototype));
}

void js_newarray(js_State *J)
{
	js_pushobject(J, jsR_newobject(J, JS_CARRAY, J->Array_prototype));
}

void js_newfunction(js_State *J, js_Function *F, js_Environment *scope)
{
	js_pushobject(J, jsR_newfunction(J, F, scope));
	js_pushnumber(J, F->numparams);
	js_setproperty(J, -2, "length");
	js_newobject(J);
	js_copy(J, -2);
	js_setproperty(J, -2, "constructor");
	js_setproperty(J, -2, "prototype");
}

void js_pushcfunction(js_State *J, js_CFunction fun)
{
	js_pushobject(J, jsR_newcfunction(J, fun));
	// TODO: length property?
	js_newobject(J);
	js_copy(J, -2);
	js_setproperty(J, -2, "constructor");
	js_setproperty(J, -2, "prototype");
}

void js_pushcconstructor(js_State *J, js_CFunction fun, js_CFunction con)
{
	js_pushobject(J, jsR_newcconstructor(J, fun, con));
	js_newobject(J);
	js_copy(J, -2);
	js_setproperty(J, -2, "constructor");
	js_setproperty(J, -2, "prototype");
}

/* Read values from stack */

static const js_Value *stackidx(js_State *J, int idx)
{
	static js_Value undefined = { JS_TUNDEFINED, { 0 } };
	idx = idx < 0 ? top + idx : bot + idx;
	if (idx < 0 || idx >= top)
		return &undefined;
	return stack + idx;
}

int js_isundefined(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TUNDEFINED; }
int js_isnull(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TNULL; }
int js_isboolean(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TBOOLEAN; }
int js_isnumber(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TNUMBER; }
int js_isstring(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TSTRING; }
int js_isprimitive(js_State *J, int idx) { return stackidx(J, idx)->type != JS_TOBJECT; }
int js_isobject(js_State *J, int idx) { return stackidx(J, idx)->type == JS_TOBJECT; }

js_Value js_tovalue(js_State *J, int idx)
{
	return *stackidx(J, idx);
}

int js_toboolean(js_State *J, int idx)
{
	return jsR_toboolean(J, stackidx(J, idx));
}

double js_tonumber(js_State *J, int idx)
{
	return jsR_tonumber(J, stackidx(J, idx));
}

const char *js_tostring(js_State *J, int idx)
{
	return jsR_tostring(J, stackidx(J, idx));
}

js_Object *js_toobject(js_State *J, int idx)
{
	return jsR_toobject(J, stackidx(J, idx));
}

js_Value js_toprimitive(js_State *J, int idx, int hint)
{
	return jsR_toprimitive(J, stackidx(J, idx), hint);
}

/* Stack manipulation */

void js_pop(js_State *J, int n)
{
	top -= n;
}

void js_copy(js_State *J, int idx)
{
	stack[top] = *stackidx(J, idx);
	++top;
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

void js_rot(js_State *J, int n)
{
	int i;
	js_Value tmp = stack[top-1];
	for (i = 1; i <= n; i++)
		stack[top-i] = stack[top-i-1];
	stack[top-i] = tmp;
}

/* Global and object property accessors */

void js_getglobal(js_State *J, const char *name)
{
	js_Property *ref = jsR_getproperty(J, J->G, name);
	if (ref)
		js_pushvalue(J, ref->value);
	else
		js_pushundefined(J);
}

void js_setglobal(js_State *J, const char *name)
{
	js_Property *ref = jsR_setproperty(J, J->G, name);
	if (ref)
		ref->value = js_tovalue(J, -1);
	js_pop(J, 1);
}

void js_getownproperty(js_State *J, int idx, const char *name)
{
	js_Object *obj = js_toobject(J, idx);
	js_Property *ref = jsR_getownproperty(J, obj, name);
	if (ref)
		js_pushvalue(J, ref->value);
	else
		js_pushundefined(J);
}

void js_getproperty(js_State *J, int idx, const char *name)
{
	js_Object *obj = js_toobject(J, idx);
	js_Property *ref = jsR_getproperty(J, obj, name);
	if (ref)
		js_pushvalue(J, ref->value);
	else
		js_pushundefined(J);
}

void js_setproperty(js_State *J, int idx, const char *name)
{
	js_Object *obj = js_toobject(J, idx);
	js_Property *ref = jsR_setproperty(J, obj, name);
	if (ref)
		ref->value = js_tovalue(J, -1);
	js_pop(J, 1);
}

int js_nextproperty(js_State *J, int idx)
{
	js_Object *obj = js_toobject(J, idx);
	js_Property *ref = jsR_nextproperty(J, obj, js_tostring(J, -1));
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
	E->outer = outer;
	E->variables = vars;
	return E;
}

static js_Property *js_decvar(js_State *J, const char *name)
{
	return jsR_setproperty(J, J->E->variables, name);
}

static js_Property *js_getvar(js_State *J, const char *name)
{
	js_Environment *E = J->E;
	do {
		js_Property *ref = jsR_getproperty(J, E->variables, name);
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
		js_Property *ref = jsR_getproperty(J, E->variables, name);
		if (ref)
			return ref;
		E = E->outer;
	} while (E);
	return jsR_setproperty(J, J->G, name);
}

/* Function calls */

static void jsR_callfunction(js_State *J, int n, js_Function *F, js_Environment *scope)
{
	js_Environment *saveE;
	int i;

	saveE = J->E;

	J->E = jsR_newenvironment(J, jsR_newobject(J, JS_COBJECT, NULL), scope);
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
	int rv = F(J, n + 1);
	if (rv) {
		js_Value v = js_tovalue(J, -1);
		js_pop(J, top - bot + 1);
		js_pushvalue(J, v);
	} else {
		js_pop(J, top - bot + 1);
		js_pushundefined(J);
	}
}

void js_call(js_State *J, int n)
{
	js_Object *obj = js_toobject(J, -n - 2);
	int savebot = bot;
	bot = top - n - 1;
	if (obj->type == JS_CFUNCTION)
		jsR_callfunction(J, n, obj->function, obj->scope);
	else if (obj->type == JS_CSCRIPT)
		jsR_callscript(J, n, obj->function);
	else if (obj->type == JS_CCFUNCTION)
		jsR_callcfunction(J, n, obj->cfunction);
	else
		jsR_error(J, "TypeError (not a function)");
	bot = savebot;
}

void js_construct(js_State *J, int n)
{
	js_Object *obj = js_toobject(J, -n - 1);
	js_Object *prototype;

	/* built-in constructors create their own objects */
	if (obj->type == JS_CCFUNCTION && obj->cconstructor) {
		int savebot = bot;
		bot = top - n;
		jsR_callcfunction(J, n, obj->cconstructor);
		bot = savebot;
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
	js_pushobject(J, jsR_newobject(J, JS_COBJECT, prototype));
	if (n > 0)
		js_rot(J, n + 1);

	/* call the function */
	js_call(J, n);
}

/* Main interpreter loop */

void jsR_dumpstack(js_State *J)
{
	int i;
	printf("stack {\n");
	for (i = 0; i < top; ++i) {
		putchar(i == bot ? '>' : ' ');
		printf("% 4d: ", i);
		js_dumpvalue(J, stack[i]);
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
	fprintf(stderr, "trap at %d in ", pc);
	js_Function *F = stack[bot-1].u.object->function;
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
				jsR_error(J, "ReferenceError (%s)", str);
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
			ref = jsR_getproperty(J, obj, str);
			js_pop(J, 2);
			js_pushboolean(J, ref != NULL);
			break;

		case OP_GETPROP:
			str = js_tostring(J, -1);
			js_pop(J, 1);
			js_getproperty(J, -1, str);

			obj = js_toobject(J, -2);
			ref = jsR_getproperty(J, obj, str);
			js_pop(J, 2);
			if (ref)
				js_pushvalue(J, ref->value);
			else
				js_pushundefined(J);
			break;

		case OP_SETPROP:
			obj = js_toobject(J, -3);
			str = js_tostring(J, -2);
			ref = jsR_setproperty(J, obj, str);
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

			ref = jsR_nextproperty(J, obj, str);
			if (!ref && obj->prototype) {
				obj = obj->prototype;
				ref = jsR_nextproperty(J, obj, NULL);
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

		case OP_CALL:
			js_call(J, *pc++);
			break;

		case OP_NEW:
			js_construct(J, *pc++);
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
			js_pushboolean(J, !b);
			break;

		/* Binary expressions */

		case OP_ADD:
			{
				js_Value va = js_toprimitive(J, -2, JS_HNONE);
				js_Value vb = js_toprimitive(J, -1, JS_HNONE);
				if (va.type == JS_TSTRING || vb.type == JS_TSTRING) {
					const char *sa = jsR_tostring(J, &va);
					const char *sb = jsR_tostring(J, &vb);
					char *sab = malloc(strlen(sa) + strlen(sb) + 1);
					strcpy(sab, sa);
					strcat(sab, sb);
					js_pop(J, 2);
					js_pushstring(J, sab);
					free(sab);
				} else {
					x = jsR_tonumber(J, &va);
					y = jsR_tonumber(J, &vb);
					js_pop(J, 2);
					js_pushnumber(J, x + y);
				}
			}
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
			fprintf(stderr, "illegal instruction: %d (pc=%d)\n", opcode, (int)(pc - F->code - 1));
			return;
		}
	}
}

int jsR_loadscript(js_State *J, const char *filename, const char *source)
{
	js_Ast *P;
	js_Function *F;

	// TODO: push exception stack

	P = jsP_parse(J, filename, source);
	if (!P) return 1;
	jsP_optimize(J, P);
	F = jsC_compile(J, P);
	jsP_freeparse(J);
	if (!F) return 1;

	js_pushobject(J, jsR_newscript(J, F));
	return 0;
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
