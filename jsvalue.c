#include "jsi.h"
#include "jslex.h"
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

short jsV_numbertoint16(double n)
{
	return jsV_numbertoint32(n);
}

unsigned short jsV_numbertouint16(double n)
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
	default:
	case JS_TUNDEFINED: return 0;
	case JS_TNULL: return 0;
	case JS_TBOOLEAN: return v->u.boolean;
	case JS_TNUMBER: return v->u.number != 0 && !isnan(v->u.number);
	case JS_TSTRING: return v->u.string[0] != 0;
	case JS_TOBJECT: return 1;
	}
}

double js_stringtofloat(const char *s, char **ep)
{
	char *end;
	double n;
	const char *e = s;
	if (*e == '+' || *e == '-') ++e;
	while (*e >= '0' && *e <= '9') ++e;
	if (*e == '.') ++e;
	while (*e >= '0' && *e <= '9') ++e;
	if (*e == 'e' || *e == 'E') {
		++e;
		if (*e == '+' || *e == '-') ++e;
		while (*e >= '0' && *e <= '9') ++e;
	}
	n = js_strtod(s, &end);
	if (end == e) {
		*ep = (char*)e;
		return n;
	}
	*ep = (char*)s;
	return 0;
}

/* ToNumber() on a string */
double jsV_stringtonumber(js_State *J, const char *s)
{
	char *e;
	double n;
	while (jsY_iswhite(*s) || jsY_isnewline(*s)) ++s;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X') && s[2] != 0)
		n = strtol(s + 2, &e, 16);
	else if (!strncmp(s, "Infinity", 8))
		n = INFINITY, e = (char*)s + 8;
	else if (!strncmp(s, "+Infinity", 9))
		n = INFINITY, e = (char*)s + 9;
	else if (!strncmp(s, "-Infinity", 9))
		n = -INFINITY, e = (char*)s + 9;
	else
		n = js_stringtofloat(s, &e);
	while (jsY_iswhite(*e) || jsY_isnewline(*e)) ++e;
	if (*e) return NAN;
	return n;
}

