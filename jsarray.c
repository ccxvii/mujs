#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

static int jsB_new_Array(js_State *J, int argc)
{
	int i;

	js_newarray(J);

	if (argc == 1) {
		if (js_isnumber(J, 1)) {
			js_copy(J, 1);
			js_setproperty(J, -2, "length");
		} else {
			js_copy(J, 1);
			js_setproperty(J, -2, "0");
		}
	} else {
		char buf[32];
		for (i = 1; i <= argc; ++i) {
			sprintf(buf, "%u", i - 1);
			js_copy(J, i);
			js_setproperty(J, -2, buf);
		}
	}
	return 1;
}

static int Ap_concat(js_State *J, int argc)
{
	unsigned int n, len, i;
	char buf[32];

	js_newarray(J);
	n = 0;

	for (i = 0; i <= argc; i++) {
		js_copy(J, i);
		if (js_isarray(J, -1)) {
			unsigned int k = 0;

			js_getproperty(J, -1, "length");
			len = js_touint32(J, -1);
			js_pop(J, 1);

			while (k < len) {
				sprintf(buf, "%u", k);
				if (js_hasproperty(J, -1, buf)) {
					js_getproperty(J, -1, buf);
					sprintf(buf, "%u", n);
					js_setproperty(J, -3, buf);
				}
				++k;
				++n;
			}
			js_pop(J, 1);
		} else {
			sprintf(buf, "%u", n);
			js_setproperty(J, -2, buf);
			++n;
		}
	}

	return 1;
}

static int Ap_join(js_State *J, int argc)
{
	char * volatile out = NULL;
	char buf[32];
	const char *r, *sep = ",";
	unsigned int seplen, n;
	unsigned int k, len;

	js_getproperty(J, 0, "length");
	len = js_touint32(J, -1);
	js_pop(J, 1);

	if (argc == 1 && !js_isundefined(J, 1)) {
		sep = js_tostring(J, 1);
		seplen = strlen(sep);
	}

	if (len == 0) {
		js_pushliteral(J, "");
		return 1;
	}

	if (js_try(J)) {
		free(out);
		js_throw(J);
	}

	n = 1;
	for (k = 0; k < len; ++k) {
		sprintf(buf, "%u", k);
		js_getproperty(J, 0, buf);
		if (js_isundefined(J, -1) || js_isnull(J, -1))
			r = "";
		else
			r = js_tostring(J, -1);
		n += strlen(r);

		if (k == 0) {
			out = malloc(n);
			strcpy(out, r);
		} else {
			n += seplen;
			out = realloc(out, n);
			strcat(out, sep);
			strcat(out, r);
		}

		js_pop(J, 1);
	}

	js_pushstring(J, out);

	js_endtry(J);
	free(out);
	return 1;
}


static int Ap_pop(js_State *J, int argc)
{
	unsigned int n;
	char buf[32];

	js_getproperty(J, 0, "length");
	n = js_touint32(J, -1);
	js_pop(J, 1);

	if (n > 0) {
		sprintf(buf, "%u", n - 1);
		js_getproperty(J, 0, buf);
		js_delproperty(J, 0, buf);
		js_pushnumber(J, n - 1);
		js_setproperty(J, 0, "length");
	} else {
		js_pushnumber(J, 0);
		js_setproperty(J, 0, "length");
		js_pushundefined(J);
	}
	return 1;
}

static int Ap_push(js_State *J, int argc)
{
	unsigned int n;
	int i;
	char buf[32];

	js_getproperty(J, 0, "length");
	n = js_touint32(J, -1);
	js_pop(J, 1);

	for (i = 1; i <= argc; ++i) {
		sprintf(buf, "%u", n);
		js_copy(J, i);
		js_setproperty(J, 0, buf);
		++n;
	}

	js_pushnumber(J, n);
	js_setproperty(J, 0, "length");

	js_pushnumber(J, n);
	return 1;
}

// reverse
// shift
// slice
// sort
// splice
// unshift

static int Ap_toString(js_State *J, int argc)
{
	js_pop(J, argc);
	return Ap_join(J, 0);
}

static int A_isArray(js_State *J, int argc)
{
	if (js_isobject(J, 1)) {
		js_Object *T = js_toobject(J, 1);
		js_pushboolean(J, T->type == JS_CARRAY);
		return 1;
	}
	js_pushboolean(J, 0);
	return 1;
}

void jsB_initarray(js_State *J)
{
	js_pushobject(J, J->Array_prototype);
	{
		jsB_propf(J, "toString", Ap_toString, 0);
		jsB_propf(J, "concat", Ap_concat, 1);
		jsB_propf(J, "join", Ap_join, 1);
		jsB_propf(J, "pop", Ap_pop, 0);
		jsB_propf(J, "push", Ap_push, 1);
	}
	js_newcconstructor(J, jsB_new_Array, jsB_new_Array, 1);
	{
		/* ECMA-262-5 */
		jsB_propf(J, "isArray", A_isArray, 1);
	}
	js_defglobal(J, "Array", JS_DONTENUM);
}
