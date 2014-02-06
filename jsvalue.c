#include "jsi.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "utf.h"

double jsV_numbertointeger(double n)
{
	double sign = n < 0 ? -1 : 1;
	if (isnan(n)) return 0;
	if (n == 0 || isinf(n)) return n;
	return sign * floor(abs(n));
}

int jsV_numbertoint32(double n)
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

unsigned int jsV_numbertouint32(double n)
{
	return jsV_numbertoint32(n);
}

/* obj.toString() */
static int jsV_toString(js_State *J, js_Object *obj)
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

/* obj.valueOf() */
static int jsV_valueOf(js_State *J, js_Object *obj)
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

/* ToPrimitive() on a value */
js_Value jsV_toprimitive(js_State *J, const js_Value *v, int preferred)
{
	js_Value vv;
	js_Object *obj;

	if (v->type != JS_TOBJECT)
		return *v;

	obj = v->u.object;

	if (preferred == JS_HNONE)
		preferred = obj->type == JS_CDATE ? JS_HSTRING : JS_HNUMBER;

	if (preferred == JS_HSTRING) {
		if (jsV_toString(J, obj) || jsV_valueOf(J, obj)) {
			vv = js_tovalue(J, -1);
			js_pop(J, 1);
			return vv;
		}
	} else {
		if (jsV_valueOf(J, obj) || jsV_toString(J, obj)) {
			vv = js_tovalue(J, -1);
			js_pop(J, 1);
			return vv;
		}
	}
	js_typeerror(J, "cannot convert object to primitive");
}

/* ToBoolean() on a value */
int jsV_toboolean(js_State *J, const js_Value *v)
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

/* ToNumber() on a string */
double jsV_stringtonumber(js_State *J, const char *s)
{
	/* TODO: use lexer to parse string grammar */
	return strtod(s, NULL);
}

/* ToNumber() on a value */
double jsV_tonumber(js_State *J, const js_Value *v)
{
	switch (v->type) {
	case JS_TUNDEFINED: return NAN;
	case JS_TNULL: return 0;
	case JS_TBOOLEAN: return v->u.boolean;
	case JS_TNUMBER: return v->u.number;
	case JS_TSTRING: return jsV_stringtonumber(J, v->u.string);
	case JS_TOBJECT:
		{
			js_Value vv = jsV_toprimitive(J, v, JS_HNUMBER);
			return jsV_tonumber(J, &vv);
		}
	}
	return 0;
}

double jsV_tointeger(js_State *J, const js_Value *v)
{
	return jsV_numbertointeger(jsV_tonumber(J, v));
}

/* ToString() on a number */
const char *jsV_numbertostring(js_State *J, double n)
{
	char buf[32];
	if (isnan(n)) return "NaN";
	if (isinf(n)) return n < 0 ? "-Infinity" : "Infinity";
	if (n == 0) return "0";
	sprintf(buf, "%.17g", n); /* DBL_DECIMAL_DIG == 17 */
	return js_intern(J, buf);
}

/* ToString() on a value */
const char *jsV_tostring(js_State *J, const js_Value *v)
{
	switch (v->type) {
	case JS_TUNDEFINED: return "undefined";
	case JS_TNULL: return "null";
	case JS_TBOOLEAN: return v->u.boolean ? "true" : "false";
	case JS_TNUMBER: return jsV_numbertostring(J, v->u.number);
	case JS_TSTRING: return v->u.string;
	case JS_TOBJECT:
		{
			js_Value vv = jsV_toprimitive(J, v, JS_HSTRING);
			return jsV_tostring(J, &vv);
		}
	}
	return "undefined";
}

/* Objects */

static js_Object *jsV_newboolean(js_State *J, int v)
{
	js_Object *obj = jsV_newobject(J, JS_CBOOLEAN, J->Boolean_prototype);
	obj->u.boolean = v;
	return obj;
}

static js_Object *jsV_newnumber(js_State *J, double v)
{
	js_Object *obj = jsV_newobject(J, JS_CNUMBER, J->Number_prototype);
	obj->u.number = v;
	return obj;
}

