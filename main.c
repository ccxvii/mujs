#include <stdio.h>

#include "js.h"

#define PS1 "> "

static int jsB_gc(js_State *J, int argc)
{
	int report = js_toboolean(J, 1);
	js_gc(J, report);
	return 0;
}

static int jsB_load(js_State *J, int argc)
{
	const char *filename = js_tostring(J, 1);
	int rv = js_dofile(J, filename);
	js_pushboolean(J, !rv);
	return 1;
}

static int jsB_print(js_State *J, int argc)
{
	int i;
	for (i = 1; i <= argc; ++i) {
		const char *s = js_tostring(J, i);
		if (i > 1) putchar(' ');
		fputs(s, stdout);
	}
	putchar('\n');
	return 0;
}

int
main(int argc, char **argv)
{
	char line[256];
	js_State *J;
	int i;

	J = js_newstate();

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
