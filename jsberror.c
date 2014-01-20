#include "jsi.h"
#include "jsobject.h"
#include "jsbuiltin.h"

static int Ep_toString(js_State *J, int n)
{
	js_getproperty(J, 0, "name");
	js_pushliteral(J, ": ");
	js_concat(J);
	js_getproperty(J, 0, "message");
	js_concat(J);
	return 1;
}

#define STRSTR(X) #X
#define STR(X) STRSTR(X)

#define DECL(NAME) \
	static int jsB_new_##NAME(js_State *J, int n) { \
		js_pushobject(J, jsR_newobject(J, JS_CERROR, J->NAME##_prototype)); \
		if (n > 0) { \
			js_pushstring(J, js_tostring(J, 0)); \
			js_setproperty(J, -2, "message"); \
		} \
		return 1; \
	} \
	static int jsB_##NAME(js_State *J, int n) { \
		js_pushobject(J, jsR_newobject(J, JS_CERROR, J->NAME##_prototype)); \
		if (n > 1) { \
			js_pushstring(J, js_tostring(J, 1)); \
			js_setproperty(J, -2, "message"); \
		} \
		return 1; \
	} \
	static void jsB_init##NAME(js_State *J) { \
		js_pushobject(J, jsR_newcconstructor(J, jsB_##NAME, jsB_new_##NAME)); \
		{ \
			jsB_propn(J, "length", 1); \
			js_pushobject(J, J->NAME##_prototype); \
			{ \
				js_copy(J, -2); \
				js_setproperty(J, -2, "constructor"); \
				jsB_props(J, "name", STR(NAME)); \
				jsB_props(J, "message", "an error has occurred"); \
				jsB_propf(J, "toString", Ep_toString, 0); \
			} \
			js_setproperty(J, -2, "prototype"); \
		} \
		js_setglobal(J, STR(NAME)); \
	} \
	void jsR_throw##NAME(js_State *J, const char *message) { \
		js_pushobject(J, jsR_newobject(J, JS_CERROR, J->NAME##_prototype)); \
		js_pushstring(J, message); \
		js_setproperty(J, -2, "message"); \
		js_throw(J); \
	} \

DECL(Error);
DECL(EvalError);
DECL(RangeError);
DECL(ReferenceError);
DECL(SyntaxError);
DECL(TypeError);
DECL(URIError);

void jsB_initerror(js_State *J)
{
	jsB_initError(J);
	jsB_initEvalError(J);
	jsB_initRangeError(J);
	jsB_initReferenceError(J);
	jsB_initSyntaxError(J);
	jsB_initTypeError(J);
	jsB_initURIError(J);
}

void js_error(js_State *J, const char *fmt, ...)
{
	va_list ap;
	char buf[256];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	jsR_throwError(J, buf);
}
