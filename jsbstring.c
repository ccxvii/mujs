#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_String(js_State *J, int n) { return 0; }

void jsB_initstring(js_State *J)
{
	J->String_prototype = jsR_newobject(J, JS_COBJECT, NULL);
	js_newcfunction(J, jsB_String);
	js_pushobject(J, J->String_prototype);
	js_setproperty(J, -2, "prototype");
	js_setglobal(J, "String");
}
