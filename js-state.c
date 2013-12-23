#include "js.h"

js_State *js_newstate(void)
{
	js_State *J = malloc(sizeof *J);
	memset(J, 0, sizeof(*J));
	return J;
}

void js_close(js_State *J)
{
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
