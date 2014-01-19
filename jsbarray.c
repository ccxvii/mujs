#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_Array(js_State *J, int n) { return 0; }
static int jsB_new_Array(js_State *J, int n) { return 0; }

static int A_isArray(js_State *J, int n)
{
	if (js_isobject(J, 1)) {
		js_Object *T = js_toobject(J, 1);
		js_pushboolean(J, T->type == JS_CARRAY);
		return 1;
	}
	js_pushboolean(J, 0);
	return 0;
}

void jsB_initarray(js_State *J)
{
	js_pushobject(J, jsR_newcconstructor(J, jsB_Array, jsB_new_Array));
	{
		jsB_propn(J, "length", 1);

		js_pushobject(J, J->Array_prototype);
		{
			js_copy(J, -2);
			js_setproperty(J, -2, "constructor");
			jsB_propn(J, "length", 0);
		}
		js_setproperty(J, -2, "prototype");

		/* ECMA-262-5 */
		jsB_propf(J, "isArray", A_isArray, 1);
	}
	js_setglobal(J, "Array");
}
