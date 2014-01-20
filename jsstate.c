#include "jsi.h"
#include "jsparse.h"
#include "jscompile.h"
#include "jsvalue.h"
#include "jsrun.h"
#include "jsbuiltin.h"

void js_loadstring(js_State *J, const char *filename, const char *source)
{
	js_Ast *P;
	js_Function *F;

	if (js_try(J)) {
		jsP_freeparse(J);
		js_throw(J);
	}

	P = jsP_parse(J, filename, source);
	jsP_optimize(J, P);
	F = jsC_compile(J, P);
	jsP_freeparse(J);
	js_newscript(J, F);

	js_endtry(J);
}

void js_loadfile(js_State *J, const char *filename)
{
	FILE *f;
	char *s;
	int n, t;

	f = fopen(filename, "r");
	if (!f) {
		js_error(J, "cannot open file: '%s'", filename);
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fclose(f);
		js_error(J, "cannot seek in file: '%s'", filename);
	}
	n = ftell(f);
	fseek(f, 0, SEEK_SET);

	s = malloc(n + 1); /* add space for string terminator */
	if (!s) {
		fclose(f);
		js_error(J, "cannot allocate storage for file contents: '%s'", filename);
	}

	t = fread(s, 1, n, f);
	if (t != n) {
		free(s);
		fclose(f);
		js_error(J, "cannot read data from file: '%s'", filename);
	}

	s[n] = 0; /* zero-terminate string containing file data */

	if (js_try(J)) {
		free(s);
		fclose(f);
		js_throw(J);
	}

	js_loadstring(J, filename, s);

	free(s);
	fclose(f);
	js_endtry(J);
}

int js_dostring(js_State *J, const char *source)
{
	if (js_try(J)) {
		fprintf(stderr, "libjs: %s\n", js_tostring(J, -1));
		return 1;
	}
	js_loadstring(J, "(string)", source);
	js_pushglobal(J);
	js_call(J, 0);
	js_pop(J, 1);
	js_endtry(J);
	return 0;
}

int js_dofile(js_State *J, const char *filename)
{
	if (js_try(J)) {
		fprintf(stderr, "libjs: %s\n", js_tostring(J, -1));
		return 1;
	}
	js_loadfile(J, filename);
	js_pushglobal(J);
	js_call(J, 0);
	js_pop(J, 1);
	js_endtry(J);
	return 0;
}

js_State *js_newstate(void)
{
	js_State *J = malloc(sizeof *J);
	memset(J, 0, sizeof(*J));

	J->stack = malloc(JS_STACKSIZE * sizeof *J->stack);

	J->gcmark = 1;

	J->G = jsV_newobject(J, JS_COBJECT, NULL);
	J->E = jsR_newenvironment(J, J->G, NULL);

	jsB_init(J);

	return J;
}
