#include "jsi.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static void jsB_globalf(js_State *J, const char *name, js_CFunction cfun, int n)
{
	js_newcfunction(J, cfun, n);
	js_defglobal(J, name, JS_DONTENUM);
}

void jsB_propf(js_State *J, const char *name, js_CFunction cfun, int n)
{
	js_newcfunction(J, cfun, n);
	js_defproperty(J, -2, name, JS_DONTENUM);
}

void jsB_propn(js_State *J, const char *name, double number)
{
	js_pushnumber(J, number);
	js_defproperty(J, -2, name, JS_DONTENUM);
}

void jsB_props(js_State *J, const char *name, const char *string)
{
	js_pushliteral(J, string);
	js_defproperty(J, -2, name, JS_DONTENUM);
}

static int jsB_eval(js_State *J, int argc)
{
	if (!js_isstring(J, -1))
		return 1;
	js_loadstring(J, "(eval)", js_tostring(J, -1));
	js_copy(J, 0);
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
	js_pushnumber(J, strtod(s, NULL));
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

void jsB_init(js_State *J)
{
	/* Create the prototype objects here, before the constructors */
	J->Object_prototype = jsV_newobject(J, JS_COBJECT, NULL);
	J->Array_prototype = jsV_newobject(J, JS_CARRAY, J->Object_prototype);
	J->Function_prototype = jsV_newobject(J, JS_CCFUNCTION, J->Object_prototype);
	J->Boolean_prototype = jsV_newobject(J, JS_CBOOLEAN, J->Object_prototype);
	J->Number_prototype = jsV_newobject(J, JS_CNUMBER, J->Object_prototype);
	J->String_prototype = jsV_newobject(J, JS_CSTRING, J->Object_prototype);
	J->Date_prototype = jsV_newobject(J, JS_CDATE, J->Object_prototype);

	/* All the native error types */
	J->Error_prototype = jsV_newobject(J, JS_CERROR, J->Object_prototype);
	J->EvalError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->RangeError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->ReferenceError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->SyntaxError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->TypeError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);
	J->URIError_prototype = jsV_newobject(J, JS_CERROR, J->Error_prototype);

	/* Create the constructors and fill out the prototype objects */
	jsB_initobject(J);
	jsB_initarray(J);
	jsB_initfunction(J);
	jsB_initboolean(J);
	jsB_initnumber(J);
	jsB_initstring(J);
	jsB_initerror(J);
	jsB_initmath(J);
	jsB_initdate(J);

	/* Initialize the global object */
	js_pushnumber(J, NAN);
	js_defglobal(J, "NaN", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);

	js_pushnumber(J, INFINITY);
	js_defglobal(J, "Infinity", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);

	js_pushundefined(J);
	js_defglobal(J, "undefined", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);

	jsB_globalf(J, "eval", jsB_eval, 1);
	jsB_globalf(J, "parseInt", jsB_parseInt, 1);
	jsB_globalf(J, "parseFloat", jsB_parseFloat, 1);
	jsB_globalf(J, "isNaN", jsB_isNaN, 1);
	jsB_globalf(J, "isFinite", jsB_isFinite, 1);
}
