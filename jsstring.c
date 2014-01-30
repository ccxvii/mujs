#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"
#include "utf.h"

#include <regex.h>

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

static inline const char *utfindex(const char *s, int i)
{
	Rune rune = 0;
	while (i-- > 0) {
		rune = *(unsigned char*)s;
		if (rune < Runeself) {
			if (rune == 0)
				return NULL;
			++s;
		} else
			s += chartorune(&rune, s);
	}
	return s;
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

static int Sp_concat(js_State *J, int argc)
{
	char * volatile out;
	const char *s;
	int i, n;

	if (argc == 0)
		return 1;

	s = js_tostring(J, 0);
	n = strlen(s);
	out = malloc(n + 1);
	strcpy(out, s);

	if (js_try(J)) {
		free(out);
		js_throw(J);
	}

	for (i = 1; i <= argc; ++i) {
		s = js_tostring(J, i);
		n += strlen(s);
		out = realloc(out, n + 1);
		strcat(out, s);
	}

	js_pushstring(J, out);
	js_endtry(J);
	free(out);
	return 1;
}

static int Sp_indexOf(js_State *J, int argc)
{
	const char *haystack = js_tostring(J, 0);
	const char *needle = js_tostring(J, 1);
	int pos = js_tointeger(J, 2);
	int len = strlen(needle);
	int k = 0;
	Rune rune;
	while (*haystack) {
		if (k >= pos && !strncmp(haystack, needle, len)) {
			js_pushnumber(J, k);
			return 1;
		}
		haystack += chartorune(&rune, haystack);
		++k;
	}
	js_pushnumber(J, -1);
	return 1;
}

static int Sp_lastIndexOf(js_State *J, int argc)
{
	const char *haystack = js_tostring(J, 0);
	const char *needle = js_tostring(J, 1);
	int pos = argc > 1 ? js_tointeger(J, 2) : strlen(haystack);
	int len = strlen(needle);
	int k = 0, last = -1;
	Rune rune;
	while (*haystack && k <= pos) {
		if (!strncmp(haystack, needle, len))
			last = k;
		haystack += chartorune(&rune, haystack);
		++k;
	}
	js_pushnumber(J, last);
	return 1;
}

static int Sp_localeCompare(js_State *J, int argc)
{
	const char *a = js_tostring(J, 0);
	const char *b = js_tostring(J, 1);
	return strcmp(a, b);
}

static char *substr(js_State *J, const char *src, int a, int b)
{
	int n = b - a;
	const char *s = utfindex(src, a);
	char *dst = malloc(UTFmax * n + 1), *d = dst;
	while (n--) {
		Rune rune;
		s += chartorune(&rune, s);
		d += runetochar(d, &rune);
	}
	*d = 0;
	return dst;
}

static int Sp_slice(js_State *J, int argc)
{
	const char *str = js_tostring(J, 0);
	int len = utflen(str);
	int s = js_tointeger(J, 1);
	int e = argc > 1 ? js_tointeger(J, 2) : len;
	char *out;

	s = s < 0 ? s + len : s;
	e = e < 0 ? e + len : e;

	s = s < 0 ? 0 : s > len ? len : s;
	e = e < 0 ? 0 : e > len ? len : e;

	if (s < e)
		out = substr(J, str, s, e);
	else
		out = substr(J, str, e, s);

	if (js_try(J)) {
		free(out);
		js_throw(J);
	}
	js_pushstring(J, out);
	js_endtry(J);
	return 1;
}

static int Sp_substring(js_State *J, int argc)
{
	const char *str = js_tostring(J, 0);
	int len = utflen(str);
	int s = js_tointeger(J, 1);
	int e = argc > 1 ? js_tointeger(J, 2) : len;
	char *out;

	s = s < 0 ? 0 : s > len ? len : s;
	e = e < 0 ? 0 : e > len ? len : e;

	if (s < e)
		out = substr(J, str, s, e);
	else
		out = substr(J, str, e, s);

	if (js_try(J)) {
		free(out);
		js_throw(J);
	}
	js_pushstring(J, out);
	js_endtry(J);
	return 1;
}

static int Sp_toLowerCase(js_State *J, int argc)
{
	const char *src = js_tostring(J, 0);
	char *dst = malloc(UTFmax * strlen(src) + 1);
	const char *s = src;
	char *d = dst;
	Rune rune;
	while (*s) {
		s += chartorune(&rune, s);
		rune = tolowerrune(rune);
		d += runetochar(d, &rune);
	}
	if (js_try(J)) {
		free(dst);
		js_throw(J);
	}
	js_pushstring(J, dst);
	js_endtry(J);
	free(dst);
	return 1;
}

static int Sp_toUpperCase(js_State *J, int argc)
{
	const char *src = js_tostring(J, 0);
	char *dst = malloc(UTFmax * strlen(src) + 1);
	const char *s = src;
	char *d = dst;
	Rune rune;
	while (*s) {
		s += chartorune(&rune, s);
		rune = toupperrune(rune);
		d += runetochar(d, &rune);
	}
	if (js_try(J)) {
		free(dst);
		js_throw(J);
	}
	js_pushstring(J, dst);
	js_endtry(J);
	free(dst);
	return 1;
}

static int S_fromCharCode(js_State *J, int argc)
{
	int i;
	Rune c;
	char *s, *p;

	s = p = malloc(argc * UTFmax + 1);

	if (js_try(J)) {
		free(s);
		js_throw(J);
	}

	for (i = 1; i <= argc; ++i) {
		c = js_tointeger(J, i); // TODO: ToUInt16()
		p += runetochar(p, &c);
	}
	*p = 0;
	js_pushstring(J, s);

	js_endtry(J);
	free(s);
	return 1;
}

static int Sp_match(js_State *J, int argc)
{
	const char *text;

	text = js_tostring(J, 0);

	if (js_isregexp(J, 1))
		js_copy(J, 1);
	else if (js_isundefined(J, 1))
		js_newregexp(J, "", 0);
	else
		js_newregexp(J, js_tostring(J, 1), 0);

	// TODO: JS_REGEXP_G looping

	return js_RegExp_prototype_exec(J, -1, text);
}

static int Sp_search(js_State *J, int argc)
{
	const char *text;
	regmatch_t m;
	regex_t *prog;
	int flags;

	text = js_tostring(J, 0);

	if (js_isregexp(J, 1))
		js_copy(J, 1);
	else if (js_isundefined(J, 1))
		js_newregexp(J, "", 0);
	else
		js_newregexp(J, js_tostring(J, 1), 0);

	prog = js_toregexp(J, -1, &flags);

	if (!regexec(prog, text, 1, &m, 0))
		js_pushnumber(J, m.rm_so); // TODO: convert to utf-8 index offset
	else
		js_pushnumber(J, -1);

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
		jsB_propf(J, "concat", Sp_concat, 1);
		jsB_propf(J, "indexOf", Sp_indexOf, 1);
		jsB_propf(J, "lastIndexOf", Sp_lastIndexOf, 1);
		jsB_propf(J, "localeCompare", Sp_localeCompare, 1);
		jsB_propf(J, "match", Sp_match, 1);
		// replace (uses regexp)
		jsB_propf(J, "search", Sp_search, 1);
		jsB_propf(J, "slice", Sp_slice, 2);
		// split (uses regexp)
		jsB_propf(J, "substring", Sp_substring, 2);
		jsB_propf(J, "toLowerCase", Sp_toLowerCase, 0);
		jsB_propf(J, "toLocaleLowerCase", Sp_toLowerCase, 0);
		jsB_propf(J, "toUpperCase", Sp_toUpperCase, 0);
		jsB_propf(J, "toLocaleUpperCase", Sp_toUpperCase, 0);
		// trim (ES5)
	}
	js_newcconstructor(J, jsB_String, jsB_new_String, 1);
	{
		jsB_propf(J, "fromCharCode", S_fromCharCode, 1);
	}
	js_defglobal(J, "String", JS_DONTENUM);
}
