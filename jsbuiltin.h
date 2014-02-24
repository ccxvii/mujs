#ifndef js_builtin_h
#define js_builtin_h

void jsB_init(js_State *J);
void jsB_initobject(js_State *J);
void jsB_initarray(js_State *J);
void jsB_initfunction(js_State *J);
void jsB_initboolean(js_State *J);
void jsB_initnumber(js_State *J);
void jsB_initstring(js_State *J);
void jsB_initregexp(js_State *J);
void jsB_initerror(js_State *J);
void jsB_initmath(js_State *J);
void jsB_initjson(js_State *J);
void jsB_initdate(js_State *J);

void jsB_propf(js_State *J, const char *name, js_CFunction cfun, int n);
void jsB_propn(js_State *J, const char *name, double number);
void jsB_props(js_State *J, const char *name, const char *string);

typedef struct js_Buffer { unsigned int n, m; char s[64]; } js_Buffer;

static void sb_putc(js_Buffer **sbp, int c)
{
	js_Buffer *sb = *sbp;
	if (!sb) {
		sb = malloc(sizeof *sb);
		sb->n = 0;
		sb->m = sizeof sb->s;
		*sbp = sb;
	} else if (sb->n == sb->m) {
		sb = realloc(sb, (sb->m *= 2) + offsetof(js_Buffer, s));
		*sbp = sb;
	}
	sb->s[sb->n++] = c;
}

static inline void sb_puts(js_Buffer **sb, const char *s)
{
	while (*s)
		sb_putc(sb, *s++);
}

static inline void sb_putm(js_Buffer **sb, const char *s, const char *e)
{
	while (s < e)
		sb_putc(sb, *s++);
}

#endif
