#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static int JSON_parse(js_State *J, int argc)
{
	return 0;
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
