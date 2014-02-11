#include "jsi.h"
#include "jslex.h"
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
	if (!js_isstring(J, -1)) {
		js_copy(J, 1);
		return 1;
	}
	js_loadstring(J, "(eval)", js_tostring(J, -1));
	js_pushglobal(J);
	js_call(J, 0);
	return 1;
}

static int jsB_parseInt(js_State *J, int argc)
{
	const char *s = js_tostring(J, 1);
	double radix = js_isdefined(J, 2) ? js_tonumber(J, 2) : 10;
	while (jsY_iswhite(*s) || jsY_isnewline(*s)) ++s;
	js_pushnumber(J, strtol(s, NULL, radix == 0 ? 10 : radix));
	return 1;
}

static int jsB_parseFloat(js_State *J, int argc)
{
	const char *s = js_tostring(J, 1);
	while (jsY_iswhite(*s) || jsY_isnewline(*s)) ++s;
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

static int Encode(js_State *J, const char *str, const char *unescaped)
{
	js_Buffer *sb = NULL;

	static const char *HEX = "0123456789ABCDEF";

	while (*str) {
		int c = (unsigned char) *str++;
		if (strchr(unescaped, c))
			sb_putc(&sb, c);
		else {
			sb_putc(&sb, '%');
			sb_putc(&sb, HEX[(c >> 4) & 0xf]);
			sb_putc(&sb, HEX[c & 0xf]);
		}
	}
	sb_putc(&sb, 0);

	if (js_try(J)) {
		free(sb);
		js_throw(J);
	}
	js_pushstring(J, sb ? sb->s : "");
	js_endtry(J);
	free(sb);
	return 1;
}

static int Decode(js_State *J, const char *str, const char *reserved)
{
	js_Buffer *sb = NULL;
	int a, b;

	while (*str) {
		int c = (unsigned char) *str++;
		if (c != '%')
			sb_putc(&sb, c);
		else {
			if (!str[0] || !str[1])
				js_urierror(J, "truncated escape sequence");
			a = *str++;
			b = *str++;
			if (!jsY_ishex(a) || !jsY_ishex(b))
				js_urierror(J, "invalid escape sequence");
			c = jsY_tohex(a) << 4 | jsY_tohex(b);
			if (!strchr(reserved, c))
				sb_putc(&sb, c);
			else {
				sb_putc(&sb, '%');
				sb_putc(&sb, a);
				sb_putc(&sb, b);
			}
		}
	}
	sb_putc(&sb, 0);

	if (js_try(J)) {
		free(sb);
		js_throw(J);
	}
	js_pushstring(J, sb ? sb->s : "");
	js_endtry(J);
	free(sb);
	return 1;
}

#define URIRESERVED ";/?:@&=+$,"
#define URIALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define URIDIGIT "0123456789"
#define URIMARK "-_.!~*`()"
#define URIUNESCAPED URIALPHA URIDIGIT URIMARK

static int jsB_decodeURI(js_State *J, int argc)
{
	return Decode(J, js_tostring(J, 1), URIRESERVED "#");
}

static int jsB_decodeURIComponent(js_State *J, int argc)
{
	return Decode(J, js_tostring(J, 1), "");
}

static int jsB_encodeURI(js_State *J, int argc)
{
	return Encode(J, js_tostring(J, 1), URIUNESCAPED URIRESERVED "#");
}

static int jsB_encodeURIComponent(js_State *J, int argc)
{
	return Encode(J, js_tostring(J, 1), URIUNESCAPED);
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
	J->RegExp_prototype = jsV_newobject(J, JS_COBJECT, J->Object_prototype);
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
	jsB_initregexp(J);
	jsB_initdate(J);
	jsB_initerror(J);
	jsB_initmath(J);
	jsB_initjson(J);

	/* Initialize the global object */
	js_pushnumber(J, NAN);
	js_defglobal(J, "NaN", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	js_pushnumber(J, INFINITY);
	js_defglobal(J, "Infinity", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	js_pushundefined(J);
	js_defglobal(J, "undefined", JS_READONLY | JS_DONTENUM | JS_DONTCONF);

	jsB_globalf(J, "eval", jsB_eval, 1);
	jsB_globalf(J, "parseInt", jsB_parseInt, 1);
	jsB_globalf(J, "parseFloat", jsB_parseFloat, 1);
	jsB_globalf(J, "isNaN", jsB_isNaN, 1);
	jsB_globalf(J, "isFinite", jsB_isFinite, 1);

	jsB_globalf(J, "decodeURI", jsB_decodeURI, 1);
	jsB_globalf(J, "decodeURIComponent", jsB_decodeURIComponent, 1);
	jsB_globalf(J, "encodeURI", jsB_encodeURI, 1);
	jsB_globalf(J, "encodeURIComponent", jsB_encodeURIComponent, 1);
}
