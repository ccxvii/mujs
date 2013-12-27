#include "js.h"

#define nelem(a) (sizeof (a) / sizeof (a)[0])

struct {
	const char *string;
	js_Token token;
} keywords[] = {
	{"abstract", JS_ABSTRACT},
	{"boolean", JS_BOOLEAN},
	{"break", JS_BREAK},
	{"byte", JS_BYTE},
	{"case", JS_CASE},
	{"catch", JS_CATCH},
	{"char", JS_CHAR},
	{"class", JS_CLASS},
	{"const", JS_CONST},
	{"continue", JS_CONTINUE},
	{"debugger", JS_DEBUGGER},
	{"default", JS_DEFAULT},
	{"delete", JS_DELETE},
	{"do", JS_DO},
	{"double", JS_DOUBLE},
	{"else", JS_ELSE},
	{"enum", JS_ENUM},
	{"export", JS_EXPORT},
	{"extends", JS_EXTENDS},
	{"false", JS_FALSE},
	{"final", JS_FINAL},
	{"finally", JS_FINALLY},
	{"float", JS_FLOAT},
	{"for", JS_FOR},
	{"function", JS_FUNCTION},
	{"goto", JS_GOTO},
	{"if", JS_IF},
	{"implements", JS_IMPLEMENTS},
	{"import", JS_IMPORT},
	{"in", JS_IN},
	{"instanceof", JS_INSTANCEOF},
	{"int", JS_INT},
	{"interface", JS_INTERFACE},
	{"long", JS_LONG},
	{"native", JS_NATIVE},
	{"new", JS_NEW},
	{"null", JS_NULL},
	{"package", JS_PACKAGE},
	{"private", JS_PRIVATE},
	{"protected", JS_PROTECTED},
	{"public", JS_PUBLIC},
	{"return", JS_RETURN},
	{"short", JS_SHORT},
	{"static", JS_STATIC},
	{"super", JS_SUPER},
	{"switch", JS_SWITCH},
	{"synchronized", JS_SYNCHRONIZED},
	{"this", JS_THIS},
	{"throw", JS_THROW},
	{"throws", JS_THROWS},
	{"transient", JS_TRANSIENT},
	{"true", JS_TRUE},
	{"try", JS_TRY},
	{"typeof", JS_TYPEOF},
	{"var", JS_VAR},
	{"void", JS_VOID},
	{"volatile", JS_VOLATILE},
	{"while", JS_WHILE},
	{"with", JS_WITH},
};

static inline js_Token findkeyword(const char *s)
{
	int m, l, r;
	int c;

	l = 0;
	r = nelem(keywords) - 1;

	while (l <= r) {
		m = (l + r) >> 1;
		c = strcmp(s, keywords[m].string);
		if (c < 0)
			r = m - 1;
		else if (c > 0)
			l = m + 1;
		else
			return keywords[m].token;
	}

	return JS_IDENTIFIER;
}

const char *tokenstrings[] = {
	"ERROR", "EOF", "(identifier)", "null", "true", "false", "(number)",
	"(string)", "(regexp)", "\\n", "{", "}", "(", ")", "[", "]", ".", ";",
	",", "<", ">", "<=", ">=", "==", "!=", "===", "!==", "+", "-", "*",
	"%", "++", "--", "<<", ">>", ">>>", "&", "|", "^", "!", "~", "&&",
	"||", "?", ":", "=", "+=", "-=", "*=", "%=", "<<=", ">>=", ">>>=",
	"&=", "|=", "^=", "/", "/=", "break", "case", "catch", "continue",
	"default", "delete", "do", "else", "finally", "for", "function", "if",
	"in", "instanceof", "new", "return", "switch", "this", "throw", "try",
	"typeof", "var", "void", "while", "with", "abstract", "boolean",
	"byte", "char", "class", "const", "debugger", "double", "enum",
	"export", "extends", "final", "float", "goto", "implements", "import",
	"int", "interface", "long", "native", "package", "private",
	"protected", "public", "short", "static", "super", "synchronized",
	"throws", "transient", "volatile",
};

const char *js_tokentostring(js_Token t)
{
	return tokenstrings[t];
}

#define GET() (*(*sp)++)
#define UNGET() ((*sp)--)
#define PEEK() (**sp)
#define NEXT() ((*sp)++)
#define NEXTPEEK() (NEXT(), PEEK())
#define LOOK(x) (PEEK() == x ? (NEXT(), 1) : 0)

js_Token js_syntaxerror(js_State *J, const char *fmt, ...)
{
	va_list ap;
	fprintf(stderr, "syntax error: line %d: ", J->yyline);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	return JS_ERROR;
}

static void textinit(js_State *J)
{
	if (!J->yytext) {
		J->yycap = 4096;
		J->yytext = malloc(J->yycap);
	}
	J->yylen = 0;
}

static inline void textpush(js_State *J, int c)
{
	if (J->yylen >= J->yycap) {
		J->yycap = J->yycap * 2;
		J->yytext = realloc(J->yytext, J->yycap);
	}
	J->yytext[J->yylen++] = c;
}

