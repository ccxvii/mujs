#ifndef js_run_h
#define js_run_h

struct js_Environment
{
	js_Environment *outer;
	js_Object *variables;

	js_Environment *gcnext;
	int gcmark;
};

/* private */
void jsB_init(js_State *J);
void jsB_propf(js_State *J, const char *name, js_CFunction cfun, int n);
void jsB_propn(js_State *J, const char *name, double number);

js_Environment *jsR_newenvironment(js_State *J, js_Object *variables, js_Environment *outer);
js_Object *jsR_newcconstructor(js_State *J, js_CFunction cfunction, js_CFunction cconstructor);
int jsR_loadscript(js_State *J, const char *filename, const char *source);
void jsR_error(js_State *J, const char *fmt, ...);
void js_pushobject(js_State *J, js_Object *v);
js_Object *js_toobject(js_State *J, int idx);
js_Value js_toprimitive(js_State *J, int idx, int hint);
js_Value js_tovalue(js_State *J, int idx);
void jsR_concat(js_State *J);
int jsR_compare(js_State *J);
int jsR_equal(js_State *J);
int jsR_strictequal(js_State *J);

const char *jsR_numbertostring(js_State *J, double number);
double jsR_stringtonumber(js_State *J, const char *string);

/* public */

void js_call(js_State *J, int n);
void js_construct(js_State *J, int n);

void js_getglobal(js_State *J, const char *name);
void js_setglobal(js_State *J, const char *name);
void js_getownproperty(js_State *J, int idx, const char *name);
void js_getproperty(js_State *J, int idx, const char *name);
void js_setproperty(js_State *J, int idx, const char *name);
int js_nextproperty(js_State *J, int idx);

void js_pushglobal(js_State *J);
void js_pushundefined(js_State *J);
void js_pushnull(js_State *J);
void js_pushboolean(js_State *J, int v);
void js_pushnumber(js_State *J, double v);
void js_pushliteral(js_State *J, const char *v);
void js_pushstring(js_State *J, const char *v);

void js_newobject(js_State *J);
void js_newarray(js_State *J);
void js_newfunction(js_State *J, js_Function *function, js_Environment *scope);
void js_newscript(js_State *J, js_Function *function);
void js_newcfunction(js_State *J, js_CFunction fun, int length);

const char *js_typeof(js_State *J, int idx);
int js_isundefined(js_State *J, int idx);
int js_isnull(js_State *J, int idx);
int js_isboolean(js_State *J, int idx);
int js_isnumber(js_State *J, int idx);
int js_isstring(js_State *J, int idx);
int js_isprimitive(js_State *J, int idx);
int js_isobject(js_State *J, int idx);
int js_iscallable(js_State *J, int idx);

int js_toboolean(js_State *J, int idx);
double js_tonumber(js_State *J, int idx);
double js_tointeger(js_State *J, int idx);
const char *js_tostring(js_State *J, int idx);

void js_pop(js_State *J, int n);
void js_dup(js_State *J);
void js_copy(js_State *J, int idx);
void js_rot(js_State *J, int n);
void js_rot2(js_State *J);
void js_rot3(js_State *J);

#endif
