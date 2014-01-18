#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_new_Object(js_State *J, int n) {
	if (n == 0 || js_isundefined(J, 0) || js_isnull(J, 0))
		js_newobject(J);
	else
		js_pushobject(J, js_toobject(J, 0));
	return 1;
}

static int jsB_Object(js_State *J, int n) {
	if (n == 0 || js_isundefined(J, 1) || js_isnull(J, 1))
		js_newobject(J);
	else
		js_pushobject(J, js_toobject(J, 1));
	return 1;
}

static int jsB_Object_p_toString(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	switch (T->type) {
	case JS_COBJECT: js_pushliteral(J, "[object Object]"); break;
	case JS_CARRAY: js_pushliteral(J, "[object Array]"); break;
	case JS_CFUNCTION: js_pushliteral(J, "[object Function]"); break;
	case JS_CSCRIPT: js_pushliteral(J, "[object Function]"); break;
	case JS_CCFUNCTION: js_pushliteral(J, "[object Function]"); break;
	case JS_CERROR: js_pushliteral(J, "[object Error]"); break;
	case JS_CBOOLEAN: js_pushliteral(J, "[object Boolean]"); break;
	case JS_CNUMBER: js_pushliteral(J, "[object Number]"); break;
	case JS_CSTRING: js_pushliteral(J, "[object String]"); break;
	case JS_CREGEXP: js_pushliteral(J, "[object RegExp]"); break;
	case JS_CDATE: js_pushliteral(J, "[object Date]"); break;
	case JS_CMATH: js_pushliteral(J, "[object Math]"); break;
	default: return 0;
	}
	return 1;
}

static int jsB_Object_p_valueOf(js_State *J, int n)
{
	/* return the 'this' object */
	return 1;
}

static int jsB_Object_p_hasOwnProperty(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	const char *name = js_tostring(J, 1);
	js_Property *ref = jsR_getownproperty(J, T, name);
	js_pushboolean(J, ref != NULL);
	return 1;
}

static int jsB_Object_p_isPrototypeOf(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	if (js_isobject(J, 1)) {
		js_Object *V = js_toobject(J, 1);
		do {
			V = V->prototype;
			if (V == T) {
				js_pushboolean(J, 1);
				return 1;
			}
		} while (V);
	}
	js_pushboolean(J, 0);
	return 1;
}

static int jsB_Object_p_propertyIsEnumerable(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	const char *name = js_tostring(J, 1);
	js_Property *ref = jsR_getownproperty(J, T, name);
	js_pushboolean(J, ref && (ref->flags & JS_PENUMERABLE));
	return 1;
}

void jsB_initobject(js_State *J)
{
	J->Object_prototype = jsR_newobject(J, JS_COBJECT, NULL);
	js_pushobject(J, jsR_newcconstructor(J, jsB_Object, jsB_new_Object));
	{
		js_pushobject(J, J->Object_prototype);
		{
			js_copy(J, -2);
			js_setproperty(J, -2, "constructor");
			js_newcfunction(J, jsB_Object_p_toString);
			js_dup(J);
			js_setproperty(J, -3, "toString");
			js_setproperty(J, -2, "toLocaleString");
			js_newcfunction(J, jsB_Object_p_valueOf);
			js_setproperty(J, -2, "valueOf");
			js_newcfunction(J, jsB_Object_p_hasOwnProperty);
			js_setproperty(J, -2, "hasOwnProperty");
			js_newcfunction(J, jsB_Object_p_isPrototypeOf);
			js_setproperty(J, -2, "isPrototypeOf");
			js_newcfunction(J, jsB_Object_p_propertyIsEnumerable);
			js_setproperty(J, -2, "propertyIsEnumerable");
		}
		js_setproperty(J, -2, "prototype");
	}
	js_setglobal(J, "Object");
}
