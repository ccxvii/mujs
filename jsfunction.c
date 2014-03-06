#include "jsi.h"
#include "jsparse.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static void jsB_Function(js_State *J, unsigned int argc)
{
	const char *source;
	js_Buffer *sb;
	js_Ast *parse;
	js_Function *fun;
	unsigned int i;

	if (js_isundefined(J, 1))
		source = "";
	else {
		source = js_tostring(J, argc);
		sb = NULL;
		if (argc > 1) {
			for (i = 1; i < argc; ++i) {
				if (i > 1) js_putc(J, &sb, ',');
				js_puts(J, &sb, js_tostring(J, i));
			}
			js_putc(J, &sb, ')');
		}
	}

	if (js_try(J)) {
		js_free(J, sb);
		jsP_freeparse(J);
		js_throw(J);
	}

	parse = jsP_parsefunction(J, "Function", sb->s, source);
	fun = jsC_compilefunction(J, parse);

	js_endtry(J);
	js_free(J, sb);
	jsP_freeparse(J);

	js_newfunction(J, fun, J->GE);
}

static void jsB_Function_prototype(js_State *J, unsigned int argc)
{
	js_pushundefined(J);
}

static void Fp_toString(js_State *J, unsigned int argc)
{
	js_Object *self = js_toobject(J, 0);
	char *s;
	unsigned int i, n;

	if (!js_iscallable(J, 0))
		js_typeerror(J, "not a function");

	if (self->type == JS_CFUNCTION || self->type == JS_CSCRIPT) {
		js_Function *F = self->u.f.function;
		n = strlen("function () { ... }");
		n += strlen(F->name);
		for (i = 0; i < F->numparams; ++i)
			n += strlen(F->vartab[i]) + 1;
		s = js_malloc(J, n);
		strcpy(s, "function ");
		strcat(s, F->name);
		strcat(s, "(");
		for (i = 0; i < F->numparams; ++i) {
			if (i > 0) strcat(s, ",");
			strcat(s, F->vartab[i]);
		}
		strcat(s, ") { ... }");
		if (js_try(J)) {
			js_free(J, s);
			js_throw(J);
		}
		js_pushstring(J, s);
		js_free(J, s);
	} else {
		js_pushliteral(J, "function () { ... }");
	}
}

static void Fp_apply(js_State *J, unsigned int argc)
{
	int i, n;
	char name[20];

	if (!js_iscallable(J, 0))
		js_typeerror(J, "not a function");

	js_copy(J, 0);
	js_copy(J, 1);

	js_getproperty(J, 2, "length");
	n = js_tonumber(J, -1);
	js_pop(J, 1);

	for (i = 0; i < n; ++i) {
		sprintf(name, "%d", i);
		js_getproperty(J, 2, name);
	}

	js_call(J, n);
}

static void Fp_call(js_State *J, unsigned int argc)
{
	unsigned int i;

	if (!js_iscallable(J, 0))
		js_typeerror(J, "not a function");

	js_copy(J, 0);
	js_copy(J, 1);
	for (i = 2; i <= argc; ++i)
		js_copy(J, i);

	js_call(J, argc - 1);
}

static void callbound(js_State *J, unsigned int argc)
{
	unsigned int i, fun, args, n;

	fun = js_gettop(J);
	js_currentfunction(J);
	js_getproperty(J, fun, "__TargetFunction__");
	js_getproperty(J, fun, "__BoundThis__");

	args = js_gettop(J);
	js_getproperty(J, fun, "__BoundArguments__");
	n = js_getlength(J, args);
	for (i = 0; i < n; ++i)
		js_getindex(J, args, i);
	js_remove(J, args);

	for (i = 1; i <= argc; ++i)
		js_copy(J, i);

	js_call(J, n + argc);
}

static void constructbound(js_State *J, unsigned int argc)
{
	unsigned int i, fun, args, n;

	fun = js_gettop(J);
	js_currentfunction(J);
	js_getproperty(J, fun, "__TargetFunction__");

	args = js_gettop(J);
	js_getproperty(J, fun, "__BoundArguments__");
	n = js_getlength(J, args);
	for (i = 0; i < n; ++i)
		js_getindex(J, args, i);
	js_remove(J, args);

	for (i = 1; i <= argc; ++i)
		js_copy(J, i);

	js_construct(J, n + argc);
}

static void Fp_bind(js_State *J, unsigned int argc)
{
	unsigned int i, n;

	if (!js_iscallable(J, 0))
		js_typeerror(J, "not a function");

	n = js_getlength(J, 0);
	if (argc - 1 < n)
		n -= argc - 1;
	else
		n = 0;

	js_newcconstructor(J, callbound, constructbound, n);

	/* Reuse target function's prototype for HasInstance check. */
	js_getproperty(J, 0, "prototype");
	js_defproperty(J, -2, "prototype", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	/* target function */
	js_copy(J, 0);
	js_defproperty(J, -2, "__TargetFunction__", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	/* bound this */
	js_copy(J, 1);
	js_defproperty(J, -2, "__BoundThis__", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	/* bound arguments */
	js_newarray(J);
	for (i = 2; i <= argc; ++i) {
		js_copy(J, i);
		js_setindex(J, -2, i-2);
	}
	js_defproperty(J, -2, "__BoundArguments__", JS_READONLY | JS_DONTENUM | JS_DONTCONF);
}

void jsB_initfunction(js_State *J)
{
	J->Function_prototype->u.c.function = jsB_Function_prototype;
	J->Function_prototype->u.c.constructor = NULL;

	js_pushobject(J, J->Function_prototype);
	{
		jsB_propf(J, "toString", Fp_toString, 2);
		jsB_propf(J, "apply", Fp_apply, 2);
		jsB_propf(J, "call", Fp_call, 1);
		jsB_propf(J, "bind", Fp_bind, 1);
	}
	js_newcconstructor(J, jsB_Function, jsB_Function, 1);
	js_defglobal(J, "Function", JS_DONTENUM);
}