static inline void textend(js_State *J)
{
	textpush(J, 0);
}

static inline int iswhite(int c)
{
	return c == 0x9 || c == 0xb || c == 0xc || c == 0x20 || c == 0xa0;
}

static inline int isnewline(c)
{
	return c == 0xa || c == 0xd || c == 0x2028 || c == 0x2029;
}

static inline int isidentifierstart(int c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '_';
}

static inline int isidentifierpart(int c)
{
	return (c >= '0' && c <= '9') || isidentifierstart(c);
}

static inline int isdec(int c)
{
	return (c >= '0' && c <= '9');
}

static inline int ishex(int c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int tohex(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 0xa;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 0xa;
	return 0;
}

static inline void lexlinecomment(const char **sp)
{
	int c = PEEK();
	while (c && !isnewline(c)) {
		c = NEXTPEEK();
	}
}

static inline int lexcomment(const char **sp)
{
	while (1) {
		int c = GET();
		if (c == '*') {
			while (c == '*')
				c = GET();
			if (c == '/')
				return 0;
		} else if (c == 0) {
			return -1;
		}
	}
}

static inline double lexhex(const char **sp)
{
	double n = 0;
	int c = PEEK();
	while (ishex(c)) {
		n = n * 16 + tohex(c);
		c = NEXTPEEK();
	}
	return n;
}

static inline double lexinteger(const char **sp)
{
	double n = 0;
	int c = PEEK();
	while (isdec(c)) {
		n = n * 10 + (c - '0');
		c = NEXTPEEK();
	}
	return n;
}

static inline double lexfraction(const char **sp)
{
	double n = 0;
	double d = 1;
	int c = PEEK();
	while (isdec(c)) {
		n = n * 10 + (c - '0');
		d = d * 10;
		c = NEXTPEEK();
	}
	return n / d;
}

static inline double lexexponent(const char **sp)
{
	if (LOOK('e') || LOOK('E')) {
		if (LOOK('-'))
			return -lexinteger(sp);
		else if (LOOK('+'))
			return lexinteger(sp);
		else
			return lexinteger(sp);
	}
	return 0;
}

static inline js_Token lexnumber(js_State *J, const char **sp)
{
	double n;

	if ((*sp)[0] == '0' && ((*sp)[1] == 'x' || (*sp)[1] == 'X')) {
		*sp += 2;
		if (!ishex(PEEK()))
			return js_syntaxerror(J, "0x not followed by hexademical digit");
		J->yynumber = lexhex(sp);
		return JS_NUMBER;
	}

	if ((*sp)[0] == '0' && isdec((*sp)[1]))
		return js_syntaxerror(J, "number with leading zero");

	n = lexinteger(sp);
	if (LOOK('.'))
		n += lexfraction(sp);
	n *= pow(10, lexexponent(sp));

	if (isidentifierstart(PEEK()))
		return js_syntaxerror(J, "number with letter suffix");

	J->yynumber = n;
	return JS_NUMBER;
}

static inline int lexescape(const char **sp)
{
	int c = GET();
	int x, y, z, w;

	switch (c) {
	case '0': return 0;
	case 'u':
		x = tohex(GET());
		y = tohex(GET());
		z = tohex(GET());
		w = tohex(GET());
		return (x << 12) | (y << 8) | (z << 4) | w;
	case 'x':
		x = tohex(GET());
		y = tohex(GET());
		return (x << 4) | y;
	case '\'': return '\'';
	case '"': return '"';
	case '\\': return '\\';
	case 'b': return '\b';
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'v': return '\v';
	default: return c;
	}
}

static inline js_Token lexstring(js_State *J, const char **sp, int q)
{
	int c = GET();

	textinit(J);

	while (c != q) {
		if (c == 0 || isnewline(c))
			return js_syntaxerror(J, "string not terminated");

		if (c == '\\')
			c = lexescape(sp);

		textpush(J, c);

		c = GET();
	}

	textend(J);

	return JS_STRING;
}

/* the ugliest language wart ever... */
static int isregexpcontext(js_Token last)
{
	switch (last)
	{
	case JS_IDENTIFIER:
	case JS_NULL:
	case JS_TRUE:
	case JS_FALSE:
	case JS_THIS:
	case JS_NUMBER:
	case JS_STRING:
	case JS_RSQUARE:
	case JS_RPAREN:
		return 0;
	default:
		return 1;
	}
}

static js_Token lexregexp(js_State *J, const char **sp)
{
	int c;

	textinit(J);

	/* regexp body */
	c = GET();
	while (c != '/') {
		if (c == 0 || isnewline(c)) {
			return js_syntaxerror(J, "regular expression not terminated");
		} else if (c == '\\') {
			textpush(J, c);
			c = GET();
			if (c == 0 || isnewline(c))
				return js_syntaxerror(J, "regular expression not terminated");
			textpush(J, c);
			c = GET();
		} else {
			textpush(J, c);
			c = GET();
		}
	}

	textend(J);

	/* regexp flags */
	J->yyflags.g = J->yyflags.i = J->yyflags.m = 0;

	c = PEEK();
	while (isidentifierpart(c)) {
		if (c == 'g') J->yyflags.g ++;
		else if (c == 'i') J->yyflags.i ++;
		else if (c == 'm') J->yyflags.m ++;
		else return js_syntaxerror(J, "illegal flag in regular expression: %c", c);
		c = NEXTPEEK();
	}

	if (J->yyflags.g > 1 || J->yyflags.i > 1 || J->yyflags.m > 1)
		return js_syntaxerror(J, "duplicated flag in regular expression");

	return JS_REGEXP;
}

static js_Token js_leximp(js_State *J, const char **sp)
{
	int c = GET();
	while (c) {
		while (iswhite(c))
			c = GET();

		if (isnewline(c)) {
			/* consume CR LF as one unit */
			if (c == '\r' && PEEK() == '\n')
				NEXT();
			J->yyline++;
			return JS_NEWLINE;
		}

		if (c == '/') {
			if (LOOK('/')) {
				lexlinecomment(sp);
			} else if (LOOK('*')) {
				if (lexcomment(sp))
					return js_syntaxerror(J, "multi-line comment not terminated");
			} else if (isregexpcontext(J->lasttoken)) {
				return lexregexp(J, sp);
			} else if (LOOK('=')) {
				return JS_SLASH_EQ;
			} else {
				return JS_SLASH;
			}
		}

		if (isidentifierstart(c)) {
			textinit(J);
			textpush(J, c);

			c = PEEK();
			while (isidentifierpart(c)) {
				textpush(J, c);
				c = NEXTPEEK();
			}

			textend(J);

			return findkeyword(J->yytext);
		}

		if (c == '.') {
			if (isdec(PEEK())) {
				UNGET();
				return lexnumber(J, sp);
			}
			return JS_PERIOD;
		}

		if (c >= '0' && c <= '9') {
			UNGET();
			return lexnumber(J, sp);
		}

		if (c == '\'' || c == '"')
			return lexstring(J, sp, c);

		switch (c) {
		case '{': return JS_LCURLY;
		case '}': return JS_RCURLY;
		case '(': return JS_LPAREN;
		case ')': return JS_RPAREN;
		case '[': return JS_LSQUARE;
		case ']': return JS_RSQUARE;
		case '.': return JS_PERIOD;
		case ';': return JS_SEMICOLON;
		case ',': return JS_COMMA;

		case '<':
			if (LOOK('<')) {
				if (LOOK('='))
					return JS_LT_LT_EQ;
				return JS_LT_LT;
			}
			if (LOOK('='))
				return JS_LT_EQ;
			return JS_LT;

		case '>':
			if (LOOK('>')) {
				if (LOOK('>')) {
					if (LOOK('='))
						return JS_GT_GT_GT_EQ;
					return JS_GT_GT_GT;
				}
				if (LOOK('='))
					return JS_GT_GT_EQ;
				return JS_GT_GT;
			}
			if (LOOK('='))
				return JS_GT_EQ;
			return JS_GT;

		case '=':
			if (LOOK('=')) {
				if (LOOK('='))
					return JS_EQ_EQ_EQ;
				return JS_EQ_EQ;
			}
			return JS_EQ;

		case '!':
			if (LOOK('=')) {
				if (LOOK('='))
					return JS_EXCL_EQ_EQ;
				return JS_EXCL_EQ;
			}
			return JS_EXCL;

		case '+':
			if (LOOK('+'))
				return JS_PLUS_PLUS;
			if (LOOK('='))
				return JS_PLUS_EQ;
			return JS_PLUS;

		case '-':
			if (LOOK('-'))
				return JS_MINUS_MINUS;
			if (LOOK('='))
				return JS_MINUS_EQ;
			return JS_MINUS;

		case '*':
			if (LOOK('='))
				return JS_STAR_EQ;
			return JS_STAR;

		case '%':
			if (LOOK('='))
				return JS_PERCENT_EQ;
			return JS_PERCENT;

		case '&':
			if (LOOK('&'))
				return JS_AND_AND;
			if (LOOK('='))
				return JS_AND_EQ;
			return JS_AND;

		case '|':
			if (LOOK('|'))
				return JS_BAR_BAR;
			if (LOOK('='))
				return JS_BAR_EQ;
			return JS_BAR;

		case '^':
			if (LOOK('='))
				return JS_HAT_EQ;
			return JS_HAT;

		case '~': return JS_TILDE;
		case '?': return JS_QUESTION;
		case ':': return JS_COLON;
		}

		c = GET();
	}

	return JS_EOF;
}

js_Token js_lex(js_State *J, const char **sp)
{
	js_Token t = js_leximp(J, sp);
	J->lasttoken = t;
	return t;
}

void js_initlex(js_State *J)
{
	J->yyline = 1;
	J->lasttoken = JS_ERROR;
}
