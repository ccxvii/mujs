#include "js.h"
#include "jsparse.h"

static int jsP_loadstring(js_State *J, const char *filename, const char *source)
{
	js_Ast *prog = jsP_parse(J, filename, source);
	if (prog) {
		jsP_optimize(J, prog);
//		jsP_dumpsyntax(J, prog);
		jsP_dumplist(J, prog);
		jsP_freeparse(J);
		return 0;
	}
	return 1;
}

int js_loadstring(js_State *J, const char *source)
{
	return jsP_loadstring(J, "(string)", source);
}

int js_loadfile(js_State *J, const char *filename)
{
	FILE *f;
	char *s;
	int n, t;

	f = fopen(filename, "r");
	if (!f) {
		return js_error(J, "cannot open file: '%s'", filename);
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fclose(f);
		return js_error(J, "cannot seek in file: '%s'", filename);
	}
	n = ftell(f);
	fseek(f, 0, SEEK_SET);

	s = malloc(n + 1); /* add space for string terminator */
	if (!s) {
		fclose(f);
		return js_error(J, "cannot allocate storage for file contents: '%s'", filename);
	}

	t = fread(s, 1, n, f);
	if (t != n) {
		free(s);
		fclose(f);
		return js_error(J, "cannot read data from file: '%s'", filename);
	}

	s[n] = 0; /* zero-terminate string containing file data */

	t = jsP_loadstring(J, filename, s);

	free(s);
	fclose(f);
	return t;
}
