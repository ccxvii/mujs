#include "js.h"
#include "jscompile.h"
#include "jsrun.h"
#include "jsobject.h"
#include "jsstate.h"

static js_Object *jsR_newfunction(js_State *J, js_Function *function, js_Environment *scope)
{
	js_Object *obj = jsR_newobject(J, JS_CFUNCTION, J->Function_prototype);
	obj->function = function;
	obj->scope = scope;
	return obj;
}

static js_Object *jsR_newscript(js_State *J, js_Function *function)
{
	js_Object *obj = jsR_newobject(J, JS_CSCRIPT, NULL);
	obj->function = function;
	return obj;
}

static js_Object *jsR_newcfunction(js_State *J, js_CFunction cfunction)
{
	js_Object *obj = jsR_newobject(J, JS_CCFUNCTION, NULL);
	obj->cfunction = cfunction;
	obj->cconstructor = NULL;
	return obj;
}

static js_Object *jsR_newcconstructor(js_State *J, js_CFunction cfunction, js_CFunction cconstructor)
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

void js_newobject(js_State *J)
{
	js_pushobject(J, jsR_newobject(J, JS_COBJECT, J->Object_prototype));
}

void js_newarray(js_State *J)
{
	js_pushobject(J, jsR_newobject(J, JS_CARRAY, J->Array_prototype));
}

void js_newfunction(js_State *J, js_Function *F, js_Environment *scope)
{
	js_pushobject(J, jsR_newfunction(J, F, scope));
	js_pushnumber(J, F->numparams);
	js_setproperty(J, -2, "length");
	js_newobject(J);
	js_copy(J, -2);
	js_setproperty(J, -2, "constructor");
	js_setproperty(J, -2, "prototype");
}

void js_newscript(js_State *J, js_Function *F)
{
	js_pushobject(J, jsR_newscript(J, F));
}

void js_newcfunction(js_State *J, js_CFunction fun)
{
	js_pushobject(J, jsR_newcfunction(J, fun));
	// TODO: length property?
	js_newobject(J);
	js_copy(J, -2);
	js_setproperty(J, -2, "constructor");
	js_setproperty(J, -2, "prototype");
}

void js_newcconstructor(js_State *J, js_CFunction fun, js_CFunction con)
{
	js_pushobject(J, jsR_newcconstructor(J, fun, con));
	// TODO: length property?
	js_newobject(J);
	js_copy(J, -2);
	js_setproperty(J, -2, "constructor");
	js_setproperty(J, -2, "prototype");
}
