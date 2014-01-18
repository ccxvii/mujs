#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_Array(js_State *J, int n) { return 0; }

void jsB_initarray(js_State *J)
{
	J->Array_prototype = jsR_newobject(J, JS_COBJECT, NULL);
	js_newcfunction(J, jsB_Array);
	js_pushobject(J, J->Array_prototype);
	js_setproperty(J, -2, "prototype");
	js_setglobal(J, "Array");
}
