#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"
#include "jsutf.h"

static int jsB_new_String(js_State *J, int n)
{
	js_pushobject(J, jsR_newstring(J, n > 0 ? js_tostring(J, 0) : ""));
	return 1;
}

static int jsB_String(js_State *J, int n)
{
	js_pushliteral(J, n > 0 ? js_tostring(J, 1) : "");
	return 1;
}

static int Sp_toString(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	if (T->type != JS_CSTRING) jsR_error(J, "TypeError");
	js_pushliteral(J, T->primitive.string);
	return 1;
}

static int Sp_valueOf(js_State *J, int n)
{
	js_Object *T = js_toobject(J, 0);
	if (T->type != JS_CSTRING) jsR_error(J, "TypeError");
	js_pushliteral(J, T->primitive.string);
	return 1;
}

static int S_fromCharCode(js_State *J, int n)
{
	int i;
	Rune c;
	char *s = malloc(n * UTFmax + 1), *p = s;
	// TODO: guard malloc with try/catch
	for (i = 0; i < n; i++) {
		c = js_tonumber(J, i + 1); // TODO: ToUInt16()
		p += runetochar(p, &c);
	}
	*p = 0;
	js_pushstring(J, s);
	free(s);
	return 1;
}

void jsB_initstring(js_State *J)
{
	J->String_prototype->primitive.string = "";

	js_pushobject(J, jsR_newcconstructor(J, jsB_String, jsB_new_String));
	{
		jsB_propn(J, "length", 1);

		js_pushobject(J, J->String_prototype);
		{
			js_copy(J, -2);
			js_setproperty(J, -2, "constructor");
			jsB_propf(J, "toString", Sp_toString, 0);
			jsB_propf(J, "valueOf", Sp_valueOf, 0);
		}
		js_setproperty(J, -2, "prototype");

		jsB_propf(J, "fromCharCode", S_fromCharCode, 1);
	}
	js_setglobal(J, "String");
}
