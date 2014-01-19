#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_new_Function(js_State *J, int n) { return 0; }
static int jsB_Function(js_State *J, int n) { return 0; }

static int jsB_Function_prototype(js_State *J, int n)
{
	js_pushundefined(J);
	return 1;
}

static int Fp_apply(js_State *J, int n)
{
	int i, argc;
	char name[20];

	if (!js_iscallable(J, 0))
		jsR_error(J, "TypeError");
	js_copy(J, 0);

	if (js_isundefined(J, 1) || js_isnull(J, 1))
		js_pushglobal(J);
	else
		js_copy(J, 1);

	js_getproperty(J, 2, "length");
	argc = js_tonumber(J, -1);
	js_pop(J, 1);

	for (i = 0; i < argc; ++i) {
		sprintf(name, "%d", i);
		js_getproperty(J, 2, name);
	}

	js_call(J, argc);
	return 1;
}

static int Fp_call(js_State *J, int n)
{
	int i;

	if (!js_iscallable(J, 0))
		jsR_error(J, "TypeError");
	js_copy(J, 0);

	if (js_isundefined(J, 1) || js_isnull(J, 1))
		js_pushglobal(J);
	else
		js_copy(J, 1);

	for (i = 1; i < n; ++i)
		js_copy(J, i + 1);

	js_call(J, n - 1);
	return 1;
}

void jsB_initfunction(js_State *J)
{
	J->Function_prototype->cfunction = jsB_Function_prototype;
	J->Function_prototype->cconstructor = NULL;

	js_pushobject(J, jsR_newcconstructor(J, jsB_Function, jsB_new_Function));
	{
		jsB_propn(J, "length", 1);

		js_pushobject(J, J->Function_prototype);
		{
			js_copy(J, -2);
			js_setproperty(J, -2, "constructor");
			// jsB_propf(J, "toString", Fp_toString, 2);
			// jsB_propf(J, "apply", Fp_apply, 2);
			jsB_propf(J, "call", Fp_call, 1);
		}
		js_setproperty(J, -2, "prototype");
	}
	js_setglobal(J, "Function");
}
