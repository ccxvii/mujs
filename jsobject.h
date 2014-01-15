#ifndef js_object_h
#define js_object_h

enum js_ValueType {
	JS_TUNDEFINED,
	JS_TNULL,
	JS_TBOOLEAN,
	JS_TNUMBER,
	JS_TSTRING,
	JS_TOBJECT,
	JS_TCLOSURE,
	JS_TCFUNCTION,
};

struct js_Value
{
	union {
		int boolean;
		double number;
		const char *string;
		js_Object *object;
		js_Closure *closure;
		js_CFunction cfunction;
	} u;
	js_ValueType type;
};

struct js_Property
{
	char *name;
	js_Property *left, *right;
	int level;
	js_Value value;
};

struct js_Object
{
	js_Property *properties;
	js_Object *prototype;
	js_Object *outer;
};

js_Object *js_newobject(js_State *J);
js_Property *js_getproperty(js_State *J, js_Object *obj, const char *name);
js_Property *js_setproperty(js_State *J, js_Object *obj, const char *name);
void js_deleteproperty(js_State *J, js_Object *obj, const char *name);

js_Property *js_firstproperty(js_State *J, js_Object *obj);
js_Property *js_nextproperty(js_State *J, js_Object *obj, const char *name);

void js_dumpobject(js_State *J, js_Object *obj);

#endif
