#include "jsi.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsrun.h"

static void jsR_run(js_State *J, js_Function *F);

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

void js_pushlstring(js_State *J, const char *v, int n)
{
	char buf[256];
	if (n + 1 < sizeof buf) {
		memcpy(buf, v, n);
		buf[n] = 0;
		js_pushstring(J, buf);
	} else {
		char *s = malloc(n + 1);
		memcpy(s, v, n);
		s[n] = 0;
		if (js_try(J)) {
			free(s);
			js_throw(J);
		}
		js_pushstring(J, s);
		js_endtry(J);
		free(s);
	}
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
		return v->u.object->type == JS_CFUNCTION ||
			v->u.object->type == JS_CSCRIPT ||
			v->u.object->type == JS_CCFUNCTION;
	return 0;
}

int js_isarray(js_State *J, int idx)
{
	const js_Value *v = stackidx(J, idx);
	return v->type == JS_TOBJECT && v->u.object->type == JS_CARRAY;
}

int js_isregexp(js_State *J, int idx)
{
	const js_Value *v = stackidx(J, idx);
	return v->type == JS_TOBJECT && v->u.object->type == JS_CREGEXP;
}

int js_isiterator(js_State *J, int idx)
{
	const js_Value *v = stackidx(J, idx);
	return v->type == JS_TOBJECT && v->u.object->type == JS_CITERATOR;
}

int js_isuserdata(js_State *J, const char *tag, int idx)
{
	const js_Value *v = stackidx(J, idx);
	if (v->type == JS_TOBJECT && v->u.object->type == JS_CUSERDATA)
		return !strcmp(tag, v->u.object->u.user.tag);
	return 0;
}

