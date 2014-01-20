#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static int jsB_new_Boolean(js_State *J, int n)
{
	js_newboolean(J, js_toboolean(J, 0));
	return 1;
}

static int jsB_Boolean(js_State *J, int n)
{
	js_pushboolean(J, js_toboolean(J, 1));
	return 1;
}

static int Bp_toString(js_State *J, int n)
{
	js_Object *self = js_toobject(J, 0);
	if (self->type != JS_CBOOLEAN) jsR_throwTypeError(J, "not a boolean");
	js_pushliteral(J, self->u.boolean ? "true" : "false");
	return 1;
}

static int Bp_valueOf(js_State *J, int n)
{
	js_Object *self = js_toobject(J, 0);
	if (self->type != JS_CBOOLEAN) jsR_throwTypeError(J, "not a boolean");
	js_pushboolean(J, self->u.boolean);
	return 1;
}

void jsB_initboolean(js_State *J)
{
	J->Boolean_prototype->u.boolean = 0;

	js_pushobject(J, J->Boolean_prototype);
	{
		jsB_propf(J, "toString", Bp_toString, 0);
		jsB_propf(J, "valueOf", Bp_valueOf, 0);
	}
	js_newcconstructor(J, jsB_Boolean, jsB_new_Boolean);
	js_setglobal(J, "Boolean");
}
