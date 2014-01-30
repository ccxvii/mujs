#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

#define nelem(a) (sizeof (a) / sizeof (a)[0])

#include <regex.h>

int js_RegExp_prototype_exec(js_State *J, int idx, const char *text)
{
	int flags, opts;
	regex_t *prog;
	regmatch_t m[10];
	char *s;
	int i, n;

	prog = js_toregexp(J, idx, &flags);

	opts = REG_EXTENDED;
	if (flags & JS_REGEXP_I) opts |= REG_ICASE;
	if (flags & JS_REGEXP_M) opts |= REG_NEWLINE;

	if (!regexec(prog, text, nelem(m), m, opts)) {
		js_newarray(J);

		s = malloc(strlen(text) + 1);
		if (js_try(J)) {
			free(s);
			js_throw(J);
		}

		for (i = 0; i < nelem(m) && m[i].rm_so >= 0; ++i) {
			n = m[i].rm_eo - m[i].rm_so;
			memcpy(s, text + m[i].rm_so, n);
			s[n] = 0;
			js_pushstring(J, s);
			js_setindex(J, -2, i);
		}

		js_endtry(J);
		free(s);
		return 1;
	}

	js_pushnull(J);
	return 1;
}

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
	return js_RegExp_prototype_exec(J, 0, js_tostring(J, 1));
}

static int Rp_test(js_State *J, int argc)
{
	int flags;
	regex_t *prog;
	const char *text;

	prog = js_toregexp(J, 0, &flags);
	text = js_tostring(J, 1);

	js_pushboolean(J, !regexec(prog, text, 0, NULL, 0));
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
