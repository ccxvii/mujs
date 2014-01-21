#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static int jsB_new_Number(js_State *J, int argc)
{
	js_newnumber(J, argc > 0 ? js_tonumber(J, 1) : 0);
	return 1;
}

static int jsB_Number(js_State *J, int argc)
{
	js_pushnumber(J, argc > 0 ? js_tonumber(J, 1) : 0);
	return 1;
}

static int Np_valueOf(js_State *J, int argc)
{
	js_Object *self = js_toobject(J, 0);
	if (self->type != JS_CNUMBER) js_typeerror(J, "not a number");
	js_pushnumber(J, self->u.number);
	return 1;
}

static int Np_toString(js_State *J, int argc)
{
	js_Object *self = js_toobject(J, 0);
	if (self->type != JS_CNUMBER) js_typeerror(J, "not a number");
	js_pushliteral(J, jsV_numbertostring(J, self->u.number));
	return 1;
}

static int Np_toFixed(js_State *J, int argc)
{
	char buf[40];
	js_Object *self = js_toobject(J, 0);
	int width = js_tonumber(J, 1);
	if (self->type != JS_CNUMBER) js_typeerror(J, "not a number");
	sprintf(buf, "%.*f", width, self->u.number);
	js_pushstring(J, buf);
	return 1;
}

static int Np_toExponential(js_State *J, int argc)
{
	char buf[40];
	js_Object *self = js_toobject(J, 0);
	int width = js_tonumber(J, 1);
	if (self->type != JS_CNUMBER) js_typeerror(J, "not a number");
	sprintf(buf, "%.*e", width, self->u.number);
	js_pushstring(J, buf);
	return 1;
}

static int Np_toPrecision(js_State *J, int argc)
{
	char buf[40];
	js_Object *self = js_toobject(J, 0);
	int width = js_tonumber(J, 1);
	if (self->type != JS_CNUMBER) js_typeerror(J, "not a number");
	sprintf(buf, "%.*g", width, self->u.number);
	js_pushstring(J, buf);
	return 1;
}

void jsB_initnumber(js_State *J)
{
	J->Number_prototype->u.number = 0;

	js_pushobject(J, J->Number_prototype);
	{
		jsB_propf(J, "valueOf", Np_valueOf, 0);
		jsB_propf(J, "toString", Np_toString, 1);
		jsB_propf(J, "toLocaleString", Np_toString, 0);
		jsB_propf(J, "toFixed", Np_toFixed, 1);
		jsB_propf(J, "toExponential", Np_toExponential, 1);
		jsB_propf(J, "toPrecision", Np_toPrecision, 1);
	}
	js_newcconstructor(J, jsB_Number, jsB_new_Number);
	{
		jsB_propn(J, "MAX_VALUE", DBL_MAX);
		jsB_propn(J, "MIN_VALUE", DBL_MIN);
		jsB_propn(J, "NaN", NAN);
		jsB_propn(J, "NEGATIVE_INFINITY", -INFINITY);
		jsB_propn(J, "POSITIVE_INFINITY", INFINITY);
	}
	js_setglobal(J, "Number");
}
