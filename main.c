#include "js.h"

int
main(int argc, char **argv)
{
	js_State *J;
	int i;

	J = js_newstate();

	for (i = 1; i < argc; ++i) {
		js_dofile(J, argv[i]);
		js_gc(J, 1);
	}

	js_freestate(J);

	return 0;
}