static js_Object *jsV_newstring(js_State *J, const char *v)
{
	js_Object *obj = jsV_newobject(J, JS_CSTRING, J->String_prototype);
	obj->u.string = v;
	{
		js_Property *ref;
		ref = jsV_setproperty(J, obj, "length");
		ref->value.type = JS_TNUMBER;
		ref->value.u.number = utflen(v);
		ref->atts = JS_READONLY | JS_DONTENUM | JS_DONTDELETE;
	}
	return obj;
}

/* ToObject() on a value */
js_Object *jsV_toobject(js_State *J, const js_Value *v)
{
	switch (v->type) {
	case JS_TUNDEFINED: js_typeerror(J, "cannot convert undefined to object");
	case JS_TNULL: js_typeerror(J, "cannot convert null to object");
	case JS_TBOOLEAN: return jsV_newboolean(J, v->u.boolean);
	case JS_TNUMBER: return jsV_newnumber(J, v->u.number);
	case JS_TSTRING: return jsV_newstring(J, v->u.string);
	case JS_TOBJECT: return v->u.object;
	}
	js_typeerror(J, "cannot convert value to object");
}

void js_newobject(js_State *J)
{
	js_pushobject(J, jsV_newobject(J, JS_COBJECT, J->Object_prototype));
}

void js_newarray(js_State *J)
{
	js_pushobject(J, jsV_newobject(J, JS_CARRAY, J->Array_prototype));
}

void js_newboolean(js_State *J, int v)
{
	js_pushobject(J, jsV_newboolean(J, v));
}

void js_newnumber(js_State *J, double v)
{
	js_pushobject(J, jsV_newnumber(J, v));
}

void js_newstring(js_State *J, const char *v)
{
	js_pushobject(J, jsV_newstring(J, v));
}

void js_newfunction(js_State *J, js_Function *fun, js_Environment *scope)
{
	js_Object *obj = jsV_newobject(J, JS_CFUNCTION, J->Function_prototype);
	obj->u.f.function = fun;
	obj->u.f.scope = scope;
	js_pushobject(J, obj);
	{
		js_pushnumber(J, fun->numparams);
		js_defproperty(J, -2, "length", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);
		js_newobject(J);
		{
			js_copy(J, -2);
			js_defproperty(J, -2, "constructor", JS_DONTENUM);
		}
		js_defproperty(J, -2, "prototype", JS_DONTDELETE);
	}
}

void js_newscript(js_State *J, js_Function *fun)
{
	js_Object *obj = jsV_newobject(J, JS_CSCRIPT, NULL);
	obj->u.f.function = fun;
	obj->u.f.scope = NULL;
	js_pushobject(J, obj);
}

void js_newcfunction(js_State *J, js_CFunction cfun, int length)
{
	js_Object *obj = jsV_newobject(J, JS_CCFUNCTION, J->Function_prototype);
	obj->u.c.function = cfun;
	obj->u.c.constructor = NULL;
	js_pushobject(J, obj);
	{
		js_pushnumber(J, length);
		js_defproperty(J, -2, "length", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);
		js_newobject(J);
		{
			js_copy(J, -2);
			js_defproperty(J, -2, "constructor", JS_DONTENUM);
		}
		js_defproperty(J, -2, "prototype", JS_DONTDELETE);
	}
}

/* prototype -- constructor */
void js_newcconstructor(js_State *J, js_CFunction cfun, js_CFunction ccon, int length)
{
	js_Object *obj = jsV_newobject(J, JS_CCFUNCTION, J->Function_prototype);
	obj->u.c.function = cfun;
	obj->u.c.constructor = ccon;
	js_pushobject(J, obj); /* proto obj */
	{
		js_pushnumber(J, length);
		js_defproperty(J, -2, "length", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);
		js_rot2(J); /* obj proto */
		js_copy(J, -2); /* obj proto obj */
		js_defproperty(J, -2, "constructor", JS_DONTENUM);
		js_defproperty(J, -2, "prototype", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);
	}
}

