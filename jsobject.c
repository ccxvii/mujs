#include "js.h"
#include "jsobject.h"

js_Object *jsR_newfunction(js_State *J, js_Function *function, js_Environment *scope)
{
	js_Object *obj = jsR_newobject(J, JS_CFUNCTION);
	obj->function = function;
	obj->scope = scope;
	return obj;
}

js_Object *jsR_newcfunction(js_State *J, js_CFunction cfunction)
{
	js_Object *obj = jsR_newobject(J, JS_CCFUNCTION);
	obj->cfunction = cfunction;
	return obj;
}

js_Object *jsR_newboolean(js_State *J, int v)
{
	js_Object *obj = jsR_newobject(J, JS_CBOOLEAN);
	obj->primitive.boolean = v;
	return obj;
}

js_Object *jsR_newnumber(js_State *J, double v)
{
	js_Object *obj = jsR_newobject(J, JS_CNUMBER);
	obj->primitive.number = v;
	return obj;
}

js_Object *jsR_newstring(js_State *J, const char *v)
{
	js_Object *obj = jsR_newobject(J, JS_CSTRING);
	obj->primitive.string = v;
	return obj;
}
