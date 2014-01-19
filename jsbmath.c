#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int Math_abs(js_State *J, int nargs) {
	return js_pushnumber(J, abs(js_tonumber(J, 1))), 1;
}
static int Math_acos(js_State *J, int nargs) {
	return js_pushnumber(J, acos(js_tonumber(J, 1))), 1;
}
static int Math_asin(js_State *J, int nargs) {
	return js_pushnumber(J, asin(js_tonumber(J, 1))), 1;
}
static int Math_atan(js_State *J, int nargs) {
	return js_pushnumber(J, atan(js_tonumber(J, 1))), 1;
}
static int Math_atan2(js_State *J, int nargs) {
	return js_pushnumber(J, atan2(js_tonumber(J, 1), js_tonumber(J, 2))), 1;
}
static int Math_ceil(js_State *J, int nargs) {
	return js_pushnumber(J, ceil(js_tonumber(J, 1))), 1;
}
static int Math_cos(js_State *J, int nargs) {
	return js_pushnumber(J, cos(js_tonumber(J, 1))), 1;
}
static int Math_exp(js_State *J, int nargs) {
	return js_pushnumber(J, exp(js_tonumber(J, 1))), 1;
}
static int Math_floor(js_State *J, int nargs) {
	return js_pushnumber(J, floor(js_tonumber(J, 1))), 1;
}
static int Math_log(js_State *J, int nargs) {
	return js_pushnumber(J, log(js_tonumber(J, 1))), 1;
}
static int Math_pow(js_State *J, int nargs) {
	return js_pushnumber(J, pow(js_tonumber(J, 1), js_tonumber(J, 2))), 1;
}
static int Math_random(js_State *J, int nargs) {
	return js_pushnumber(J, (double)rand() / (RAND_MAX - 1)), 1;
}
static int Math_round(js_State *J, int nargs) {
	return js_pushnumber(J, round(js_tonumber(J, 1))), 1;
}
static int Math_sin(js_State *J, int nargs) {
	return js_pushnumber(J, sin(js_tonumber(J, 1))), 1;
}
static int Math_sqrt(js_State *J, int nargs) {
	return js_pushnumber(J, sqrt(js_tonumber(J, 1))), 1;
}
static int Math_tan(js_State *J, int nargs) {
	return js_pushnumber(J, tan(js_tonumber(J, 1))), 1;
}

static int Math_max(js_State *J, int nargs)
{
	double n = js_tonumber(J, 1);
	int i;
	for (i = 2; i < nargs; i++) {
		double m = js_tonumber(J, i);
		n = n > m ? n : m;
	}
	js_pushnumber(J, n);
	return 1;
}

static int Math_min(js_State *J, int nargs)
{
	double n = js_tonumber(J, 1);
	int i;
	for (i = 2; i < nargs; i++) {
		double m = js_tonumber(J, i);
		n = n < m ? n : m;
	}
	js_pushnumber(J, n);
	return 1;
}

void jsB_initmath(js_State *J)
{
	js_pushobject(J, jsR_newobject(J, JS_CMATH, J->Object_prototype));
	{
		jsB_propn(J, "E", M_E);
		jsB_propn(J, "LN10", M_LN10);
		jsB_propn(J, "LN2", M_LN2);
		jsB_propn(J, "LOG2E", M_LOG2E);
		jsB_propn(J, "LOG10E", M_LOG10E);
		jsB_propn(J, "PI", M_PI);
		jsB_propn(J, "SQRT1_2", M_SQRT1_2);
		jsB_propn(J, "SQRT2", M_SQRT2);

		jsB_propf(J, "abs", Math_abs, 1);
		jsB_propf(J, "acos", Math_acos, 1);
		jsB_propf(J, "asin", Math_asin, 1);
		jsB_propf(J, "atan", Math_atan, 1);
		jsB_propf(J, "atan2", Math_atan2, 2);
		jsB_propf(J, "ceil", Math_ceil, 1);
		jsB_propf(J, "cos", Math_cos, 1);
		jsB_propf(J, "exp", Math_exp, 1);
		jsB_propf(J, "floor", Math_floor, 1);
		jsB_propf(J, "log", Math_log, 1);
		jsB_propf(J, "max", Math_max, 2);
		jsB_propf(J, "min", Math_min, 2);
		jsB_propf(J, "pow", Math_pow, 2);
		jsB_propf(J, "random", Math_random, 0);
		jsB_propf(J, "round", Math_round, 1);
		jsB_propf(J, "sin", Math_sin, 1);
		jsB_propf(J, "sqrt", Math_sqrt, 1);
		jsB_propf(J, "tan", Math_tan, 1);
	}
	js_setglobal(J, "Math");
}
