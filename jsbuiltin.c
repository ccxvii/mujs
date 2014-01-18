#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_print(js_State *J, int argc)
{
	int i;
	for (i = 1; i < argc; ++i) {
		const char *s = js_tostring(J, i);
		if (i > 1) putchar(' ');
		fputs(s, stdout);
	}
	putchar('\n');
	return 0;
}

static int jsB_collectGarbage(js_State *J, int argc)
{
	int report = js_toboolean(J, 1);
	js_gc(J, report);
	return 0;
}

static int jsB_eval(js_State *J, int argc)
{
	const char *s;

	if (!js_isstring(J, -1))
		return 1;

	s = js_tostring(J, -1);
	if (jsR_loadscript(J, "(eval)", s))
		jsR_error(J, "SyntaxError (eval)");

	js_copy(J, 0); /* copy this */
	js_call(J, 0);
	return 1;
}

static int jsB_parseInt(js_State *J, int argc)
{
	const char *s = js_tostring(J, 1);
	double radix = argc > 1 ? js_tonumber(J, 2) : 10;
	js_pushnumber(J, strtol(s, NULL, radix == 0 ? 10 : radix));
	return 1;
}

static int jsB_parseFloat(js_State *J, int argc)
{
	const char *s = js_tostring(J, 1);
	js_pushnumber(J, jsR_stringtonumber(J, s));
	return 1;
}

static int jsB_isNaN(js_State *J, int argc)
{
	double n = js_tonumber(J, 1);
	js_pushboolean(J, isnan(n));
	return 1;
}

static int jsB_isFinite(js_State *J, int argc)
{
	double n = js_tonumber(J, 1);
	js_pushboolean(J, isfinite(n));
	return 1;
}

static void jsB_register(js_State *J, const char *name, js_CFunction cfun)
{
	js_newcfunction(J, cfun);
	js_setglobal(J, name);
}

void jsB_init(js_State *J)
{
	jsB_initobject(J);
	jsB_initarray(J);
	jsB_initfunction(J);
	jsB_initboolean(J);
	jsB_initnumber(J);
	jsB_initstring(J);

	js_pushnumber(J, NAN);
	js_setglobal(J, "NaN");

	js_pushnumber(J, INFINITY);
	js_setglobal(J, "Infinity");

	js_pushundefined(J);
	js_setglobal(J, "undefined");

	jsB_register(J, "eval", jsB_eval);
	jsB_register(J, "parseInt", jsB_parseInt);
	jsB_register(J, "parseFloat", jsB_parseFloat);
	jsB_register(J, "isNaN", jsB_isNaN);
	jsB_register(J, "isFinite", jsB_isFinite);

	jsB_register(J, "collectGarbage", jsB_collectGarbage);
	jsB_register(J, "print", jsB_print);
}
