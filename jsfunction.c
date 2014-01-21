#include "jsi.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static int jsB_Function(js_State *J, int argc)
{
	return 0;
}

static int jsB_Function_prototype(js_State *J, int argc)
{
	js_pushundefined(J);
	return 1;
}

static int Fp_toString(js_State *J, int argc)
{
	js_Object *self = js_toobject(J, 0);
	char *s;
	int i, n;

	if (!js_iscallable(J, 0))
		js_typeerror(J, "not a function");

	if (self->type == JS_CFUNCTION || self->type == JS_CSCRIPT) {
		js_Function *F = self->u.f.function;
		n = strlen("function () { ... }");
		n += strlen(F->name);
		for (i = 0; i < F->numparams; i++)
			n += strlen(F->params[i]) + 1;
		s = malloc(n);
		strcpy(s, "function ");
		strcat(s, F->name);
		strcat(s, "(");
		for (i = 0; i < F->numparams; i++) {
			if (i > 0) strcat(s, ",");
			strcat(s, F->params[i]);
		}
		strcat(s, ") { ... }");
		js_pushstring(J, s);
		free(s);
	} else {
		js_pushliteral(J, "function () { ... }");
	}

	return 1;
}

static int Fp_apply(js_State *J, int argc)
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
	return 1;
}

static int Fp_call(js_State *J, int argc)
{
	int i;

	if (!js_iscallable(J, 0))
		js_typeerror(J, "not a function");

	js_copy(J, 0);
	js_copy(J, 1);
	for (i = 2; i <= argc; ++i)
		js_copy(J, i);

	js_call(J, argc - 1);
	return 1;
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
	}
	js_newcconstructor(J, jsB_Function, jsB_Function);
	js_setglobal(J, "Function");
}