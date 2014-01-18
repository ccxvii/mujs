#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_Function(js_State *J, int n) { return 0; }

void jsB_initfunction(js_State *J)
{
	J->Function_prototype = jsR_newobject(J, JS_COBJECT, NULL);
	js_newcfunction(J, jsB_Function);
	js_pushobject(J, J->Function_prototype);
	js_setproperty(J, -2, "prototype");
	js_setglobal(J, "Function");
}
