#ifndef js_run_h
#define js_run_h

struct js_Environment
{
	js_Environment *outer;
	js_Object *variables;
};

js_Environment *jsR_newenvironment(js_State *J, js_Object *variables, js_Environment *outer);

int jsR_loadstring(js_State *J, const char *filename, const char *source, js_Environment *E);

void js_call(js_State *J, int n);

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
void js_pushstring(js_State *J, const char *v);
void js_newobject(js_State *J);
void js_newarray(js_State *J);
void js_pushcfunction(js_State *J, js_CFunction v);

int js_isundefined(js_State *J, int idx);
int js_isstring(js_State *J, int idx);

int js_toboolean(js_State *J, int idx);
double js_tonumber(js_State *J, int idx);
double js_tointeger(js_State *J, int idx);
const char *js_tostring(js_State *J, int idx);

void js_pop(js_State *J, int n);
void js_dup(js_State *J, int idx);

#endif
