#ifndef js_object_h
#define js_object_h

typedef enum js_Type js_Type;
typedef enum js_Class js_Class;

typedef struct js_Property js_Property;

enum js_Type {
	JS_TUNDEFINED,
	JS_TNULL,
	JS_TBOOLEAN,
	JS_TNUMBER,
	JS_TSTRING,
	JS_TOBJECT,
};

enum js_Class {
	JS_COBJECT,
	JS_CARRAY,
	JS_CFUNCTION,
	JS_CSCRIPT, /* function created from global/eval code */
	JS_CCFUNCTION, /* built-in function */
	JS_CERROR,
	JS_CBOOLEAN,
	JS_CNUMBER,
	JS_CSTRING,
	JS_CREGEXP,
	JS_CDATE,
	JS_CMATH,
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

struct js_Object
{
	js_Class type;
	js_Property *properties;
	js_Object *prototype;
	union {
		int boolean;
		double number;
		const char *string;
		struct {
			js_Function *function;
			js_Environment *scope;
		} f;
		struct {
			js_CFunction function;
			js_CFunction constructor;
		} c;
	} u;
	js_Object *gcnext;
	int gcmark;
};

struct js_Property
{
	const char *name;
	js_Property *left, *right;
	int level;
	unsigned short readonly, dontenum, dontconf;
	js_Value value;
};

js_Value js_tovalue(js_State *J, int idx);
js_Value js_toprimitive(js_State *J, int idx, int hint);
js_Object *js_toobject(js_State *J, int idx);

void js_pushvalue(js_State *J, js_Value v);
void js_pushobject(js_State *J, js_Object *v);

/* jsvalue.c */
int jsR_toboolean(js_State *J, const js_Value *v);
double jsR_tonumber(js_State *J, const js_Value *v);
const char *jsR_tostring(js_State *J, const js_Value *v);
js_Object *jsR_toobject(js_State *J, const js_Value *v);
js_Value jsR_toprimitive(js_State *J, const js_Value *v, int preferred);

/* jsproperty.c */
js_Object *jsR_newobject(js_State *J, js_Class type, js_Object *prototype);
js_Property *jsR_getownproperty(js_State *J, js_Object *obj, const char *name);
js_Property *jsR_getproperty(js_State *J, js_Object *obj, const char *name);
js_Property *jsR_setproperty(js_State *J, js_Object *obj, const char *name);
js_Property *jsR_nextproperty(js_State *J, js_Object *obj, const char *name);

/* jsobject.c */
js_Object *jsR_newboolean(js_State *J, int v);
js_Object *jsR_newnumber(js_State *J, double v);
js_Object *jsR_newstring(js_State *J, const char *v);

/* jsrun.c */
void jsR_pushobject(js_State *J, js_Object *v);
js_Object *js_toobject(js_State *J, int idx);

void js_dumpobject(js_State *J, js_Object *obj);
void js_dumpvalue(js_State *J, js_Value v);

#endif
