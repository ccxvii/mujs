#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_new_Number(js_State *J, int n)
{
	js_pushobject(J, jsR_newnumber(J, n > 0 ? js_tonumber(J, 0) : 0));
	return 1;
}

static int jsB_Number(js_State *J, int n)
{
	js_pushnumber(J, n > 0 ? js_tonumber(J, 1) : 0);
	return 1;
}

static int jsB_Number_p_valueOf(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	if (T->type != JS_CNUMBER) jsR_error(J, "TypeError");
	js_pushnumber(J, T->primitive.number);
	return 1;
}

static int jsB_Number_p_toString(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	if (T->type != JS_CNUMBER) jsR_error(J, "TypeError");
	js_pushliteral(J, jsR_numbertostring(J, T->primitive.number));
	return 1;
}

static int jsB_Number_p_toFixed(js_State *J, int n)
{
	char buf[40];
	js_Object *T = js_toobject(J, 0);
	int width = js_tonumber(J, 1);
	if (T->type != JS_CNUMBER) jsR_error(J, "TypeError");
	sprintf(buf, "%*f", width, T->primitive.number);
	js_pushstring(J, buf);
	return 1;
}

static int jsB_Number_p_toExponential(js_State *J, int n)
{
	char buf[40];
	js_Object *T = js_toobject(J, 0);
	int width = js_tonumber(J, 1);
	if (T->type != JS_CNUMBER) jsR_error(J, "TypeError");
	sprintf(buf, "%*e", width, T->primitive.number);
	js_pushstring(J, buf);
	return 1;
}

static int jsB_Number_p_toPrecision(js_State *J, int n)
{
	char buf[40];
	js_Object *T = js_toobject(J, 0);
	int width = js_tonumber(J, 1);
	if (T->type != JS_CNUMBER) jsR_error(J, "TypeError");
	sprintf(buf, "%*g", width, T->primitive.number);
	js_pushstring(J, buf);
	return 1;
}

void jsB_initnumber(js_State *J)
{
	J->Number_prototype = jsR_newobject(J, JS_CNUMBER, J->Object_prototype);
	J->Number_prototype->primitive.number = 0;

	js_pushobject(J, jsR_newcconstructor(J, jsB_Number, jsB_new_Number));
	{
		js_pushobject(J, J->Number_prototype);
		{
			js_copy(J, -2);
			js_setproperty(J, -2, "constructor");
			js_newcfunction(J, jsB_Number_p_valueOf);
			js_setproperty(J, -2, "valueOf");
			js_newcfunction(J, jsB_Number_p_toString);
			js_dup(J);
			js_setproperty(J, -3, "toString");
			js_setproperty(J, -2, "toLocaleString");
			js_newcfunction(J, jsB_Number_p_toFixed);
			js_setproperty(J, -2, "toFixed");
			js_newcfunction(J, jsB_Number_p_toExponential);
			js_setproperty(J, -2, "toExponential");
			js_newcfunction(J, jsB_Number_p_toPrecision);
			js_setproperty(J, -2, "toPrecision");
		}
		js_setproperty(J, -2, "prototype");

		js_pushnumber(J, DBL_MAX);
		js_setproperty(J, -2, "MAX_VALUE");
		js_pushnumber(J, DBL_MIN);
		js_setproperty(J, -2, "MIN_VALUE");
		js_pushnumber(J, NAN);
		js_setproperty(J, -2, "NaN");
		js_pushnumber(J, -INFINITY);
		js_setproperty(J, -2, "NEGATIVE_INFINITY");
		js_pushnumber(J, INFINITY);
		js_setproperty(J, -2, "POSITIVE_INFINITY");
	}
	js_setglobal(J, "Number");
}
