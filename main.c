#include <stdio.h>

#include "mujs.h"

#define PS1 "> "

static void jsB_gc(js_State *J)
{
	int report = js_toboolean(J, 1);
	js_gc(J, report);
	js_pushundefined(J);
}

static void jsB_load(js_State *J)
{
	const char *filename = js_tostring(J, 1);
	int rv = js_dofile(J, filename);
	js_pushboolean(J, !rv);
}

static void jsB_print(js_State *J)
{
	unsigned int i, top = js_gettop(J);
	for (i = 1; i < top; ++i) {
		const char *s = js_tostring(J, i);
		if (i > 1) putchar(' ');
		fputs(s, stdout);
	}
	putchar('\n');
	js_pushundefined(J);
}

int
main(int argc, char **argv)
{
	char line[256];
	js_State *J;
	int i;

	J = js_newstate(NULL, NULL);

	js_newcfunction(J, jsB_gc, 0);
	js_setglobal(J, "gc");

	js_newcfunction(J, jsB_load, 1);
	js_setglobal(J, "load");

	js_newcfunction(J, jsB_print, 1);
	js_setglobal(J, "print");

	if (argc > 1) {
		for (i = 1; i < argc; ++i) {
			if (js_dofile(J, argv[i]))
				return 1;
			js_gc(J, 0);
		}
	} else {
		fputs(PS1, stdout);
		while (fgets(line, sizeof line, stdin)) {
			js_dostring(J, line, 1);
			fputs(PS1, stdout);
		}
		putchar('\n');
		js_gc(J, 1);
	}

	js_freestate(J);

	return 0;
}
