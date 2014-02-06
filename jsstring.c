#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"
#include "utf.h"

#include <regex.h>

#define nelem(a) (sizeof (a) / sizeof (a)[0])

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

static inline const char *utfidx(const char *s, int i)
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

static int Sp_slice(js_State *J, int argc)
{
	const char *str = js_tostring(J, 0);
	const char *ss, *ee;
	int len = utflen(str);
	int s = js_tointeger(J, 1);
	int e = argc > 1 ? js_tointeger(J, 2) : len;

	s = s < 0 ? s + len : s;
	e = e < 0 ? e + len : e;

	s = s < 0 ? 0 : s > len ? len : s;
	e = e < 0 ? 0 : e > len ? len : e;

	if (s < e) {
		ss = utfidx(str, s);
		ee = utfidx(ss, e - s);
	} else {
		ss = utfidx(str, e);
		ee = utfidx(ss, s - e);
	}

	js_pushlstring(J, ss, ee - ss);
	return 1;
}

static int Sp_substring(js_State *J, int argc)
{
	const char *str = js_tostring(J, 0);
	const char *ss, *ee;
	int len = utflen(str);
	int s = js_tointeger(J, 1);
	int e = argc > 1 ? js_tointeger(J, 2) : len;

	s = s < 0 ? 0 : s > len ? len : s;
	e = e < 0 ? 0 : e > len ? len : e;

	if (s < e) {
		ss = utfidx(str, s);
		ee = utfidx(ss, e - s);
	} else {
		ss = utfidx(str, e);
		ee = utfidx(ss, s - e);
	}

	js_pushlstring(J, ss, ee - ss);
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
	js_Regexp *re;
	regmatch_t m[10];
	const char *text;
	unsigned int len, a, b, c, e;

	text = js_tostring(J, 0);

	if (js_isregexp(J, 1))
		js_copy(J, 1);
	else if (js_isundefined(J, 1))
		js_newregexp(J, "", 0);
	else
		js_newregexp(J, js_tostring(J, 1), 0);

	re = js_toregexp(J, -1);
	if (!(re->flags & JS_REGEXP_G))
		return js_RegExp_prototype_exec(J, re, text);

	re->last = 0;

	js_newarray(J);

	e = strlen(text);
	len = 0;
	a = 0;
	while (a <= e) {
		if (regexec(re->prog, text + a, nelem(m), m, a > 0 ? REG_NOTBOL : 0))
			break;

		b = a + m[0].rm_so;
		c = a + m[0].rm_eo;

		js_pushlstring(J, text + b, c - b);
		js_setindex(J, -2, len++);

		a = c;
		if (c - b == 0)
			++a;
	}

	return 1;
}

static int Sp_search(js_State *J, int argc)
{
	js_Regexp *re;
	regmatch_t m[10];
	const char *text;

	text = js_tostring(J, 0);

	if (js_isregexp(J, 1))
		js_copy(J, 1);
	else if (js_isundefined(J, 1))
		js_newregexp(J, "", 0);
	else
		js_newregexp(J, js_tostring(J, 1), 0);

	re = js_toregexp(J, -1);

	if (!regexec(re->prog, text, nelem(m), m, 0))
		js_pushnumber(J, m[0].rm_so); // TODO: convert to utf-8 index offset
	else
		js_pushnumber(J, -1);

	return 1;
}

static int Sp_replace_regexp(js_State *J, int argc)
{
	js_Regexp *re;
	regmatch_t m[10];
	const char *source, *s, *r;
	struct sbuffer *sb = NULL;
	int n, x;

	source = js_tostring(J, 0);
	re = js_toregexp(J, 1);

	if (regexec(re->prog, source, nelem(m), m, 0)) {
		js_copy(J, 0);
		return 1;
	}

	re->last = 0;

loop:
	s = source + m[0].rm_so;
	n = m[0].rm_eo - m[0].rm_so;

	if (js_iscallable(J, 2)) {
		js_copy(J, 2);
		js_pushglobal(J);
		for (x = 0; m[x].rm_so >= 0; ++x) /* arg 0..x: substring and subexps that matched */
			js_pushlstring(J, source + m[x].rm_so, m[x].rm_eo - m[x].rm_so);
		js_pushnumber(J, s - source); /* arg x+2: offset within search string */
		js_copy(J, 0); /* arg x+3: search string */
		js_call(J, 2 + x);
		r = js_tostring(J, -1);
		sb = sb_putm(sb, source, s);
		sb = sb_puts(sb, r);
		js_pop(J, 1);
	} else {
		r = js_tostring(J, 2);
		sb = sb_putm(sb, source, s);
		while (*r) {
			if (*r == '$') {
				switch (*(++r)) {
				case '$': sb = sb_putc(sb, '$'); break;
				case '`': sb = sb_putm(sb, source, s); break;
				case '\'': sb = sb_puts(sb, s + n); break;
				case '&':
					sb = sb_putm(sb, s, s + n);
					break;
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					x = *r - '0';
					if (m[x].rm_so >= 0) {
						sb = sb_putm(sb, source + m[x].rm_so, source + m[x].rm_eo);
					} else {
						sb = sb_putc(sb, '$');
						sb = sb_putc(sb, '0'+x);
					}
					break;
				default:
					sb = sb_putc(sb, '$');
					sb = sb_putc(sb, *r);
					break;
				}
				++r;
			} else {
				sb = sb_putc(sb, *r++);
			}
		}
	}

	if (re->flags & JS_REGEXP_G) {
		source = source + m[0].rm_eo;
		if (n == 0) {
			if (*source)
				sb = sb_putc(sb, *source++);
			else
				goto end;
		}
		if (!regexec(re->prog, source, nelem(m), m, REG_NOTBOL))
			goto loop;
	}

end:
	sb = sb_puts(sb, s + n);
	sb = sb_putc(sb, 0);

	if (js_try(J)) {
		free(sb);
		js_throw(J);
	}
	js_pushstring(J, sb ? sb->s : "");
	js_endtry(J);
	free(sb);
	return 1;
}

