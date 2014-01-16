#include "js.h"
#include "jsobject.h"

enum {
	JS_HNONE,
	JS_HNUMBER,
	JS_HSTRING,
};

static js_Value jsR_toprimitive(js_State *J, const js_Value *v, int preferred)
{
	js_Object *obj = v->u.object;

	if (preferred == JS_HNONE)
		preferred = obj->type == JS_CDATE ? JS_HSTRING : JS_HNUMBER;

	if (preferred == JS_HSTRING) {
		// try "toString"
		// if result is primitive, return result
		// try "valueOf"
		// if result is primitive, return result
	} else {
		// try "toString"
		// if result is primitive, return result
		// try "valueOf"
		// if result is primitive, return result
	}
	jsR_error(J, "TypeError (ToPrimitive)");
}

int jsR_toboolean(js_State *J, const js_Value *v)
{
	switch (v->type) {
	case JS_TUNDEFINED: return 0;
	case JS_TNULL: return 0;
	case JS_TBOOLEAN: return v->u.boolean;
	case JS_TNUMBER: return v->u.number != 0 && !isnan(v->u.number);
	case JS_TSTRING: return v->u.string[0] != 0;
	case JS_TOBJECT: return 0;
	}
	return 0;
}

double jsR_tonumber(js_State *J, const js_Value *v)
{
	switch (v->type) {
	case JS_TUNDEFINED: return NAN;
	case JS_TNULL: return 0;
	case JS_TBOOLEAN: return v->u.boolean;
	case JS_TNUMBER: return v->u.number;
	case JS_TSTRING:
		{
			/* TODO: use lexer to parse string grammar */
			return strtod(v->u.string, NULL);
		}
	case JS_TOBJECT:
		{
			js_Value vv = jsR_toprimitive(J, v, JS_HNUMBER);
			return jsR_tonumber(J, &vv);
		}
	}
	return 0;
}

const char *jsR_tostring(js_State *J, const js_Value *v)
{
	switch (v->type) {
	case JS_TUNDEFINED: return "undefined";
	case JS_TNULL: return "null";
	case JS_TBOOLEAN: return v->u.boolean ? "true" : "false";
	case JS_TNUMBER:
		{
			char buf[32];
			double n = v->u.number;
			if (isnan(n)) return "NaN";
			if (isinf(n)) return n < 0 ? "-Infinity" : "Infinity";
			if (n == 0) return "0";
			sprintf(buf, "%.17g", n); /* DBL_DECIMAL_DIG == 17 */
			return js_intern(J, buf);
		}
	case JS_TSTRING: return v->u.string;
	case JS_TOBJECT:
		{
			js_Value vv = jsR_toprimitive(J, v, JS_HSTRING);
			return jsR_tostring(J, &vv);
		}
	}
	return NULL;
}

js_Object *jsR_toobject(js_State *J, const js_Value *v)
{
	switch (v->type) {
	case JS_TUNDEFINED: jsR_error(J, "TypeError (ToObject(undefined))");
	case JS_TNULL: jsR_error(J, "TypeError (ToObject(null))");
	case JS_TBOOLEAN: return jsR_newboolean(J, v->u.boolean);
	case JS_TNUMBER: return jsR_newnumber(J, v->u.number);
	case JS_TSTRING: return jsR_newstring(J, v->u.string);
	case JS_TOBJECT: return v->u.object;
	}
	return NULL;
}