/* ToNumber() on a value */
double jsV_tonumber(js_State *J, const js_Value *v)
{
	switch (v->type) {
	default:
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
}

double jsV_tointeger(js_State *J, const js_Value *v)
{
	return jsV_numbertointeger(jsV_tonumber(J, v));
}

/* ToString() on a number */
const char *jsV_numbertostring(js_State *J, double f)
{
	char buf[32], digits[32], *p = buf, *s = digits;
	int exp, neg, ndigits, point;

	if (isnan(f)) return "NaN";
	if (isinf(f)) return f < 0 ? "-Infinity" : "Infinity";
	if (f == 0) return "0";

	js_dtoa(f, digits, &exp, &neg, &ndigits);
	point = ndigits + exp;

	if (neg)
		*p++ = '-';

	if (point < -5 || point > 21) {
		*p++ = *s++;
		if (ndigits > 1) {
			int n = ndigits - 1;
			*p++ = '.';
			while (n--)
				*p++ = *s++;
		}
		js_fmtexp(p, point - 1);
	}

	else if (point <= 0) {
		*p++ = '0';
		*p++ = '.';
		while (point++ < 0)
			*p++ = '0';
		while (ndigits-- > 0)
			*p++ = *s++;
		*p = 0;
	}

	else {
		while (ndigits-- > 0) {
			*p++ = *s++;
			if (--point == 0 && ndigits > 0)
				*p++ = '.';
		}
		while (point-- > 0)
			*p++ = '0';
		*p = 0;
	}

	return js_intern(J, buf);
}

/* ToString() on a value */
const char *jsV_tostring(js_State *J, const js_Value *v)
{
	switch (v->type) {
	default:
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
	obj->u.s.string = v;
	obj->u.s.length = utflen(v);
	return obj;
}

/* ToObject() on a value */
js_Object *jsV_toobject(js_State *J, const js_Value *v)
{
	switch (v->type) {
	default:
	case JS_TUNDEFINED: js_typeerror(J, "cannot convert undefined to object");
	case JS_TNULL: js_typeerror(J, "cannot convert null to object");
	case JS_TBOOLEAN: return jsV_newboolean(J, v->u.boolean);
	case JS_TNUMBER: return jsV_newnumber(J, v->u.number);
	case JS_TSTRING: return jsV_newstring(J, v->u.string);
	case JS_TOBJECT: return v->u.object;
	}
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
		js_defproperty(J, -2, "length", JS_READONLY | JS_DONTENUM | JS_DONTCONF);
		js_newobject(J);
		{
			js_copy(J, -2);
			js_defproperty(J, -2, "constructor", JS_DONTENUM);
		}
		js_defproperty(J, -2, "prototype", JS_DONTCONF);
	}
}

void js_newscript(js_State *J, js_Function *fun)
{
	js_Object *obj = jsV_newobject(J, JS_CSCRIPT, NULL);
	obj->u.f.function = fun;
	obj->u.f.scope = NULL;
	js_pushobject(J, obj);
}

void js_newcfunction(js_State *J, js_CFunction cfun, unsigned int length)
{
	js_Object *obj = jsV_newobject(J, JS_CCFUNCTION, J->Function_prototype);
	obj->u.c.function = cfun;
	obj->u.c.constructor = NULL;
	obj->u.c.length = length;
	js_pushobject(J, obj);
	{
		js_pushnumber(J, length);
		js_defproperty(J, -2, "length", JS_READONLY | JS_DONTENUM | JS_DONTCONF);
		js_newobject(J);
		{
			js_copy(J, -2);
			js_defproperty(J, -2, "constructor", JS_DONTENUM);
		}
		js_defproperty(J, -2, "prototype", JS_DONTCONF);
	}
}

/* prototype -- constructor */
void js_newcconstructor(js_State *J, js_CFunction cfun, js_CFunction ccon, unsigned int length)
{
	js_Object *obj = jsV_newobject(J, JS_CCFUNCTION, J->Function_prototype);
	obj->u.c.function = cfun;
	obj->u.c.constructor = ccon;
	js_pushobject(J, obj); /* proto obj */
	{
		js_pushnumber(J, length);
		js_defproperty(J, -2, "length", JS_READONLY | JS_DONTENUM | JS_DONTCONF);
		js_rot2(J); /* obj proto */
		js_copy(J, -2); /* obj proto obj */
		js_defproperty(J, -2, "constructor", JS_DONTENUM);
		js_defproperty(J, -2, "prototype", JS_READONLY | JS_DONTENUM | JS_DONTCONF);
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
		char *sab = js_malloc(J, strlen(sa) + strlen(sb) + 1);
		strcpy(sab, sa);
		strcat(sab, sb);
		if (js_try(J)) {
			js_free(J, sab);
			js_throw(J);
		}
		js_pop(J, 2);
		js_pushstring(J, sab);
		js_endtry(J);
		js_free(J, sab);
	} else {
		double x = jsV_tonumber(J, &va);
		double y = jsV_tonumber(J, &vb);
		js_pop(J, 2);
		js_pushnumber(J, x + y);
	}
}

int js_compare(js_State *J, int *okay)
{
	js_Value va = js_toprimitive(J, -2, JS_HNUMBER);
	js_Value vb = js_toprimitive(J, -1, JS_HNUMBER);

	*okay = 1;
	if (va.type == JS_TSTRING && vb.type == JS_TSTRING) {
		return strcmp(va.u.string, vb.u.string);
	} else {
		double x = jsV_tonumber(J, &va);
		double y = jsV_tonumber(J, &vb);
		if (isnan(x) || isnan(y))
			*okay = 0;
		return x < y ? -1 : x > y ? 1 : 0;
	}
}

int js_equal(js_State *J)
{
	js_Value x = js_tovalue(J, -2);
	js_Value y = js_tovalue(J, -1);

retry:
	if (x.type == y.type) {
		if (x.type == JS_TUNDEFINED) return 1;
		if (x.type == JS_TNULL) return 1;
		if (x.type == JS_TNUMBER) return x.u.number == y.u.number;
		if (x.type == JS_TBOOLEAN) return x.u.boolean == y.u.boolean;
		if (x.type == JS_TSTRING) return !strcmp(x.u.string, y.u.string);
		if (x.type == JS_TOBJECT) return x.u.object == y.u.object;
		return 0;
	}

	if (x.type == JS_TNULL && y.type == JS_TUNDEFINED) return 1;
	if (x.type == JS_TUNDEFINED && y.type == JS_TNULL) return 1;

	if (x.type == JS_TNUMBER && y.type == JS_TSTRING)
		return x.u.number == jsV_tonumber(J, &y);
	if (x.type == JS_TSTRING && y.type == JS_TNUMBER)
		return jsV_tonumber(J, &x) == y.u.number;

	if (x.type == JS_TBOOLEAN) {
		x.type = JS_TNUMBER;
		x.u.number = x.u.boolean;
		goto retry;
	}
	if (y.type == JS_TBOOLEAN) {
		y.type = JS_TNUMBER;
		y.u.number = y.u.boolean;
		goto retry;
	}
	if ((x.type == JS_TSTRING || x.type == JS_TNUMBER) && y.type == JS_TOBJECT) {
		y = jsV_toprimitive(J, &y, JS_HNONE);
		goto retry;
	}
	if (x.type == JS_TOBJECT && (y.type == JS_TSTRING || y.type == JS_TNUMBER)) {
		x = jsV_toprimitive(J, &x, JS_HNONE);
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
