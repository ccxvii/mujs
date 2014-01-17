#include "js.h"
#include "jsobject.h"
#include "jsstate.h"

js_Object *jsR_newfunction(js_State *J, js_Function *function, js_Environment *scope)
{
	js_Object *obj = jsR_newobject(J, JS_CFUNCTION, J->Function_prototype);
	obj->function = function;
	obj->scope = scope;
	return obj;
}

js_Object *jsR_newscript(js_State *J, js_Function *function)
{
	js_Object *obj = jsR_newobject(J, JS_CSCRIPT, NULL);
	obj->function = function;
	return obj;
}

js_Object *jsR_newcfunction(js_State *J, js_CFunction cfunction)
{
	js_Object *obj = jsR_newobject(J, JS_CCFUNCTION, NULL);
	obj->cfunction = cfunction;
	obj->cconstructor = NULL;
	return obj;
}

js_Object *jsR_newcconstructor(js_State *J, js_CFunction cfunction, js_CFunction cconstructor)
{
	js_Object *obj = jsR_newobject(J, JS_CCFUNCTION, NULL);
	obj->cfunction = cfunction;
	obj->cconstructor = cconstructor;
	return obj;
}

js_Object *jsR_newboolean(js_State *J, int v)
{
	js_Object *obj = jsR_newobject(J, JS_CBOOLEAN, J->Boolean_prototype);
	obj->primitive.boolean = v;
	return obj;
}

js_Object *jsR_newnumber(js_State *J, double v)
{
	js_Object *obj = jsR_newobject(J, JS_CNUMBER, J->Number_prototype);
	obj->primitive.number = v;
	return obj;
}

js_Object *jsR_newstring(js_State *J, const char *v)
{
	js_Object *obj = jsR_newobject(J, JS_CSTRING, J->String_prototype);
	obj->primitive.string = v;
	return obj;
}
