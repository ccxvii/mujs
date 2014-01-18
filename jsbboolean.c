#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_Boolean(js_State *J, int n) { return 0; }

void jsB_initboolean(js_State *J)
{
	J->Boolean_prototype = jsR_newobject(J, JS_COBJECT, NULL);
	js_newcfunction(J, jsB_Boolean);
	js_pushobject(J, J->Boolean_prototype);
	js_setproperty(J, -2, "prototype");
	js_setglobal(J, "Boolean");
}