void js_newuserdata(js_State *J, const char *tag, void *data)
{
	js_Object *prototype = NULL;
	js_Object *obj;

	if (js_isobject(J, -1))
		prototype = js_toobject(J, -1);
	js_pop(J, 1);

	obj = jsV_newobject(J, JS_CUSERDATA, prototype);
	obj->u.user.tag = tag;
	obj->u.user.data = data;
	js_pushobject(J, obj);
}

/* Non-trivial operations on values. These are implemented using the stack. */

int js_instanceof(js_State *J)
{
	js_Object *O, *V;

	if (!js_iscallable(J, -1))
		js_typeerror(J, "instanceof: invalid operand");

	if (!js_isobject(J, -2))
		return 0;

	js_getproperty(J, -1, "prototype");
	if (!js_isobject(J, -1))
		js_typeerror(J, "instanceof: 'prototype' property is not an object");
	O = js_toobject(J, -1);
	js_pop(J, 1);

	V = js_toobject(J, -2);
	while (V) {
		V = V->prototype;
		if (O == V)
			return 1;
	}

	return 0;
}

void js_concat(js_State *J)
{
	js_Value va = js_toprimitive(J, -2, JS_HNONE);
	js_Value vb = js_toprimitive(J, -1, JS_HNONE);
	if (va.type == JS_TSTRING || vb.type == JS_TSTRING) {
		const char *sa = jsV_tostring(J, &va);
		const char *sb = jsV_tostring(J, &vb);
		char *sab = malloc(strlen(sa) + strlen(sb) + 1);
		strcpy(sab, sa);
		strcat(sab, sb);
		if (js_try(J)) {
			free(sab);
			js_throw(J);
		}
		js_pop(J, 2);
		js_pushstring(J, sab);
		js_endtry(J);
		free(sab);
	} else {
		double x = jsV_tonumber(J, &va);
		double y = jsV_tonumber(J, &vb);
		js_pop(J, 2);
		js_pushnumber(J, x + y);
	}
}

int js_compare(js_State *J)
{
	js_Value va = js_toprimitive(J, -2, JS_HNUMBER);
	js_Value vb = js_toprimitive(J, -1, JS_HNUMBER);

	if (va.type == JS_TSTRING && vb.type == JS_TSTRING) {
		return strcmp(va.u.string, vb.u.string);
	} else {
		double x = jsV_tonumber(J, &va);
		double y = jsV_tonumber(J, &vb);
		return x < y ? -1 : x > y ? 1 : 0;
	}
}

int js_equal(js_State *J)
{
	js_Value va = js_tovalue(J, -2);
	js_Value vb = js_tovalue(J, -1);

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
		return va.u.number == jsV_tonumber(J, &vb);
	if ((va.type == JS_TSTRING || va.type == JS_TBOOLEAN) && vb.type == JS_TNUMBER)
		return jsV_tonumber(J, &va) == vb.u.number;

	if ((va.type == JS_TSTRING || va.type == JS_TNUMBER) && vb.type == JS_TOBJECT) {
		vb = jsV_toprimitive(J, &vb, JS_HNONE);
		goto retry;
	}
	if (va.type == JS_TOBJECT && (vb.type == JS_TSTRING || vb.type == JS_TNUMBER)) {
		va = jsV_toprimitive(J, &va, JS_HNONE);
		goto retry;
	}

	return 0;
}

int js_strictequal(js_State *J)
{
	js_Value va = js_tovalue(J, -2);
	js_Value vb = js_tovalue(J, -1);

	if (va.type != vb.type) return 0;
	if (va.type == JS_TUNDEFINED) return 1;
	if (va.type == JS_TNULL) return 1;
	if (va.type == JS_TNUMBER) return va.u.number == vb.u.number;
	if (va.type == JS_TBOOLEAN) return va.u.boolean == vb.u.boolean;
	if (va.type == JS_TSTRING) return !strcmp(va.u.string, vb.u.string);
	if (va.type == JS_TOBJECT) return va.u.object == vb.u.object;
	return 0;
}
