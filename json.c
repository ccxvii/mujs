#include "jsi.h"
#include "jslex.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static inline void jsonnext(js_State *J)
{
	J->lookahead = jsY_lexjson(J);
}

static inline int jsonaccept(js_State *J, int t)
{
	if (J->lookahead == t) {
		jsonnext(J);
		return 1;
	}
	return 0;
}

static inline void jsonexpect(js_State *J, int t)
{
	if (!jsonaccept(J, t))
		js_syntaxerror(J, "JSON: unexpected token: %s (expected %s)",
				jsY_tokenstring(J->lookahead), jsY_tokenstring(t));
}

static void jsonvalue(js_State *J)
{
	int i;
	const char *name;

	switch (J->lookahead) {
	case TK_STRING:
		js_pushliteral(J, J->text);
		jsonnext(J);
		break;

	case TK_NUMBER:
		js_pushnumber(J, J->number);
		jsonnext(J);
		break;

	case '{':
		js_newobject(J);
		jsonnext(J);
		if (J->lookahead == '}')
			return;
		do {
			if (J->lookahead != TK_STRING)
				js_syntaxerror(J, "JSON: unexpected token: %s (expected string)", jsY_tokenstring(J->lookahead));
			name = J->text;
			jsonnext(J);
			jsonexpect(J, ':');
			jsonvalue(J);
			js_setproperty(J, -2, name);
		} while (jsonaccept(J, ','));
		jsonexpect(J, '}');
		break;

	case '[':
		js_newarray(J);
		jsonnext(J);
		i = 0;
		if (J->lookahead == ']')
			return;
		do {
			jsonvalue(J);
			js_setindex(J, -2, i++);
		} while (jsonaccept(J, ','));
		jsonexpect(J, ']');
		break;

	case TK_TRUE:
		js_pushboolean(J, 1);
		jsonnext(J);
		break;

	case TK_FALSE:
		js_pushboolean(J, 0);
		jsonnext(J);
		break;

	case TK_NULL:
		js_pushnull(J);
		jsonnext(J);
		break;

	default:
		js_syntaxerror(J, "JSON: unexpected token: %s", jsY_tokenstring(J->lookahead));
	}
}

static int JSON_parse(js_State *J, int argc)
{
	const char *source = js_tostring(J, 1);
	jsY_initlex(J, "JSON", source);
	jsonnext(J);
	jsonvalue(J);
	// TODO: reviver Walk()
	return 1;
}

static int JSON_stringify(js_State *J, int argc)
{
	return 0;
}

void jsB_initjson(js_State *J)
{
	js_pushobject(J, jsV_newobject(J, JS_CJSON, J->Object_prototype));
	{
		jsB_propf(J, "parse", JSON_parse, 2);
		jsB_propf(J, "stringify", JSON_stringify, 3);
	}
	js_defglobal(J, "JSON", JS_DONTENUM);
}
