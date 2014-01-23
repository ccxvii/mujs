#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"
#include "jsutf.h"

static int jsB_new_String(js_State *J, int argc)
{
	js_newstring(J, argc > 0 ? js_tostring(J, 1) : "");
	return 1;
}

static int jsB_String(js_State *J, int argc)
{
	js_pushliteral(J, argc > 0 ? js_tostring(J, 1) : "");
	return 1;
}

static int Sp_toString(js_State *J, int argc)
{
	js_Object *self = js_toobject(J, 0);
	if (self->type != JS_CSTRING) js_typeerror(J, "not a string");
	js_pushliteral(J, self->u.string);
	return 1;
}

static int Sp_valueOf(js_State *J, int argc)
{
	js_Object *self = js_toobject(J, 0);
	if (self->type != JS_CSTRING) js_typeerror(J, "not a string");
	js_pushliteral(J, self->u.string);
	return 1;
}

static inline Rune runeAt(const char *s, int i)
{
	Rune rune = 0;
	while (i-- >= 0) {
		rune = *(unsigned char*)s;
		if (rune < Runeself) {
			if (rune == 0)
				return 0;
			++s;
		} else
			s += chartorune(&rune, s);
	}
	return rune;
}

static int Sp_charAt(js_State *J, int argc)
{
	char buf[UTFmax + 1];
	const char *s = js_tostring(J, 0);
	int pos = js_tointeger(J, 1);
	Rune rune = runeAt(s, pos);
	if (rune > 0) {
		buf[runetochar(buf, &rune)] = 0;
		js_pushstring(J, buf);
	} else {
		js_pushliteral(J, "");
	}
	return 1;
}

static int Sp_charCodeAt(js_State *J, int argc)
{
	const char *s = js_tostring(J, 0);
	int pos = js_tointeger(J, 1);
	Rune rune = runeAt(s, pos);
	if (rune > 0)
		js_pushnumber(J, rune);
	else
		js_pushnumber(J, NAN);
	return 1;
}

static int S_fromCharCode(js_State *J, int argc)
{
	int i;
	Rune c;
	char *s = malloc(argc * UTFmax + 1), *p = s;
	// TODO: guard malloc with try/catch
	for (i = 0; i <= argc; ++i) {
		c = js_tointeger(J, i + 1); // TODO: ToUInt16()
		p += runetochar(p, &c);
	}
	*p = 0;
	js_pushstring(J, s);
	free(s);
	return 1;
}

void jsB_initstring(js_State *J)
{
	J->String_prototype->u.string = "";

	js_pushobject(J, J->String_prototype);
	{
		jsB_propf(J, "toString", Sp_toString, 0);
		jsB_propf(J, "valueOf", Sp_valueOf, 0);
		jsB_propf(J, "charAt", Sp_charAt, 1);
		jsB_propf(J, "charCodeAt", Sp_charCodeAt, 1);
		//jsB_propf(J, "concat", Sp_concat, 1);
		//jsB_propf(J, "indexOf", Sp_indexOf, 1);
		//jsB_propf(J, "lastIndexOf", Sp_lastIndexOf, 1);
		//jsB_propf(J, "localeCompare", Sp_localeCompare, 1);
		//jsB_propf(J, "slice", Sp_slice, 2);
		// match (uses regexp)
		// replace (uses regexp)
		// search (uses regexp)
		// split (uses regexp)
		//jsB_propf(J, "substring", Sp_substring, 2);
		//jsB_propf(J, "toLowerCase", Sp_toLowerCase, 0);
		//jsB_propf(J, "toUpperCase", Sp_toUpperCase, 0);
	}
	js_newcconstructor(J, jsB_String, jsB_new_String);
	{
		jsB_propf(J, "fromCharCode", S_fromCharCode, 1);
	}
	js_defglobal(J, "String", JS_DONTENUM);
}
