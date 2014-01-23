#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static int jsB_Array(js_State *J, int argc) { return 0; }
static int jsB_new_Array(js_State *J, int argc) { return 0; }

static int A_isArray(js_State *J, int argc)
{
	if (js_isobject(J, 1)) {
		js_Object *T = js_toobject(J, 1);
		js_pushboolean(J, T->type == JS_CARRAY);
		return 1;
	}
	js_pushboolean(J, 0);
	return 1;
}

void jsB_initarray(js_State *J)
{
	js_pushobject(J, J->Array_prototype);
	js_newcconstructor(J, jsB_Array, jsB_new_Array);
	{
		/* ECMA-262-5 */
		jsB_propf(J, "isArray", A_isArray, 1);
	}
	js_defglobal(J, "Array", JS_DONTENUM);
}
