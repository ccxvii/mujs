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
void jsB_initdate(js_State *J);

void jsB_propf(js_State *J, const char *name, js_CFunction cfun, int n);
void jsB_propn(js_State *J, const char *name, double number);
void jsB_props(js_State *J, const char *name, const char *string);

struct sbuffer { int n, m; char s[64]; };

static struct sbuffer *sb_putc(struct sbuffer *sb, int c)
{
	if (!sb) {
		sb = malloc(sizeof *sb);
		sb->n = 0;
		sb->m = sizeof sb->s;
	} else if (sb->n == sb->m) {
		sb = realloc(sb, (sb->m *= 2) + offsetof(struct sbuffer, s));
	}
	sb->s[sb->n++] = c;
	return sb;
}

static inline struct sbuffer *sb_puts(struct sbuffer *sb, const char *s)
{
	while (*s)
		sb = sb_putc(sb, *s++);
	return sb;
}

static inline struct sbuffer *sb_putm(struct sbuffer *sb, const char *s, const char *e)
{
	while (s < e)
		sb = sb_putc(sb, *s++);
	return sb;
}

#endif
