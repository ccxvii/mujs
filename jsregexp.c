#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

#define nelem(a) (sizeof (a) / sizeof (a)[0])

#include <regex.h>

void js_newregexp(js_State *J, const char *pattern, int flags)
{
	char msg[256];
	js_Object *obj;
	regex_t *prog;
	int opts, status;

	obj = jsV_newobject(J, JS_CREGEXP, J->RegExp_prototype);

	opts = REG_EXTENDED;
	if (flags & JS_REGEXP_I) opts |= REG_ICASE;
	if (flags & JS_REGEXP_M) opts |= REG_NEWLINE;

	prog = malloc(sizeof (regex_t));
	status = regcomp(prog, pattern, opts);
	if (status) {
		free(prog);
		regerror(status, prog, msg, sizeof msg);
		js_syntaxerror(J, "%s", msg);
	}

	obj->u.r.prog = prog;
	obj->u.r.source = pattern;
	obj->u.r.flags = flags;
	obj->u.r.last = 0;
	js_pushobject(J, obj);
}

int js_RegExp_prototype_exec(js_State *J, js_Regexp *re, const char *text)
{
	regmatch_t m[10];
	int opts;
	int i;

	opts = 0;
	if (re->flags & JS_REGEXP_G) {
		if (re->last < 0 || re->last > strlen(text)) {
			re->last = 0;
			js_pushnull(J);
			return 1;
		}
		if (re->last > 0) {
			text += re->last;
			opts |= REG_NOTBOL;
		}
	}

	if (!regexec(re->prog, text, nelem(m), m, opts)) {
		js_newarray(J);
		for (i = 0; i < nelem(m) && m[i].rm_so >= 0; ++i) {
			js_pushlstring(J, text + m[i].rm_so, m[i].rm_eo - m[i].rm_so);
			js_setindex(J, -2, i);
		}
		if (re->flags & JS_REGEXP_G)
			re->last = re->last + m[0].rm_eo;
		return 1;
	}

	if (re->flags & JS_REGEXP_G)
		re->last = 0;

	js_pushnull(J);
	return 1;
}

static int Rp_test(js_State *J, int argc)
{
	js_Regexp *re;
	const char *text;
	regmatch_t m[10];
	int opts;

	re = js_toregexp(J, 0);
	text = js_tostring(J, 1);

	opts = 0;
	if (re->flags & JS_REGEXP_G) {
		if (re->last < 0 || re->last > strlen(text)) {
			re->last = 0;
			js_pushboolean(J, 0);
			return 1;
		}
		if (re->last > 0) {
			text += re->last;
			opts |= REG_NOTBOL;
		}
	}

	if (!regexec(re->prog, text, nelem(m), m, opts)) {
		if (re->flags & JS_REGEXP_G)
			re->last = re->last + m[0].rm_eo;
		js_pushboolean(J, 1);
		return 1;
	}

	if (re->flags & JS_REGEXP_G)
		re->last = 0;

	js_pushboolean(J, 0);
	return 1;
}

static int jsB_new_RegExp(js_State *J, int argc)
{
	js_Regexp *old;
	const char *pattern;
	int flags;

	if (js_isregexp(J, 1)) {
		if (argc > 1)
			js_typeerror(J, "cannot supply flags when creating one RegExp from another");
		old = js_toregexp(J, 1);
		pattern = old->source;
		flags = old->flags;
	} else if (js_isundefined(J, 1)) {
		pattern = "";
		flags = 0;
	} else {
		pattern = js_tostring(J, 1);
		flags = 0;
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
	js_Regexp *re;
	char *out;

	re = js_toregexp(J, 0);

	out = malloc(strlen(re->source) + 6); /* extra space for //gim */
	strcpy(out, "/");
	strcat(out, re->source);
	strcat(out, "/");
	if (re->flags & JS_REGEXP_G) strcat(out, "g");
	if (re->flags & JS_REGEXP_I) strcat(out, "i");
	if (re->flags & JS_REGEXP_M) strcat(out, "m");

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
	return js_RegExp_prototype_exec(J, js_toregexp(J, 0), js_tostring(J, 1));
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
