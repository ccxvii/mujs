#ifndef js_object_h
#define js_object_h

struct js_Property
{
	char *key;
	js_Property *left, *right;
	int level;
	js_Value value;
};

struct js_Object
{
	js_Property *properties;
	js_Object *prototype;
	js_Object *parent;
};

js_Object *js_newobject(js_State *J);
js_Property *js_getproperty(js_State *J, js_Object *obj, const char *name);
js_Property *js_setproperty(js_State *J, js_Object *obj, const char *name);
void js_deleteproperty(js_State *J, js_Object *obj, const char *name);

void js_dumpobject(js_State *J, js_Object *obj);

#endif
