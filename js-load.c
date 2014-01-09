#include "js.h"
#include "js-parse.h"

static int jsP_loadstring(js_State *J, const char *source)
{
	int t;

	jsP_initlex(J, source);
	t = jsP_parse(J);
	printf("parse result = %d\n", t);

	return 0;
}


int js_loadstring(js_State *J, const char *source)
{
	J->yyfilename = "(string)";
	return jsP_loadstring(J, source);
}

int js_loadfile(js_State *J, const char *filename)
{
	FILE *f;
	char *s;
	int n, t;

	f = fopen(filename, "r");
	if (!f)
		return js_error(J, "cannot open file: '%s'", filename);

	fseek(f, 0, SEEK_END);
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

	J->yyfilename = filename;
	t = jsP_loadstring(J, s);

	free(s);
	fclose(f);
	return t;
}
