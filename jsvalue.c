#include "js.h"
#include "jsobject.h"
#include "jsrun.h"

const char *jsR_stringfromnumber(js_State *J, double n)
{
	char buf[32];
	if (isnan(n)) return "NaN";
	if (isinf(n)) return n < 0 ? "-Infinity" : "Infinity";
	if (n == 0) return "0";
	sprintf(buf, "%.17g", n); /* DBL_DECIMAL_DIG == 17 */
	return js_intern(J, buf);
}

double jsR_numberfromstring(js_State *J, const char *s)
{
	/* TODO: use lexer to parse string grammar */
	return strtod(s, NULL);
}

static int jsR_toString(js_State *J, js_Object *obj)
{
	js_pushobject(J, obj);
	js_getproperty(J, -1, "toString");
	if (js_iscallable(J, -1)) {
		js_rot2(J);
		js_call(J, 0);
		if (js_isprimitive(J, -1))
			return 1;
		js_pop(J, 1);
		return 0;
	}
	js_pop(J, 2);
	return 0;
}

static int jsR_valueOf(js_State *J, js_Object *obj)
{
	js_pushobject(J, obj);
	js_getproperty(J, -1, "valueOf");
	if (js_iscallable(J, -1)) {
		js_rot2(J);
		js_call(J, 0);
		if (js_isprimitive(J, -1))
			return 1;
		js_pop(J, 1);
		return 0;
	}
	js_pop(J, 2);
	return 0;
}

js_Value jsR_toprimitive(js_State *J, const js_Value *v, int preferred)
{
	js_Value vv;
	js_Object *obj;

	if (v->type != JS_TOBJECT)
		return *v;

	obj = v->u.object;

	if (preferred == JS_HNONE)
		preferred = obj->type == JS_CDATE ? JS_HSTRING : JS_HNUMBER;

	if (preferred == JS_HSTRING) {
		if (jsR_toString(J, obj) || jsR_valueOf(J, obj)) {
			vv = js_tovalue(J, -1);
			js_pop(J, 1);
			return vv;
		}
	} else {
		if (jsR_valueOf(J, obj) || jsR_toString(J, obj)) {
			vv = js_tovalue(J, -1);
			js_pop(J, 1);
			return vv;
		}
	}
	jsR_throwTypeError(J, "cannot convert object to primitive");
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
	case JS_TSTRING: return jsR_numberfromstring(J, v->u.string);
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
	case JS_TNUMBER: return jsR_stringfromnumber(J, v->u.number);
	case JS_TSTRING: return v->u.string;
	case JS_TOBJECT:
		{
			js_Value vv = jsR_toprimitive(J, v, JS_HSTRING);
			return jsR_tostring(J, &vv);
		}
	}
	return "undefined";
}

js_Object *jsR_toobject(js_State *J, const js_Value *v)
{
	switch (v->type) {
	case JS_TUNDEFINED: jsR_throwTypeError(J, "cannot convert undefined to object");
	case JS_TNULL: jsR_throwTypeError(J, "cannot convert null to object");
	case JS_TBOOLEAN: return jsR_newboolean(J, v->u.boolean);
	case JS_TNUMBER: return jsR_newnumber(J, v->u.number);
	case JS_TSTRING: return jsR_newstring(J, v->u.string);
	case JS_TOBJECT: return v->u.object;
	}
	jsR_throwTypeError(J, "cannot convert value to object");
}

void jsR_concat(js_State *J)
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
		double x = jsR_tonumber(J, &va);
		double y = jsR_tonumber(J, &vb);
		js_pop(J, 2);
		js_pushnumber(J, x + y);
	}
}

int jsR_compare(js_State *J)
{
	js_Value va = js_toprimitive(J, -2, JS_HNUMBER);
	js_Value vb = js_toprimitive(J, -1, JS_HNUMBER);
	js_pop(J, 2);
	if (va.type == JS_TSTRING && vb.type == JS_TSTRING) {
		return strcmp(va.u.string, va.u.string);
	} else {
		double x = jsR_tonumber(J, &va);
		double y = jsR_tonumber(J, &vb);
		return x < y ? -1 : x > y ? 1 : 0;
	}
}

int jsR_equal(js_State *J)
{
	js_Value va = js_tovalue(J, -2);
	js_Value vb = js_tovalue(J, -1);
	js_pop(J, 2);

retry:
	if (va.type == vb.type) {
		if (va.type == JS_TUNDEFINED) return 1;
		if (va.type == JS_TNULL) return 1;
		if (va.type == JS_TNUMBER) return va.u.number == vb.u.number;
		if (va.type == JS_TBOOLEAN) return va.u.boolean == vb.u.boolean;
		if (va.type == JS_TSTRING) return !strcmp(va.u.string, vb.u.string);
		if (va.type == JS_TOBJECT) return va.u.object == vb.u.object;
		return 0;
	}

	if (va.type == JS_TNULL && vb.type == JS_TUNDEFINED) return 1;
	if (va.type == JS_TUNDEFINED && vb.type == JS_TNULL) return 1;

	if (va.type == JS_TNUMBER && (vb.type == JS_TSTRING || vb.type == JS_TBOOLEAN))
		return va.u.number == jsR_tonumber(J, &vb);
	if ((va.type == JS_TSTRING || va.type == JS_TBOOLEAN) && vb.type == JS_TNUMBER)
		return jsR_tonumber(J, &va) == vb.u.number;

	if ((va.type == JS_TSTRING || va.type == JS_TNUMBER) && vb.type == JS_TOBJECT) {
		vb = jsR_toprimitive(J, &vb, JS_HNONE);
		goto retry;
	}
	if (va.type == JS_TOBJECT && (vb.type == JS_TSTRING || vb.type == JS_TNUMBER)) {
		va = jsR_toprimitive(J, &va, JS_HNONE);
		goto retry;
	}

	return 0;
}

int jsR_strictequal(js_State *J)
{
	js_Value va = js_tovalue(J, -2);
	js_Value vb = js_tovalue(J, -1);
	js_pop(J, 2);

	if (va.type != vb.type) return 0;
	if (va.type == JS_TUNDEFINED) return 1;
	if (va.type == JS_TNULL) return 1;
	if (va.type == JS_TNUMBER) return va.u.number == vb.u.number;
	if (va.type == JS_TBOOLEAN) return va.u.boolean == vb.u.boolean;
	if (va.type == JS_TSTRING) return !strcmp(va.u.string, vb.u.string);
	if (va.type == JS_TOBJECT) return va.u.object == vb.u.object;
	return 0;
}
