#ifndef js_object_h
#define js_object_h

typedef struct js_Property js_Property;
typedef struct js_Iterator js_Iterator;

/* Hint to ToPrimitive() */
enum {
	JS_HNONE,
	JS_HNUMBER,
	JS_HSTRING
};

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
	JS_CJSON,
	JS_CITERATOR,
	JS_CUSERDATA,
};

struct js_Value
{
	enum js_Type type;
	union {
		int boolean;
		double number;
		const char *string;
		js_Object *object;
	} u;
};

struct js_Regexp
{
	void *prog;
	const char *source;
	unsigned short flags;
	unsigned short last;
};

struct js_Object
{
	enum js_Class type;
	int extensible;
	js_Property *properties;
	js_Property *head, *tail; /* for enumeration */
	js_Object *prototype;
	union {
		int boolean;
		double number;
		struct {
			const char *string;
			unsigned int length;
		} s;
		struct {
			unsigned int length;
		} a;
		struct {
			js_Function *function;
			js_Environment *scope;
		} f;
		struct {
			js_CFunction function;
			js_CFunction constructor;
			unsigned int length;
		} c;
		js_Regexp r;
		struct {
			js_Object *target;
			js_Iterator *head;
		} iter;
		struct {
			const char *tag;
			void *data;
		} user;
	} u;
	js_Object *gcnext;
	int gcmark;
};

struct js_Property
{
	const char *name;
	js_Property *left, *right;
	js_Property **prevp, *next; /* for enumeration */
	int level;
	int atts;
	js_Value value;
	js_Object *getter;
	js_Object *setter;
};

struct js_Iterator
{
	const char *name;
	js_Iterator *next;
};

/* jsrun.c */
js_Value js_tovalue(js_State *J, int idx);
js_Value js_toprimitive(js_State *J, int idx, int hint);
js_Object *js_toobject(js_State *J, int idx);
void js_pushvalue(js_State *J, js_Value v);
void js_pushobject(js_State *J, js_Object *v);

/* jsvalue.c */
int jsV_toboolean(js_State *J, const js_Value *v);
double jsV_tonumber(js_State *J, const js_Value *v);
double jsV_tointeger(js_State *J, const js_Value *v);
const char *jsV_tostring(js_State *J, const js_Value *v);
js_Object *jsV_toobject(js_State *J, const js_Value *v);
js_Value jsV_toprimitive(js_State *J, const js_Value *v, int preferred);

double js_stringtofloat(const char *s, char **ep);
double jsV_numbertointeger(double n);
int jsV_numbertoint32(double n);
unsigned int jsV_numbertouint32(double n);
short jsV_numbertoint16(double n);
unsigned short jsV_numbertouint16(double n);
const char *jsV_numbertostring(js_State *J, double number);
double jsV_stringtonumber(js_State *J, const char *string);

/* jsproperty.c */
js_Object *jsV_newobject(js_State *J, enum js_Class type, js_Object *prototype);
js_Property *jsV_getownproperty(js_State *J, js_Object *obj, const char *name);
js_Property *jsV_getpropertyx(js_State *J, js_Object *obj, const char *name, int *own);
js_Property *jsV_getproperty(js_State *J, js_Object *obj, const char *name);
js_Property *jsV_setproperty(js_State *J, js_Object *obj, const char *name);
js_Property *jsV_nextproperty(js_State *J, js_Object *obj, const char *name);
void jsV_delproperty(js_State *J, js_Object *obj, const char *name);

js_Object *jsV_newiterator(js_State *J, js_Object *obj, int own);
const char *jsV_nextiterator(js_State *J, js_Object *iter);

void jsV_resizearray(js_State *J, js_Object *obj, unsigned int newlen);

/* jsdump.c */
void js_dumpobject(js_State *J, js_Object *obj);
void js_dumpvalue(js_State *J, js_Value v);

#endif
