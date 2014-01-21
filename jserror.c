#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

#define QQ(X) #X
#define Q(X) QQ(X)

static int Ep_toString(js_State *J, int argc)
{
	js_getproperty(J, 0, "name");
	js_pushliteral(J, ": ");
	js_concat(J);
	js_getproperty(J, 0, "message");
	js_concat(J);
	return 1;
}

static int jsB_ErrorX(js_State *J, int argc, js_Object *prototype)
{
	js_pushobject(J, jsV_newobject(J, JS_CERROR, prototype));
	if (argc > 0) {
		js_pushstring(J, js_tostring(J, 1));
		js_setproperty(J, -2, "message");
	}
	return 1;
}

static void js_newerrorx(js_State *J, const char *message, js_Object *prototype)
{
	js_pushobject(J, jsV_newobject(J, JS_CERROR, prototype));
	js_pushstring(J, message);
	js_setproperty(J, -2, "message");
}

#define DERROR(name, Name) \
	static int jsB_##Name(js_State *J, int n) { \
		return jsB_ErrorX(J, n, J->Name##_prototype); \
	} \
	void js_new##name(js_State *J, const char *s) { \
		js_newerrorx(J, s, J->Name##_prototype); \
	} \
	void js_##name(js_State *J, const char *fmt, ...) { \
		va_list ap; \
		char buf[256]; \
		va_start(ap, fmt); \
		vsnprintf(buf, sizeof buf, fmt, ap); \
		va_end(ap); \
		js_newerrorx(J, buf, J->Name##_prototype); \
		js_throw(J); \
	}

DERROR(error, Error)
DERROR(evalerror, EvalError)
DERROR(rangeerror, RangeError)
DERROR(referenceerror, ReferenceError)
DERROR(syntaxerror, SyntaxError)
DERROR(typeerror, TypeError)
DERROR(urierror, URIError)

#undef DERROR

void jsB_initerror(js_State *J)
{
	js_pushobject(J, J->Error_prototype);
	{
			jsB_props(J, "name", "Error");
			jsB_props(J, "message", "an error has occurred");
			jsB_propf(J, "toString", Ep_toString, 0);
	}
	js_newcconstructor(J, jsB_Error, jsB_Error);
	js_setglobal(J, "Error");

	#define IERROR(NAME) \
		js_pushobject(J, J->NAME##_prototype); \
		jsB_props(J, "name", Q(NAME)); \
		js_newcconstructor(J, jsB_##NAME, jsB_##NAME); \
		js_setglobal(J, Q(NAME));

	IERROR(EvalError);
	IERROR(RangeError);
	IERROR(ReferenceError);
	IERROR(SyntaxError);
	IERROR(TypeError);
	IERROR(URIError);

	#undef IERROR
}
