#include "js.h"

int js_loadstring(js_State *J, const char *source)
{
	js_Token t;

	do {
		t = js_lex(J, &source);

		if (t == JS_NUMBER)
			printf("%g\n", J->yynumber);
		else if (t == JS_IDENTIFIER)
			printf("id:%s\n", J->yytext);
		else if (t == JS_STRING)
			printf("'%s'\n", J->yytext);
		else if (t == JS_REGEXP)
			printf("/%s/\n", J->yytext);
		else
			printf("%s\n", js_tokentostring(t));
	} while (t != JS_EOF && t != JS_ERROR);

	return 0;
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

	t = js_loadstring(J, s);

	free(s);
	fclose(f);
	return t;
}
