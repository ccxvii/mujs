#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

static int jsB_print(js_State *J, int argc)
{
	int i;
	for (i = 1; i < argc; ++i) {
		const char *s = js_tostring(J, i);
		if (i > 1) putchar(' ');
		fputs(s, stdout);
	}
	putchar('\n');
	return 0;
}

js_State *js_newstate(void)
{
	js_State *J = malloc(sizeof *J);
	memset(J, 0, sizeof(*J));

	J->G = jsR_newobject(J, JS_COBJECT);
	J->GE = jsR_newenvironment(J, J->G, NULL);

	js_pushcfunction(J, jsB_print);
	js_setglobal(J, "print");

	return J;
}

void js_close(js_State *J)
{
	free(J->buf.text);
	free(J);
}

int js_error(js_State *J, const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "error: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");

	return 0;
}
