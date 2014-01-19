#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_new_Boolean(js_State *J, int n)
{
	js_pushobject(J, jsR_newboolean(J, js_toboolean(J, 0)));
	return 1;
}

static int jsB_Boolean(js_State *J, int n)
{
	js_pushboolean(J, js_toboolean(J, 1));
	return 1;
}

static int Bp_toString(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	if (T->type != JS_CBOOLEAN) jsR_error(J, "TypeError");
	js_pushliteral(J, T->primitive.boolean ? "true" : "false");
	return 1;
}

static int Bp_valueOf(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	if (T->type != JS_CBOOLEAN) jsR_error(J, "TypeError");
	js_pushboolean(J, T->primitive.boolean);
	return 1;
}

void jsB_initboolean(js_State *J)
{
	J->Boolean_prototype->primitive.boolean = 0;

	js_pushobject(J, jsR_newcconstructor(J, jsB_Boolean, jsB_new_Boolean));
	{
		jsB_propn(J, "length", 1);
		js_pushobject(J, J->Boolean_prototype);
		{
			js_copy(J, -2);
			js_setproperty(J, -2, "constructor");
			jsB_propf(J, "toString", Bp_toString, 0);
			jsB_propf(J, "valueOf", Bp_valueOf, 0);
		}
		js_setproperty(J, -2, "prototype");
	}
	js_setglobal(J, "Boolean");
}
