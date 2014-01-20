#include "jsi.h"
#include "jsvalue.h"

static void js_newerrorx(js_State *J, const char *message, js_Object *prototype)
{
	js_pushobject(J, jsV_newobject(J, JS_CERROR, prototype));
	js_pushstring(J, message);
	js_setproperty(J, -2, "message");
}

void js_newerror(js_State *J, const char *s) { js_newerrorx(J, s, J->Error_prototype); }
void js_newevalerror(js_State *J, const char *s) { js_newerrorx(J, s, J->EvalError_prototype); }
void js_newrangeerror(js_State *J, const char *s) { js_newerrorx(J, s, J->RangeError_prototype); }
void js_newreferenceerror(js_State *J, const char *s) { js_newerrorx(J, s, J->ReferenceError_prototype); }
void js_newsyntaxerror(js_State *J, const char *s) { js_newerrorx(J, s, J->SyntaxError_prototype); }
void js_newtypeerror(js_State *J, const char *s) { js_newerrorx(J, s, J->TypeError_prototype); }
void js_newurierror(js_State *J, const char *s) { js_newerrorx(J, s, J->URIError_prototype); }

#define ERR(NAME) \
	va_list ap; \
	char buf[256]; \
	va_start(ap, fmt); \
	vsnprintf(buf, sizeof buf, fmt, ap); \
	va_end(ap); \
	js_newerrorx(J, buf, J->NAME##_prototype); \
	js_throw(J)

void js_error(js_State *J, const char *fmt, ...) { ERR(Error); }
void js_evalerror(js_State *J, const char *fmt, ...) { ERR(EvalError); }
void js_rangeerror(js_State *J, const char *fmt, ...) { ERR(RangeError); }
void js_referenceerror(js_State *J, const char *fmt, ...) { ERR(ReferenceError); }
void js_syntaxerror(js_State *J, const char *fmt, ...) { ERR(SyntaxError); }
void js_typeerror(js_State *J, const char *fmt, ...) { ERR(TypeError); }
void js_urierror(js_State *J, const char *fmt, ...) { ERR(URIError); }
