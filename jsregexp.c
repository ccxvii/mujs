#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

void js_newregexp(js_State *J, const char *pattern, int flags)
{
	js_Object *obj;

	obj = jsV_newobject(J, JS_CREGEXP, J->RegExp_prototype);
	obj->u.r.prog = NULL;
	obj->u.r.flags = flags;
	js_pushobject(J, obj);

	js_pushstring(J, pattern);
	js_defproperty(J, -2, "source", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);
	js_pushboolean(J, flags & JS_REGEXP_G);
	js_defproperty(J, -2, "global", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);
	js_pushboolean(J, flags & JS_REGEXP_I);
	js_defproperty(J, -2, "ignoreCase", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);
	js_pushboolean(J, flags & JS_REGEXP_M);
	js_defproperty(J, -2, "multiline", JS_READONLY | JS_DONTENUM | JS_DONTDELETE);

	// TODO: lastIndex
}

static int jsB_new_RegExp(js_State *J, int argc)
{
	const char *pattern;
	int flags;

	if (js_isregexp(J, 1)) {
		if (argc > 1)
			js_typeerror(J, "cannot supply flags when creating one RegExp from another");
		js_toregexp(J, 1, &flags);
		js_getproperty(J, 1, "source");
		pattern = js_tostring(J, -1);
		js_pop(J, 1);
	} else if (js_isundefined(J, 1)) {
		pattern = "";
	} else {
		pattern = js_tostring(J, 1);
	}

	if (argc > 1) {
		const char *s = js_tostring(J, 2);
		int g = 0, i = 0, m = 0;
		while (*s) {
			if (*s == 'g') ++g;
			else if (*s == 'i') ++i;
			else if (*s == 'm') ++m;
			else js_syntaxerror(J, "invalid regular expression flag: '%c'", *s);
			++s;
		}
		if (g > 1) js_syntaxerror(J, "invalid regular expression flag: 'g'");
		if (i > 1) js_syntaxerror(J, "invalid regular expression flag: 'i'");
		if (m > 1) js_syntaxerror(J, "invalid regular expression flag: 'm'");
		if (g) flags |= JS_REGEXP_G;
		if (i) flags |= JS_REGEXP_I;
		if (m) flags |= JS_REGEXP_M;
	}

	js_newregexp(J, pattern, flags);
	return 1;
}

static int jsB_RegExp(js_State *J, int argc)
{
	if (js_isregexp(J, 1) && argc == 1)
		return 1;
	return jsB_new_RegExp(J, argc);
}

static int Rp_toString(js_State *J, int argc)
{
	const char *source;
	int flags;
	char *out;

	js_Object *self = js_toobject(J, 0);
	if (self->type != JS_CREGEXP)
		js_typeerror(J, "not a regexp");

	flags = self->u.r.flags;

	js_getproperty(J, 0, "source");
	source = js_tostring(J, -1);

	out = malloc(strlen(source) + 6); /* extra space for //gim */
	strcpy(out, "/");
	strcat(out, source);
	strcat(out, "/");
	if (flags & JS_REGEXP_G) strcat(out, "g");
	if (flags & JS_REGEXP_I) strcat(out, "i");
	if (flags & JS_REGEXP_M) strcat(out, "m");

	if (js_try(J)) {
		free(out);
		js_throw(J);
	}
	js_pop(J, 0);
	js_pushstring(J, out);
	js_endtry(J);
	free(out);
	return 1;
}

static int Rp_exec(js_State *J, int argc)
{
	js_pushnull(J);
	return 1;
}

static int Rp_test(js_State *J, int argc)
{
	js_pushboolean(J, 0);
	return 1;
}

void jsB_initregexp(js_State *J)
{
	js_pushobject(J, J->RegExp_prototype);
	{
		jsB_propf(J, "toString", Rp_toString, 0);
		jsB_propf(J, "test", Rp_test, 0);
		jsB_propf(J, "exec", Rp_exec, 0);
	}
	js_newcconstructor(J, jsB_RegExp, jsB_new_RegExp, 1);
	js_defglobal(J, "RegExp", JS_DONTENUM);
}
