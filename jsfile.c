#include "jsi.h"
#include <stdio.h>
#include <errno.h>


static void jsB_fexists(js_State *J)
{
	const char *filename = js_tostring(J, 1);
	FILE *f;
	
	f = fopen(filename, "r");
	
	if(errno != 0)
	{
		js_pushboolean(J, 0);
	}
	else
	{
		fclose(f);
		js_pushboolean(J, 1);
	}
}

static void jsB_fwrite(js_State *J)
{
	const char *filename = js_tostring(J, 1);
	const char *filecont = js_tostring(J, 2);
	FILE *f;
	
	f = fopen(filename, "w");
	
	if(errno != 0)
	{
		js_error(J, "cannot write file '%s': %s", filename, strerror(errno));
	}
	
	if (f == NULL) {
		fclose(f);
		js_error(J, "cannot write file '%s': %s", filename, strerror(errno));
	}
	
	if (fprintf(f, "%s", filecont) < 0) {
		fclose(f);
		js_error(J, "cannot write data in file '%s': %s", filename, strerror(errno));
	}
	
	fclose(f);
}

static void jsB_fread(js_State *J)
{
	const char *filename = js_tostring(J, 1);
	FILE *f;
	char *s;
	int n, t;

	f = fopen(filename, "rb");
	if (!f) {
		js_error(J, "cannot open file '%s': %s", filename, strerror(errno));
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fclose(f);
		js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno));
	}

	n = ftell(f);
	if (n < 0) {
		fclose(f);
		js_error(J, "cannot tell in file '%s': %s", filename, strerror(errno));
	}

	if (fseek(f, 0, SEEK_SET) < 0) {
		fclose(f);
		js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno));
	}

	s = malloc(n + 1);
	if (!s) {
		fclose(f);
		js_error(J, "out of memory");
	}

	t = fread(s, 1, n, f);
	if (t != n) {
		free(s);
		fclose(f);
		js_error(J, "cannot read data from file '%s': %s", filename, strerror(errno));
	}
	s[n] = 0;

	js_pushstring(J, s);
	free(s);
	fclose(f);
}

void jsB_initfile(js_State *J)
{
	js_pushobject(J, jsV_newobject(J, JS_CFILE, J->Object_prototype));
	{
		jsB_propf(J, "File.exists", jsB_fexists, 1);
		jsB_propf(J, "File.write", jsB_fwrite, 2);
		jsB_propf(J, "File.read", jsB_fread, 1);
	}
	js_defglobal(J, "File", JS_DONTENUM);
}