static const char *js_typeof(js_State *J, int idx)
{
	const js_Value *v = stackidx(J, idx);
	switch (v->type) {
	case JS_TUNDEFINED: return "undefined";
	case JS_TNULL: return "object";
	case JS_TBOOLEAN: return "boolean";
	case JS_TNUMBER: return "number";
	case JS_TSTRING: return "string";
	case JS_TOBJECT:
		if (v->u.object->type == JS_CFUNCTION || v->u.object->type == JS_CCFUNCTION)
			return "function";
		return "object";
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
	return jsV_numbertointeger(jsV_tonumber(J, stackidx(J, idx)));
}

int js_toint32(js_State *J, int idx)
{
	return jsV_numbertoint32(jsV_tonumber(J, stackidx(J, idx)));
}

unsigned int js_touint32(js_State *J, int idx)
{
	return jsV_numbertouint32(jsV_tonumber(J, stackidx(J, idx)));
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

void *js_toregexp(js_State *J, int idx, int *flags)
{
	const js_Value *v = stackidx(J, idx);
	if (v->type == JS_TOBJECT && v->u.object->type == JS_CREGEXP)
		return *flags = v->u.object->u.r.flags, v->u.object->u.r.prog;
	js_typeerror(J, "not a regexp");
}

void *js_touserdata(js_State *J, const char *tag, int idx)
{
	const js_Value *v = stackidx(J, idx);
	if (v->type == JS_TOBJECT && v->u.object->type == JS_CUSERDATA)
		if (!strcmp(tag, v->u.object->u.user.tag))
			return v->u.object->u.user.data;
	js_typeerror(J, "not a %s", tag);
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

void js_rot2pop1(js_State *J)
{
	/* A B -> B */
	STACK[TOP-2] = STACK[TOP-1];
	--TOP;
}

void js_rot3pop2(js_State *J)
{
	/* A B C -> C */
	STACK[TOP-3] = STACK[TOP-1];
	TOP -= 2;
}

void js_dup1rot3(js_State *J)
{
	/* A B -> B A B */
	STACK[TOP] = STACK[TOP-1];	/* A B B */
	STACK[TOP-1] = STACK[TOP-2];	/* A A B */
	STACK[TOP-2] = STACK[TOP];	/* B A B */
	++TOP;
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
	for (i = 1; i <= n; ++i)
		STACK[TOP-i] = STACK[TOP-i-1];
	STACK[TOP-i] = tmp;
}

/* Property access that takes care of attributes and getters/setters */

static int jsR_hasproperty(js_State *J, js_Object *obj, const char *name)
{
	js_Property *ref;

	if (obj->type == JS_CARRAY) {
		if (!strcmp(name, "length")) {
			js_pushnumber(J, obj->u.a.length);
			return 1;
		}
	}

	ref = jsV_getproperty(J, obj, name);
	if (ref) {
		js_pushvalue(J, ref->value);
		return 1;
	}

	return 0;
}

static void jsR_getproperty(js_State *J, js_Object *obj, const char *name)
{
	if (!jsR_hasproperty(J, obj, name))
		js_pushundefined(J);
}

static void jsR_setproperty(js_State *J, js_Object *obj, const char *name, int idx)
{
	js_Property *ref;

	if (obj->type == JS_CARRAY) {
		unsigned int k;
		char buf[32];

		if (!strcmp(name, "length")) {
			double rawlen = js_tonumber(J, idx);
			unsigned int newlen = jsV_numbertouint32(rawlen);
			if (newlen != rawlen)
				js_rangeerror(J, "array length");
			jsV_resizearray(J, obj, newlen);
			return;
		}

		k = jsV_numbertouint32(jsV_stringtonumber(J, name));
		if (k >= obj->u.a.length) {
			sprintf(buf, "%u", k);
			if (!strcmp(buf, name))
				obj->u.a.length = k + 1;
		}
	}

	ref = jsV_setproperty(J, obj, name);
	if (ref && !(ref->atts & JS_READONLY))
		ref->value = js_tovalue(J, idx);
}

static void jsR_defproperty(js_State *J, js_Object *obj, const char *name, int idx, int atts)
{
	js_Property *ref;

	if (obj->type == JS_CARRAY)
		if (!strcmp(name, "length"))
			return;

	ref = jsV_setproperty(J, obj, name);
	if (ref) {
		ref->value = js_tovalue(J, idx);
		ref->atts = atts;
	}
}

static int jsR_delproperty(js_State *J, js_Object *obj, const char *name)
{
	js_Property *ref = jsV_getownproperty(J, obj, name);
	if (ref) {
		if (ref->atts & JS_DONTDELETE)
			return 0;
		jsV_delproperty(J, obj, name);
	}
	return 1;
}

/* Registry, global and object property accessors */

const char *js_ref(js_State *J)
{
	const js_Value *v = stackidx(J, -1);
	const char *s;
	char buf[32];
	switch (v->type) {
	case JS_TUNDEFINED: s = "_Undefined"; break;
	case JS_TNULL: s = "_Null"; break;
	case JS_TBOOLEAN:
		s = v->u.boolean ? "_True" : "_False";
		break;
	case JS_TOBJECT:
		sprintf(buf, "%p", v->u.object);
		s = js_intern(J, buf);
		break;
	default:
		sprintf(buf, "%d", J->nextref++);
		s = js_intern(J, buf);
		break;
	}
	js_setregistry(J, s);
	return s;
}

void js_unref(js_State *J, const char *ref)
{
	js_delregistry(J, ref);
}

void js_getregistry(js_State *J, const char *name)
{
	jsR_getproperty(J, J->R, name);
}

void js_setregistry(js_State *J, const char *name)
{
	jsR_setproperty(J, J->R, name, -1);
	js_pop(J, 1);
}

void js_delregistry(js_State *J, const char *name)
{
	jsR_delproperty(J, J->R, name);
}

void js_getglobal(js_State *J, const char *name)
{
	jsR_getproperty(J, J->G, name);
}

void js_setglobal(js_State *J, const char *name)
{
	jsR_setproperty(J, J->G, name, -1);
	js_pop(J, 1);
}

void js_defglobal(js_State *J, const char *name, int atts)
{
	jsR_defproperty(J, J->G, name, -1, atts);
	js_pop(J, 1);
}

void js_getproperty(js_State *J, int idx, const char *name)
{
	jsR_getproperty(J, js_toobject(J, idx), name);
}

void js_setproperty(js_State *J, int idx, const char *name)
{
	jsR_setproperty(J, js_toobject(J, idx), name, -1);
	js_pop(J, 1);
}

void js_defproperty(js_State *J, int idx, const char *name, int atts)
{
	jsR_defproperty(J, js_toobject(J, idx), name, -1, atts);
	js_pop(J, 1);
}

void js_delproperty(js_State *J, int idx, const char *name)
{
	jsR_delproperty(J, js_toobject(J, idx), name);
}

int js_hasproperty(js_State *J, int idx, const char *name)
{
	return jsR_hasproperty(J, js_toobject(J, idx), name);
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

static void js_decvar(js_State *J, const char *name, int idx)
{
	jsR_defproperty(J, J->E->variables, name, idx, JS_DONTENUM | JS_DONTDELETE);
}

static void js_getvar(js_State *J, const char *name)
{
	js_Environment *E = J->E;
	do {
		js_Property *ref = jsV_getproperty(J, E->variables, name);
		if (ref) {
			// TODO: use getter
			js_pushvalue(J, ref->value);
			return;
		}
		E = E->outer;
	} while (E);
	js_referenceerror(J, "%s is not defined", name);
}

static void js_setvar(js_State *J, const char *name)
{
	js_Environment *E = J->E;
	do {
		js_Property *ref = jsV_getproperty(J, E->variables, name);
		if (ref) {
			// TODO: use setter
			if (!(ref->atts & JS_READONLY))
				ref->value = js_tovalue(J, -1);
			return;
		}
		E = E->outer;
	} while (E);
	jsR_setproperty(J, J->G, name, -1);
}

static int js_delvar(js_State *J, const char *name)
{
	js_Environment *E = J->E;
	do {
		js_Property *ref = jsV_getownproperty(J, E->variables, name);
		if (ref) {
			if (ref->atts & JS_DONTDELETE)
				return 0;
			jsV_delproperty(J, E->variables, name);
			return 1;
		}
		E = E->outer;
	} while (E);
	return jsR_delproperty(J, J->G, name);
}

/* Function calls */

static void jsR_callfunction(js_State *J, int n, js_Function *F, js_Environment *scope)
{
	js_Environment *saveE;
	js_Value v;
	int i;

	saveE = J->E;

	J->E = jsR_newenvironment(J, jsV_newobject(J, JS_COBJECT, NULL), scope);
	for (i = 0; i < F->numparams; ++i) {
		if (i < n)
			js_decvar(J, F->params[i], i + 1);
		else {
			js_pushundefined(J);
			js_decvar(J, F->params[i], -1);
			js_pop(J, 1);
		}
	}
	js_pop(J, n);

	jsR_run(J, F);
	v = js_tovalue(J, -1);
	TOP = --BOT; /* clear stack */
	js_pushvalue(J, v);

	J->E = saveE;
}

static void jsR_callscript(js_State *J, int n, js_Function *F)
{
	js_Value v;
	js_pop(J, n);
	jsR_run(J, F);
	v = js_tovalue(J, -1);
	TOP = --BOT; /* clear stack */
	js_pushvalue(J, v);
}

static void jsR_callcfunction(js_State *J, int n, js_CFunction F)
{
	int rv = F(J, n);
	if (rv) {
		js_Value v = js_tovalue(J, -1);
		TOP = --BOT; /* clear stack */
		js_pushvalue(J, v);
	} else {
		TOP = --BOT; /* clear stack */
		js_pushundefined(J);
	}
}

void js_call(js_State *J, int n)
{
	js_Object *obj;
	int savebot;

	if (!js_iscallable(J, -n - 2))
		js_typeerror(J, "called object is not a function");

	obj = js_toobject(J, -n - 2);

	savebot = BOT;
	BOT = TOP - n - 1;

	if (obj->type == JS_CFUNCTION)
		jsR_callfunction(J, n, obj->u.f.function, obj->u.f.scope);
	else if (obj->type == JS_CSCRIPT)
		jsR_callscript(J, n, obj->u.f.function);
	else if (obj->type == JS_CCFUNCTION)
		jsR_callcfunction(J, n, obj->u.c.function);

	BOT = savebot;
}

void js_construct(js_State *J, int n)
{
	js_Object *obj;
	js_Object *prototype;
	js_Object *newobj;

	if (!js_iscallable(J, -n - 1))
		js_typeerror(J, "called object is not a function");

	obj = js_toobject(J, -n - 1);

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
		js_error(J, "try: exception stack overflow");
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
	js_OpCode opcode;
	int offset;

	const char *str;
	js_Object *obj;
	double x, y;
	unsigned int ux, uy;
	int ix, iy;
	int b;

	while (1) {
		if (J->gccounter > JS_GCLIMIT) {
			J->gccounter = 0;
			js_gc(J, 0);
		}

		opcode = *pc++;
		switch (opcode) {
		case OP_POP: js_pop(J, 1); break;
		case OP_POP2: js_pop(J, 2); break;
		case OP_DUP: js_dup(J); break;
		case OP_DUP2: js_dup2(J); break;
		case OP_ROT2: js_rot2(J); break;
		case OP_ROT3: js_rot3(J); break;
		case OP_ROT2POP1: js_rot2pop1(J); break;
		case OP_ROT3POP2: js_rot3pop2(J); break;
		case OP_DUP1ROT3: js_dup1rot3(J); break;
		case OP_DUP1ROT4: js_dup1rot4(J); break;

		case OP_NUMBER_0: js_pushnumber(J, 0); break;
		case OP_NUMBER_1: js_pushnumber(J, 1); break;
		case OP_NUMBER_N: js_pushnumber(J, *pc++); break;
		case OP_NUMBER: js_pushnumber(J, NT[*pc++]); break;
		case OP_STRING: js_pushliteral(J, ST[*pc++]); break;

		case OP_CLOSURE: js_newfunction(J, FT[*pc++], J->E); break;
		case OP_NEWOBJECT: js_newobject(J); break;
		case OP_NEWARRAY: js_newarray(J); break;
		case OP_NEWREGEXP: js_newregexp(J, ST[pc[0]], pc[1]); pc += 2; break;

		case OP_UNDEF: js_pushundefined(J); break;
		case OP_NULL: js_pushnull(J); break;
		case OP_TRUE: js_pushboolean(J, 1); break;
		case OP_FALSE: js_pushboolean(J, 0); break;

		case OP_THIS: js_copy(J, 0); break;
		case OP_GLOBAL: js_pushobject(J, J->G); break;

		case OP_FUNDEC:
			js_decvar(J, ST[*pc++], -1);
			js_pop(J, 1);
			break;

		case OP_VARDEC:
			js_pushundefined(J);
			js_decvar(J, ST[*pc++], -1);
			js_pop(J, 1);
			break;

		case OP_GETVAR:
			js_getvar(J, ST[*pc++]);
			break;

		case OP_SETVAR:
			js_setvar(J, ST[*pc++]);
			break;

		case OP_DELVAR:
			b = js_delvar(J, ST[*pc++]);
			js_pushboolean(J, b);
			break;

		case OP_IN:
			str = js_tostring(J, -2);
			b = js_hasproperty(J, -1, str);
			js_pop(J, 2 + b);
			js_pushboolean(J, b);
			break;

		case OP_INITPROP:
			str = js_tostring(J, -2);
			obj = js_toobject(J, -3);
			jsR_setproperty(J, obj, str, -1);
			js_pop(J, 2);
			break;

		case OP_INITPROP_S:
			str = ST[*pc++];
			obj = js_toobject(J, -2);
			jsR_setproperty(J, obj, str, -1);
			js_pop(J, 1);
			break;

		case OP_INITPROP_N:
			str = jsV_numbertostring(J, *pc++);
			obj = js_toobject(J, -2);
			jsR_setproperty(J, obj, str, -1);
			js_pop(J, 1);
			break;

		case OP_GETPROP:
			str = js_tostring(J, -1);
			obj = js_toobject(J, -2);
			jsR_getproperty(J, obj, str);
			js_rot3pop2(J);
			break;

		case OP_GETPROP_S:
			str = ST[*pc++];
			obj = js_toobject(J, -1);
			jsR_getproperty(J, obj, str);
			js_rot2pop1(J);
			break;

		case OP_SETPROP:
			str = js_tostring(J, -2);
			obj = js_toobject(J, -3);
			jsR_setproperty(J, obj, str, -1);
			js_rot3pop2(J);
			break;

		case OP_SETPROP_S:
			str = ST[*pc++];
			obj = js_toobject(J, -2);
			jsR_setproperty(J, obj, str, -1);
			js_rot2pop1(J);
			break;

		case OP_DELPROP:
			str = js_tostring(J, -1);
			obj = js_toobject(J, -2);
			b = jsR_delproperty(J, obj, str);
			js_pop(J, 2);
			js_pushboolean(J, b);
			break;

		case OP_DELPROP_S:
			str = ST[*pc++];
			obj = js_toobject(J, -1);
			b = jsR_delproperty(J, obj, str);
			js_pop(J, 1);
			js_pushboolean(J, b);
			break;

		case OP_ITERATOR:
			if (!js_isundefined(J, -1) && !js_isnull(J, -1)) {
				obj = jsV_newiterator(J, js_toobject(J, -1), 0);
				js_pop(J, 1);
				js_pushobject(J, obj);
			}
			break;

		case OP_NEXTITER:
			if (js_isiterator(J, -1)) {
				obj = js_toobject(J, -1);
				str = jsV_nextiterator(J, obj);
				if (str) {
					js_pushliteral(J, str);
					js_pushboolean(J, 1);
				} else {
					js_pop(J, 1);
					js_pushboolean(J, 0);
				}
			} else {
				js_pop(J, 1);
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
			ix = js_toint32(J, -1);
			js_pop(J, 1);
			js_pushnumber(J, ~ix);
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
			ix = js_toint32(J, -2);
			uy = js_touint32(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, ix << (uy & 0x1F));
			break;

		case OP_SHR:
			ix = js_toint32(J, -2);
			uy = js_touint32(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, ix >> (uy & 0x1F)); break;
			break;

		case OP_USHR:
			ux = js_touint32(J, -2);
			uy = js_touint32(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, ux >> (uy & 0x1F)); break;
			break;

		/* Relational operators */

		case OP_LT: b = js_compare(J); js_pop(J, 2); js_pushboolean(J, b < 0); break;
		case OP_GT: b = js_compare(J); js_pop(J, 2); js_pushboolean(J, b > 0); break;
		case OP_LE: b = js_compare(J); js_pop(J, 2); js_pushboolean(J, b <= 0); break;
		case OP_GE: b = js_compare(J); js_pop(J, 2); js_pushboolean(J, b >= 0); break;

		case OP_INSTANCEOF:
			b = js_instanceof(J);
			js_pop(J, 2);
			js_pushboolean(J, b);
			break;

		/* Equality */

		case OP_EQ: b = js_equal(J); js_pop(J, 2); js_pushboolean(J, b); break;
		case OP_NE: b = js_equal(J); js_pop(J, 2); js_pushboolean(J, !b); break;
		case OP_STRICTEQ: b = js_strictequal(J); js_pop(J, 2); js_pushboolean(J, b); break;
		case OP_STRICTNE: b = js_strictequal(J); js_pop(J, 2); js_pushboolean(J, !b); break;

		case OP_JCASE:
			offset = *pc++;
			b = js_strictequal(J);
			if (b) {
				js_pop(J, 2);
				pc = pcstart + offset;
			} else {
				js_pop(J, 1);
			}
			break;

		/* Binary bitwise operators */

		case OP_BITAND:
			ix = js_toint32(J, -2);
			iy = js_toint32(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, ix & iy);
			break;

		case OP_BITXOR:
			ix = js_toint32(J, -2);
			iy = js_toint32(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, ix ^ iy);
			break;

		case OP_BITOR:
			ix = js_toint32(J, -2);
			iy = js_toint32(J, -1);
			js_pop(J, 2);
			js_pushnumber(J, ix | iy);
			break;

		/* Try and Catch */

		case OP_THROW:
			js_throw(J);
			break;

		case OP_TRY:
			offset = *pc++;
			if (js_trypc(J, pc)) {
				pc = J->trybuf[J->trylen].pc;
			} else {
				pc = pcstart + offset;
			}
			break;

		case OP_ENDTRY:
			js_endtry(J);
			break;

		case OP_CATCH:
			str = ST[*pc++];
			obj = jsV_newobject(J, JS_COBJECT, NULL);
			js_pushobject(J, obj);
			js_rot2(J);
			js_setproperty(J, -2, str);
			J->E = jsR_newenvironment(J, obj, J->E);
			js_pop(J, 1);
			break;

		case OP_ENDCATCH:
			J->E = J->E->outer;
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
