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

#define DECL(NAME) \
	static int jsB_##NAME(js_State *J, int argc) { \
		js_pushobject(J, jsV_newobject(J, JS_CERROR, J->NAME##_prototype)); \
		if (argc > 0) { \
			js_pushstring(J, js_tostring(J, 1)); \
			js_setproperty(J, -2, "message"); \
		} \
		return 1; \
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
	js_pushobject(J, J->Error_prototype);
	{
			jsB_props(J, "name", "Error");
			jsB_props(J, "message", "an error has occurred");
			jsB_propf(J, "toString", Ep_toString, 0);
	}
	js_newcconstructor(J, jsB_Error, jsB_Error);
	js_setglobal(J, "Error");

	#define INIT(NAME) \
		js_pushobject(J, J->NAME##_prototype); \
		jsB_props(J, "name", Q(NAME)); \
		js_newcconstructor(J, jsB_##NAME, jsB_##NAME); \
		js_setglobal(J, Q(NAME));

	INIT(EvalError);
	INIT(RangeError);
	INIT(ReferenceError);
	INIT(SyntaxError);
	INIT(TypeError);
	INIT(URIError);
}
