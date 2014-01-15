#ifndef js_object_h
#define js_object_h

typedef enum js_Class js_Class;
typedef enum js_Type js_Type;

struct js_Environment
{
	js_Environment *outer;
	js_Object *variables;
};

enum js_Type {
	JS_TUNDEFINED,
	JS_TNULL,
	JS_TBOOLEAN,
	JS_TNUMBER,
	JS_TSTRING,
	JS_TOBJECT,
};

struct js_Value
{
	js_Type type;
	union {
		int boolean;
		double number;
		const char *string;
		js_Object *object;
	} u;
};

enum {
	JS_PWRITABLE = 1,
	JS_PENUMERABLE = 2,
	JS_PCONFIGURABLE = 4,
};

struct js_Property
{
	char *name;
	js_Property *left, *right;
	int level;
	js_Value value;
	int flags;
};

enum js_Class {
	JS_CARRAY,
	JS_CBOOLEAN,
	JS_CCFUNCTION,
	JS_CDATE,
	JS_CERROR,
	JS_CFUNCTION,
	JS_CMATH,
	JS_CNUMBER,
	JS_COBJECT,
	JS_CREGEXP,
	JS_CSTRING,
};

struct js_Object
{
	js_Class type;
	js_Property *properties;
	js_Object *prototype;
	union {
		int boolean;
		double number;
		const char *string;
	} primitive;
	js_Environment *scope;
	js_Function *function;
	js_CFunction cfunction;
};

js_Object *js_newobject(js_State *J, js_Class type);
js_Object *js_newfunction(js_State *J, js_Function *function, js_Environment *scope);
js_Object *js_newcfunction(js_State *J, js_CFunction cfunction);

js_Environment *js_newenvironment(js_State *J, js_Environment *outer, js_Object *vars);
js_Property *js_decvar(js_State *J, js_Environment *E, const char *name);
js_Property *js_getvar(js_State *J, js_Environment *E, const char *name);
js_Property *js_setvar(js_State *J, js_Environment *E, const char *name);

js_Property *js_getproperty(js_State *J, js_Object *obj, const char *name);
js_Property *js_setproperty(js_State *J, js_Object *obj, const char *name);
void js_deleteproperty(js_State *J, js_Object *obj, const char *name);

js_Property *js_firstproperty(js_State *J, js_Object *obj);
js_Property *js_nextproperty(js_State *J, js_Object *obj, const char *name);

void js_dumpobject(js_State *J, js_Object *obj);
void js_dumpvalue(js_State *J, js_Value v);

#endif