static int Sp_replace_string(js_State *J, int argc)
{
	const char *source, *needle, *s, *r;
	struct sbuffer *sb = NULL;
	int n;

	source = js_tostring(J, 0);
	needle = js_tostring(J, 1);

	s = strstr(source, needle);
	if (!s) {
		js_copy(J, 0);
		return 1;
	}
	n = strlen(needle);

	if (js_iscallable(J, 2)) {
		js_copy(J, 2);
		js_pushglobal(J);
		js_pushlstring(J, s, n); /* arg 1: substring that matched */
		js_pushnumber(J, s - source); /* arg 2: offset within search string */
		js_copy(J, 0); /* arg 3: search string */
		js_call(J, 3);
		r = js_tostring(J, -1);
		sb = sb_putm(sb, source, s);
		sb = sb_puts(sb, r);
		sb = sb_puts(sb, s + n);
		sb = sb_putc(sb, 0);
		js_pop(J, 1);
	} else {
		r = js_tostring(J, 2);
		sb = sb_putm(sb, source, s);
		while (*r) {
			if (*r == '$') {
				switch (*(++r)) {
				case '$': sb = sb_putc(sb, '$'); break;
				case '&': sb = sb_putm(sb, s, s + n); break;
				case '`': sb = sb_putm(sb, source, s); break;
				case '\'': sb = sb_puts(sb, s + n); break;
				default: sb = sb_putc(sb, '$'); sb = sb_putc(sb, *r); break;
				}
				++r;
			} else {
				sb = sb_putc(sb, *r++);
			}
		}
		sb = sb_puts(sb, s + n);
		sb = sb_putc(sb, 0);
	}

	if (js_try(J)) {
		free(sb);
		js_throw(J);
	}
	js_pushstring(J, sb ? sb->s : "");
	js_endtry(J);
	free(sb);
	return 1;
}

static int Sp_replace(js_State *J, int argc)
{
	if (js_isregexp(J, 1))
		return Sp_replace_regexp(J, argc);
	return Sp_replace_string(J, argc);
}

static int Sp_split_regexp(js_State *J, int argc)
{
	js_Regexp *re;
	regmatch_t m[10];
	const char *str;
	unsigned int limit, len, k, e;
	unsigned int p, a, b, c;

	str = js_tostring(J, 0);
	re = js_toregexp(J, 1);
	limit = !js_isundefined(J, 2) ? js_touint32(J, 2) : 1 << 30;

	js_newarray(J);
	len = 0;

	e = strlen(str);

	/* splitting the empty string */
	if (e == 0) {
		if (regexec(re->prog, str, nelem(m), m, 0)) {
			if (len == limit) return 1;
			js_pushliteral(J, "");
			js_setindex(J, -2, 0);
		}
		return 1;
	}

	p = a = 0;
	while (a < e) {
		if (regexec(re->prog, str + a, nelem(m), m, a > 0 ? REG_NOTBOL : 0))
			break; /* no match */

		b = a + m[0].rm_so;
		c = a + m[0].rm_eo;

		/* empty string at end of last match */
		if (b == p) {
			++a;
			continue;
		}

		if (len == limit) return 1;
		js_pushlstring(J, str + p, b - p);
		js_setindex(J, -2, len++);

		for (k = 1; k < nelem(m) && m[k].rm_so >= 0; ++k) {
			if (len == limit) return 1;
			js_pushlstring(J, str + a + m[k].rm_so, m[k].rm_eo - m[k].rm_so);
			js_setindex(J, -2, len++);
		}

		a = p = c;
	}

	if (len == limit) return 1;
	js_pushstring(J, str + p);
	js_setindex(J, -2, len);

	return 1;
}

static int Sp_split_string(js_State *J, int argc)
{
	const char *str = js_tostring(J, 0);
	const char *sep = js_tostring(J, 1);
	unsigned int limit = !js_isundefined(J, 2) ? js_touint32(J, 2) : 1 << 30;
	unsigned int i, n;

	js_newarray(J);

	n = strlen(sep);

	/* empty string */
	if (n == 0) {
		Rune rune;
		for (i = 0; *str && i < limit; ++i) {
			n = chartorune(&rune, str);
			js_pushlstring(J, str, n);
			js_setindex(J, -2, i);
			str += n;
		}
		return 1;
	}

	for (i = 0; str && i < limit; ++i) {
		const char *s = strstr(str, sep);
		if (s) {
			js_pushlstring(J, str, s-str);
			js_setindex(J, -2, i);
			str = s + n;
		} else {
			js_pushstring(J, str);
			js_setindex(J, -2, i);
			str = NULL;
		}
	}

	return 1;
}

static int Sp_split(js_State *J, int argc)
{
	if (js_isundefined(J, 1)) {
		js_newarray(J);
		js_copy(J, 0);
		js_setindex(J, -2, 0);
		return 1;
	}
	if (js_isregexp(J, 1))
		return Sp_split_regexp(J, argc);
	return Sp_split_string(J, argc);
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
		jsB_propf(J, "replace", Sp_replace, 2);
		jsB_propf(J, "search", Sp_search, 1);
		jsB_propf(J, "slice", Sp_slice, 2);
		jsB_propf(J, "split", Sp_split, 2);
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
