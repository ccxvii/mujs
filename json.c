#include "jsi.h"
#include "jslex.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

#include "utf.h"

static inline void jsonnext(js_State *J)
{
	J->lookahead = jsY_lexjson(J);
}

static inline int jsonaccept(js_State *J, int t)
{
	if (J->lookahead == t) {
		jsonnext(J);
		return 1;
	}
	return 0;
}

static inline void jsonexpect(js_State *J, int t)
{
	if (!jsonaccept(J, t))
		js_syntaxerror(J, "JSON: unexpected token: %s (expected %s)",
				jsY_tokenstring(J->lookahead), jsY_tokenstring(t));
}

static void jsonvalue(js_State *J)
{
	int i;
	const char *name;

	switch (J->lookahead) {
	case TK_STRING:
		js_pushliteral(J, J->text);
		jsonnext(J);
		break;

	case TK_NUMBER:
		js_pushnumber(J, J->number);
		jsonnext(J);
		break;

	case '{':
		js_newobject(J);
		jsonnext(J);
		if (J->lookahead == '}')
			return;
		do {
			if (J->lookahead != TK_STRING)
				js_syntaxerror(J, "JSON: unexpected token: %s (expected string)", jsY_tokenstring(J->lookahead));
			name = J->text;
			jsonnext(J);
			jsonexpect(J, ':');
			jsonvalue(J);
			js_setproperty(J, -2, name);
		} while (jsonaccept(J, ','));
		jsonexpect(J, '}');
		break;

	case '[':
		js_newarray(J);
		jsonnext(J);
		i = 0;
		if (J->lookahead == ']')
			return;
		do {
			jsonvalue(J);
			js_setindex(J, -2, i++);
		} while (jsonaccept(J, ','));
		jsonexpect(J, ']');
		break;

	case TK_TRUE:
		js_pushboolean(J, 1);
		jsonnext(J);
		break;

	case TK_FALSE:
		js_pushboolean(J, 0);
		jsonnext(J);
		break;

	case TK_NULL:
		js_pushnull(J);
		jsonnext(J);
		break;

	default:
		js_syntaxerror(J, "JSON: unexpected token: %s", jsY_tokenstring(J->lookahead));
	}
}

static int JSON_parse(js_State *J, unsigned int argc)
{
	const char *source = js_tostring(J, 1);
	jsY_initlex(J, "JSON", source);
	jsonnext(J);
	jsonvalue(J);
	// TODO: reviver Walk()
	return 1;
}

static void fmtnum(js_Buffer **sb, double n)
{
	if (isnan(n)) sb_puts(sb, "NaN");
	else if (isinf(n)) sb_puts(sb, n < 0 ? "-Infinity" : "Infinity");
	else if (n == 0) sb_puts(sb, "0");
	else {
		char buf[40];
		sprintf(buf, "%.17g", n);
		sb_puts(sb, buf);
	}
}

static void fmtstr(js_Buffer **sb, const char *s)
{
	static const char *HEX = "0123456789ABCDEF";
	Rune c;
	sb_putc(sb, '"');
	while (*s) {
		s += chartorune(&c, s);
		switch (c) {
		case '"': sb_puts(sb, "\\\""); break;
		case '\\': sb_puts(sb, "\\\\"); break;
		case '\b': sb_puts(sb, "\\b"); break;
		case '\f': sb_puts(sb, "\\f"); break;
		case '\n': sb_puts(sb, "\\n"); break;
		case '\r': sb_puts(sb, "\\r"); break;
		case '\t': sb_puts(sb, "\\t"); break;
		default:
			if (c < ' ') {
				sb_puts(sb, "\\u");
				sb_putc(sb, HEX[(c>>12)&15]);
				sb_putc(sb, HEX[(c>>8)&15]);
				sb_putc(sb, HEX[(c>>4)&15]);
				sb_putc(sb, HEX[c&15]);
			} else {
				sb_putc(sb, c); break;
			}
		}
	}
	sb_putc(sb, '"');
}

static int fmtvalue(js_State *J, js_Buffer **sb, const char *key);

static void fmtobject(js_State *J, js_Buffer **sb, js_Object *obj)
{
	js_Property *ref;
	int save;
	int n = 0;

	sb_putc(sb, '{');
	for (ref = obj->head; ref; ref = ref->next) {
		if (ref->atts & JS_DONTENUM)
			continue;
		save = (*sb)->n;
		if (n) sb_putc(sb, ',');
		fmtstr(sb, ref->name);
		sb_putc(sb, ':');
		js_pushvalue(J, ref->value);
		if (!fmtvalue(J, sb, ref->name))
			(*sb)->n = save;
		else
			++n;
		js_pop(J, 1);
	}
	sb_putc(sb, '}');
}

static void fmtarray(js_State *J, js_Buffer **sb)
{
	unsigned int len, k;
	char buf[40];

	js_getproperty(J, -1, "length");
	len = js_touint32(J, -1);
	js_pop(J, 1);

	sb_putc(sb, '[');
	for (k = 0; k < len; ++k) {
		if (k) sb_putc(sb, ',');
		sprintf(buf, "%u", k);
		js_getproperty(J, -1, buf);
		if (!fmtvalue(J, sb, buf))
			sb_puts(sb, "null");
		js_pop(J, 1);
	}
	sb_putc(sb, ']');
}

static int fmtvalue(js_State *J, js_Buffer **sb, const char *key)
{
	if (js_try(J)) {
		free(*sb);
		js_throw(J);
	}
	if (js_isobject(J, -1)) {
		if (js_hasproperty(J, -1, "toJSON")) {
			if (js_iscallable(J, -1)) {
				js_copy(J, -2);
				js_pushliteral(J, key);
				js_call(J, 1);
				js_rot2pop1(J);
			} else {
				js_pop(J, 1);
			}
		}
	}
	js_endtry(J);

	// TODO: replacer()

	if (js_isobject(J, -1) && !js_iscallable(J, -1)) {
		js_Object *obj = js_toobject(J, -1);
		switch (obj->type) {
		case JS_CNUMBER: fmtnum(sb, obj->u.number); break;
		case JS_CSTRING: fmtstr(sb, obj->u.s.string); break;
		case JS_CBOOLEAN: sb_puts(sb, obj->u.boolean ? "true" : "false"); break;
		case JS_CARRAY: fmtarray(J, sb); break;
		default: fmtobject(J, sb, obj); break;
		}
	}
	else if (js_isboolean(J, -1))
		sb_puts(sb, js_toboolean(J, -1) ? "true" : "false");
	else if (js_isnumber(J, -1))
		fmtnum(sb, js_tonumber(J, -1));
	else if (js_isstring(J, -1))
		fmtstr(sb, js_tostring(J, -1));
	else if (js_isnull(J, -1))
		sb_puts(sb, "null");
	else
		return 0;

	return 1;
}

static int JSON_stringify(js_State *J, unsigned int argc)
{
	js_Buffer *sb = NULL;
	if (argc > 0) {
		js_copy(J, 1);
		if (fmtvalue(J, &sb, "")) {
			sb_putc(&sb, 0);
			if (js_try(J)) {
				free(sb);
				js_throw(J);
			}
			js_pushstring(J, sb ? sb->s : "");
			js_endtry(J);
			free(sb);
			return 1;
		}
	}
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
