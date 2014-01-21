#include <stdio.h>

#include "js.h"

#define PS1 "> "

int
main(int argc, char **argv)
{
	char line[256];
	js_State *J;
	int i;

	J = js_newstate();

	if (argc > 1) {
		for (i = 1; i < argc; ++i) {
			js_dofile(J, argv[i]);
			js_gc(J, 1);
		}
	} else {
		fputs(PS1, stdout);
		while (fgets(line, sizeof line, stdin)) {
			js_dostring(J, line, 1);
			fputs(PS1, stdout);
		}
		js_gc(J, 1);
	}

	js_freestate(J);

	return 0;
}
