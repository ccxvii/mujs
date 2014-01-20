#ifndef js_h
#define js_h

#include "jsconf.h"

typedef struct js_State js_State;
typedef int (*js_CFunction)(js_State *J, int argc);

/* Basic functions */

js_State *js_newstate(void);
void js_freestate(js_State *J);

int js_dostring(js_State *J, const char *source);
int js_dofile(js_State *J, const char *filename);

void js_gc(js_State *J, int report);

const char *js_intern(js_State *J, const char *s);

/* Push a new Error object with the formatted message and throw it */
JS_NORETURN void js_error(js_State *J, const char *fmt, ...) JS_PRINTFLIKE(2,3);

/* Property attribute flags */
enum {
	JS_READONLY = 1,
	JS_DONTENUM = 2,
	JS_DONTDELETE = 4,
};

JS_NORETURN void js_throw(js_State *J);

void js_loadstring(js_State *J, const char *filename, const char *source);
void js_loadfile(js_State *J, const char *filename);

void js_call(js_State *J, int n);
void js_construct(js_State *J, int n);

void js_getglobal(js_State *J, const char *name);
void js_setglobal(js_State *J, const char *name);

void js_getownproperty(js_State *J, int idx, const char *name);
void js_getproperty(js_State *J, int idx, const char *name);
void js_setproperty(js_State *J, int idx, const char *name);
void js_cfgproperty(js_State *J, int idx, const char *name, int atts);
void js_delproperty(js_State *J, int idx, const char *name);
int js_nextproperty(js_State *J, int idx);

void js_pushglobal(js_State *J);
void js_pushundefined(js_State *J);
void js_pushnull(js_State *J);
void js_pushboolean(js_State *J, int v);
void js_pushnumber(js_State *J, double v);
void js_pushstring(js_State *J, const char *v);
void js_pushliteral(js_State *J, const char *v);

void js_newobject(js_State *J);
void js_newarray(js_State *J);
void js_newboolean(js_State *J, int v);
void js_newnumber(js_State *J, double v);
void js_newstring(js_State *J, const char *v);
void js_newerror(js_State *J, const char *message);
void js_newcfunction(js_State *J, js_CFunction fun, int length);
void js_newcconstructor(js_State *J, js_CFunction fun, js_CFunction con);

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
const char *js_tostring(js_State *J, int idx);

double js_tointeger(js_State *J, int idx);
int js_toint32(js_State *J, int idx);
unsigned int js_touint32(js_State *J, int idx);
short js_toint16(js_State *J, int idx);
unsigned short js_touint16(js_State *J, int idx);

int js_gettop(js_State *J);
void js_settop(js_State *J, int idx);
void js_pop(js_State *J, int n);
void js_copy(js_State *J, int idx);
void js_remove(js_State *J, int idx);
void js_insert(js_State *J, int idx);
void js_replace(js_State* J, int idx);

void js_concat(js_State *J);
int js_compare(js_State *J);
int js_equal(js_State *J);
int js_strictequal(js_State *J);

#endif
