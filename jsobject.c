#include "jsi.h"
#include "jscompile.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsutf.h"

static js_Object *jsR_newfunction(js_State *J, js_Function *function, js_Environment *scope)
{
	js_Object *obj = jsR_newobject(J, JS_CFUNCTION, J->Function_prototype);
	obj->u.f.function = function;
	obj->u.f.scope = scope;
	return obj;
}

static js_Object *jsR_newscript(js_State *J, js_Function *function)
{
	js_Object *obj = jsR_newobject(J, JS_CSCRIPT, NULL);
	obj->u.f.function = function;
	obj->u.f.scope = NULL;
	return obj;
}

static js_Object *jsR_newcfunction(js_State *J, js_CFunction cfunction)
{
	js_Object *obj = jsR_newobject(J, JS_CCFUNCTION, J->Function_prototype);
	obj->u.c.function = cfunction;
	obj->u.c.constructor = NULL;
	return obj;
}

js_Object *jsR_newcconstructor(js_State *J, js_CFunction cfunction, js_CFunction cconstructor)
{
	js_Object *obj = jsR_newobject(J, JS_CCFUNCTION, J->Function_prototype);
	obj->u.c.function = cfunction;
	obj->u.c.constructor = cconstructor;
	return obj;
}

js_Object *jsR_newboolean(js_State *J, int v)
{
	js_Object *obj = jsR_newobject(J, JS_CBOOLEAN, J->Boolean_prototype);
	obj->u.boolean = v;
	return obj;
}

js_Object *jsR_newnumber(js_State *J, double v)
{
	js_Object *obj = jsR_newobject(J, JS_CNUMBER, J->Number_prototype);
	obj->u.number = v;
	return obj;
}

js_Object *jsR_newstring(js_State *J, const char *v)
{
	js_Object *obj = jsR_newobject(J, JS_CSTRING, J->String_prototype);
	obj->u.string = v;
	{
		js_Property *ref;
		ref = jsR_setproperty(J, obj, "length");
		ref->value.type = JS_TNUMBER;
		ref->value.u.number = utflen(v);
		ref->readonly = 1;
		ref->dontenum = 1;
		ref->dontconf = 1;
	}
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
	{
		js_pushnumber(J, F->numparams);
		js_setproperty(J, -2, "length");
		js_newobject(J);
		{
			js_copy(J, -2);
			js_setproperty(J, -2, "constructor");
		}
		js_setproperty(J, -2, "prototype");
	}
}

void js_newscript(js_State *J, js_Function *F)
{
	js_pushobject(J, jsR_newscript(J, F));
}

void js_newcfunction(js_State *J, js_CFunction fun, int length)
{
	js_pushobject(J, jsR_newcfunction(J, fun));
	{
		js_pushnumber(J, length);
		js_setproperty(J, -2, "length");
		js_newobject(J);
		{
			js_copy(J, -2);
			js_setproperty(J, -2, "constructor");
		}
		js_setproperty(J, -2, "prototype");
	}
}
