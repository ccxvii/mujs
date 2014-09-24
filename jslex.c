#include "jsi.h"
#include "jslex.h"
#include "utf.h"

JS_NORETURN static void jsY_error(js_State *J, const char *fmt, ...) JS_PRINTFLIKE(2,3);

static void jsY_error(js_State *J, const char *fmt, ...)
{
	va_list ap;
	char buf[512];
	char msgbuf[256];

	va_start(ap, fmt);
	vsnprintf(msgbuf, 256, fmt, ap);
	va_end(ap);

	snprintf(buf, 256, "%s:%d: ", J->filename, J->lexline);
	strcat(buf, msgbuf);

	js_newsyntaxerror(J, buf);
	js_throw(J);
}

static const char *tokenstring[] = {
	"(end-of-file)",
	"'\\x01'", "'\\x02'", "'\\x03'", "'\\x04'", "'\\x05'", "'\\x06'", "'\\x07'",
	"'\\x08'", "'\\x09'", "'\\x0A'", "'\\x0B'", "'\\x0C'", "'\\x0D'", "'\\x0E'", "'\\x0F'",
	"'\\x10'", "'\\x11'", "'\\x12'", "'\\x13'", "'\\x14'", "'\\x15'", "'\\x16'", "'\\x17'",
	"'\\x18'", "'\\x19'", "'\\x1A'", "'\\x1B'", "'\\x1C'", "'\\x1D'", "'\\x1E'", "'\\x1F'",
	"' '", "'!'", "'\"'", "'#'", "'$'", "'%'", "'&'", "'\\''",
	"'('", "')'", "'*'", "'+'", "','", "'-'", "'.'", "'/'",
	"'0'", "'1'", "'2'", "'3'", "'4'", "'5'", "'6'", "'7'",
	"'8'", "'9'", "':'", "';'", "'<'", "'='", "'>'", "'?'",
	"'@'", "'A'", "'B'", "'C'", "'D'", "'E'", "'F'", "'G'",
	"'H'", "'I'", "'J'", "'K'", "'L'", "'M'", "'N'", "'O'",
	"'P'", "'Q'", "'R'", "'S'", "'T'", "'U'", "'V'", "'W'",
	"'X'", "'Y'", "'Z'", "'['", "'\'", "']'", "'^'", "'_'",
	"'`'", "'a'", "'b'", "'c'", "'d'", "'e'", "'f'", "'g'",
	"'h'", "'i'", "'j'", "'k'", "'l'", "'m'", "'n'", "'o'",
	"'p'", "'q'", "'r'", "'s'", "'t'", "'u'", "'v'", "'w'",
	"'x'", "'y'", "'z'", "'{'", "'|'", "'}'", "'~'", "'\\x7F'",

	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

	"(identifier)", "(number)", "(string)", "(regexp)",

	"'<='", "'>='", "'=='", "'!='", "'==='", "'!=='",
	"'<<'", "'>>'", "'>>>'", "'&&'", "'||'",
	"'+='", "'-='", "'*='", "'/='", "'%='",
	"'<<='", "'>>='", "'>>>='", "'&='", "'|='", "'^='",
	"'++'", "'--'",

	"'break'", "'case'", "'catch'", "'continue'", "'debugger'",
	"'default'", "'delete'", "'do'", "'else'", "'false'", "'finally'", "'for'",
	"'function'", "'if'", "'in'", "'instanceof'", "'new'", "'null'", "'return'",
	"'switch'", "'this'", "'throw'", "'true'", "'try'", "'typeof'", "'var'",
	"'void'", "'while'", "'with'",
};

const char *jsY_tokenstring(int token)
{
	if (token >= 0 && token < (int)nelem(tokenstring))
		if (tokenstring[token])
			return tokenstring[token];
	return "<unknown>";
}

static const char *keywords[] = {
	"break", "case", "catch", "continue", "debugger", "default", "delete",
	"do", "else", "false", "finally", "for", "function", "if", "in",
	"instanceof", "new", "null", "return", "switch", "this", "throw",
	"true", "try", "typeof", "var", "void", "while", "with",
};

int jsY_findword(const char *s, const char **list, int num)
{
	int l = 0;
	int r = num - 1;
	while (l <= r) {
		int m = (l + r) >> 1;
		int c = strcmp(s, list[m]);
		if (c < 0)
			r = m - 1;
		else if (c > 0)
			l = m + 1;
		else
			return m;
	}
	return -1;
}

static int jsY_findkeyword(js_State *J, const char *s)
{
	int i = jsY_findword(s, keywords, nelem(keywords));
	if (i >= 0) {
		J->text = keywords[i];
		return TK_BREAK + i; /* first keyword + i */
	}
	J->text = js_intern(J, s);
	return TK_IDENTIFIER;
}

int jsY_iswhite(int c)
{
	return c == 0x9 || c == 0xB || c == 0xC || c == 0x20 || c == 0xA0 || c == 0xFEFF;
}

int jsY_isnewline(int c)
{
	return c == 0xA || c == 0xD || c == 0x2028 || c == 0x2029;
}

static int jsY_isidentifierstart(int c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '_' || isalpharune(c);
}

static int jsY_isidentifierpart(int c)
{
	return (c >= '0' && c <= '9') || jsY_isidentifierstart(c);
}

static int jsY_isdec(int c)
{
	return (c >= '0' && c <= '9');
}

int jsY_ishex(int c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int jsY_tohex(int c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 0xA;
	if (c >= 'A' && c <= 'F') return c - 'A' + 0xA;
	return 0;
}

#define PEEK (J->lexchar)
#define NEXT() jsY_next(J)
#define ACCEPT(x) (PEEK == x ? (NEXT(), 1) : 0)
#define EXPECT(x) if (!ACCEPT(x)) jsY_error(J, "expected '%c'", x)

static void jsY_next(js_State *J)
{
	Rune c;
	J->source += chartorune(&c, J->source);
	/* consume CR LF as one unit */
	if (c == '\r' && *J->source == '\n')
		++J->source;
	if (jsY_isnewline(c)) {
		J->line++;
		c = '\n';
	}
	J->lexchar = c;
}

static void jsY_unescape(js_State *J)
{
	if (ACCEPT('\\')) {
		if (ACCEPT('u')) {
			int x = 0;
			if (!jsY_ishex(PEEK)) goto error; x |= jsY_tohex(PEEK) << 12; NEXT();
			if (!jsY_ishex(PEEK)) goto error; x |= jsY_tohex(PEEK) << 8; NEXT();
			if (!jsY_ishex(PEEK)) goto error; x |= jsY_tohex(PEEK) << 4; NEXT();
			if (!jsY_ishex(PEEK)) goto error; x |= jsY_tohex(PEEK);
			J->lexchar = x;
			return;
		}
error:
		jsY_error(J, "unexpected escape sequence");
	}
}

static void textinit(js_State *J)
{
	if (!J->lexbuf.text) {
		J->lexbuf.cap = 4096;
		J->lexbuf.text = js_malloc(J, J->lexbuf.cap);
	}
	J->lexbuf.len = 0;
}

static void textpush(js_State *J, Rune c)
{
	int n = runelen(c);
	if (J->lexbuf.len + n > J->lexbuf.cap) {
		J->lexbuf.cap = J->lexbuf.cap * 2;
		J->lexbuf.text = js_realloc(J, J->lexbuf.text, J->lexbuf.cap);
	}
	J->lexbuf.len += runetochar(J->lexbuf.text + J->lexbuf.len, &c);
}

static char *textend(js_State *J)
{
	textpush(J, 0);
	return J->lexbuf.text;
}

static void lexlinecomment(js_State *J)
{
	while (PEEK && PEEK != '\n')
		NEXT();
}

static int lexcomment(js_State *J)
{
	/* already consumed initial '/' '*' sequence */
	while (PEEK != 0) {
		if (ACCEPT('*')) {
			while (PEEK == '*')
				NEXT();
			if (ACCEPT('/'))
				return 0;
		}
		NEXT();
	}
	return -1;
}

static double lexhex(js_State *J)
{
	double n = 0;
	if (!jsY_ishex(PEEK))
		jsY_error(J, "malformed hexadecimal number");
	while (jsY_ishex(PEEK)) {
		n = n * 16 + jsY_tohex(PEEK);
		NEXT();
	}
	return n;
}

#if 0

static double lexinteger(js_State *J)
{
	double n = 0;
	if (!jsY_isdec(PEEK))
		jsY_error(J, "malformed number");
	while (jsY_isdec(PEEK)) {
		n = n * 10 + (PEEK - '0');
		NEXT();
	}
	return n;
}

static double lexfraction(js_State *J)
{
	double n = 0;
	double d = 1;
	while (jsY_isdec(PEEK)) {
		n = n * 10 + (PEEK - '0');
		d = d * 10;
		NEXT();
	}
	return n / d;
}

static double lexexponent(js_State *J)
{
	double sign;
	if (ACCEPT('e') || ACCEPT('E')) {
		if (ACCEPT('-')) sign = -1;
		else if (ACCEPT('+')) sign = 1;
		else sign = 1;
		return sign * lexinteger(J);
	}
	return 0;
}

static int lexnumber(js_State *J)
{
	double n;
	double e;

	if (ACCEPT('0')) {
		if (ACCEPT('x') || ACCEPT('X')) {
			J->number = lexhex(J);
			return TK_NUMBER;
		}
		if (jsY_isdec(PEEK))
			jsY_error(J, "number with leading zero");
		n = 0;
		if (ACCEPT('.'))
			n += lexfraction(J);
	} else if (ACCEPT('.')) {
		if (!jsY_isdec(PEEK))
			return '.';
		n = lexfraction(J);
	} else {
		n = lexinteger(J);
		if (ACCEPT('.'))
			n += lexfraction(J);
	}

	e = lexexponent(J);
	if (e < 0)
		n /= pow(10, -e);
	else if (e > 0)
		n *= pow(10, e);

	if (jsY_isidentifierstart(PEEK))
		jsY_error(J, "number with letter suffix");

	J->number = n;
	return TK_NUMBER;
}

#else

static int lexnumber(js_State *J)
{
	const char *s = J->source - 1;

	if (ACCEPT('0')) {
		if (ACCEPT('x') || ACCEPT('X')) {
			J->number = lexhex(J);
			return TK_NUMBER;
		}
		if (jsY_isdec(PEEK))
			jsY_error(J, "number with leading zero");
		if (ACCEPT('.')) {
			while (jsY_isdec(PEEK))
				NEXT();
		}
	} else if (ACCEPT('.')) {
		if (!jsY_isdec(PEEK))
			return '.';
		while (jsY_isdec(PEEK))
			NEXT();
	} else {
		while (jsY_isdec(PEEK))
			NEXT();
		if (ACCEPT('.')) {
			while (jsY_isdec(PEEK))
				NEXT();
		}
	}

	if (ACCEPT('e') || ACCEPT('E')) {
		if (PEEK == '-' || PEEK == '+')
			NEXT();
		while (jsY_isdec(PEEK))
			NEXT();
	}

	if (jsY_isidentifierstart(PEEK))
		jsY_error(J, "number with letter suffix");

	J->number = js_strtod(s, NULL);
	return TK_NUMBER;

}

#endif

static int lexescape(js_State *J)
{
	int x = 0;

	/* already consumed '\' */

	if (ACCEPT('\n'))
		return 0;

	switch (PEEK) {
	case 'u':
		NEXT();
		if (!jsY_ishex(PEEK)) return 1; else { x |= jsY_tohex(PEEK) << 12; NEXT(); }
		if (!jsY_ishex(PEEK)) return 1; else { x |= jsY_tohex(PEEK) << 8; NEXT(); }
		if (!jsY_ishex(PEEK)) return 1; else { x |= jsY_tohex(PEEK) << 4; NEXT(); }
		if (!jsY_ishex(PEEK)) return 1; else { x |= jsY_tohex(PEEK); NEXT(); }
		textpush(J, x);
		break;
	case 'x':
		NEXT();
		if (!jsY_ishex(PEEK)) return 1; else { x |= jsY_tohex(PEEK) << 4; NEXT(); }
		if (!jsY_ishex(PEEK)) return 1; else { x |= jsY_tohex(PEEK); NEXT(); }
		textpush(J, x);
		break;
	case '0': textpush(J, 0); NEXT(); break;
	case '\\': textpush(J, '\\'); NEXT(); break;
	case '\'': textpush(J, '\''); NEXT(); break;
	case '"': textpush(J, '"'); NEXT(); break;
	case 'b': textpush(J, '\b'); NEXT(); break;
	case 'f': textpush(J, '\f'); NEXT(); break;
	case 'n': textpush(J, '\n'); NEXT(); break;
	case 'r': textpush(J, '\r'); NEXT(); break;
	case 't': textpush(J, '\t'); NEXT(); break;
	case 'v': textpush(J, '\v'); NEXT(); break;
	default: textpush(J, PEEK); NEXT(); break;
	}
	return 0;
}

static int lexstring(js_State *J)
{
	const char *s;

	int q = PEEK;
	NEXT();

	textinit(J);

	while (PEEK != q) {
		if (PEEK == 0 || PEEK == '\n')
			jsY_error(J, "string not terminated");
		if (ACCEPT('\\')) {
			if (lexescape(J))
				jsY_error(J, "malformed escape sequence");
		} else {
			textpush(J, PEEK);
			NEXT();
		}
	}
	EXPECT(q);

	s = textend(J);

	J->text = js_intern(J, s);
	return TK_STRING;
}

/* the ugliest language wart ever... */
static int isregexpcontext(int last)
{
	switch (last) {
	case ']':
	case ')':
	case '}':
	case TK_IDENTIFIER:
	case TK_NUMBER:
	case TK_STRING:
	case TK_FALSE:
	case TK_NULL:
	case TK_THIS:
	case TK_TRUE:
		return 0;
	default:
		return 1;
	}
}

static int lexregexp(js_State *J)
{
	const char *s;
	int g, m, i;

	/* already consumed initial '/' */

	textinit(J);

	/* regexp body */
	while (PEEK != '/') {
		if (PEEK == 0 || PEEK == '\n') {
			jsY_error(J, "regular expression not terminated");
		} else if (ACCEPT('\\')) {
			if (ACCEPT('/')) {
				textpush(J, '/');
			} else {
				textpush(J, '\\');
				if (PEEK == 0 || PEEK == '\n')
					jsY_error(J, "regular expression not terminated");
				textpush(J, PEEK);
				NEXT();
			}
		} else {
			textpush(J, PEEK);
			NEXT();
		}
	}
	EXPECT('/');

	s = textend(J);

	/* regexp flags */
	g = i = m = 0;

	while (jsY_isidentifierpart(PEEK)) {
		if (ACCEPT('g')) ++g;
		else if (ACCEPT('i')) ++i;
		else if (ACCEPT('m')) ++m;
		else jsY_error(J, "illegal flag in regular expression: %c", PEEK);
	}

	if (g > 1 || i > 1 || m > 1)
		jsY_error(J, "duplicated flag in regular expression");

	J->text = js_intern(J, s);
	J->number = 0;
	if (g) J->number += JS_REGEXP_G;
	if (i) J->number += JS_REGEXP_I;
	if (m) J->number += JS_REGEXP_M;
	return TK_REGEXP;
}

/* simple "return [no Line Terminator here] ..." contexts */
static int isnlthcontext(int last)
{
	switch (last) {
	case TK_BREAK:
	case TK_CONTINUE:
	case TK_RETURN:
	case TK_THROW:
		return 1;
	default:
		return 0;
	}
}

static int jsY_lexx(js_State *J)
{
	J->newline = 0;

	while (1) {
		J->lexline = J->line; /* save location of beginning of token */

		while (jsY_iswhite(PEEK))
			NEXT();

		if (ACCEPT('\n')) {
			J->newline = 1;
			if (isnlthcontext(J->lasttoken))
				return ';';
			continue;
		}

		if (ACCEPT('/')) {
			if (ACCEPT('/')) {
				lexlinecomment(J);
				continue;
			} else if (ACCEPT('*')) {
				if (lexcomment(J))
					jsY_error(J, "multi-line comment not terminated");
				continue;
			} else if (isregexpcontext(J->lasttoken)) {
				return lexregexp(J);
			} else if (ACCEPT('=')) {
				return TK_DIV_ASS;
			} else {
				return '/';
			}
		}

		if (PEEK >= '0' && PEEK <= '9') {
			return lexnumber(J);
		}

		switch (PEEK) {
		case '(': NEXT(); return '(';
		case ')': NEXT(); return ')';
		case ',': NEXT(); return ',';
		case ':': NEXT(); return ':';
		case ';': NEXT(); return ';';
		case '?': NEXT(); return '?';
		case '[': NEXT(); return '[';
		case ']': NEXT(); return ']';
		case '{': NEXT(); return '{';
		case '}': NEXT(); return '}';
		case '~': NEXT(); return '~';

		case '\'':
		case '"':
			return lexstring(J);

		case '.':
			return lexnumber(J);

		case '<':
			NEXT();
			if (ACCEPT('<')) {
				if (ACCEPT('='))
					return TK_SHL_ASS;
				return TK_SHL;
			}
			if (ACCEPT('='))
				return TK_LE;
			return '<';

		case '>':
			NEXT();
			if (ACCEPT('>')) {
				if (ACCEPT('>')) {
					if (ACCEPT('='))
						return TK_USHR_ASS;
					return TK_USHR;
				}
				if (ACCEPT('='))
					return TK_SHR_ASS;
				return TK_SHR;
			}
			if (ACCEPT('='))
				return TK_GE;
			return '>';

		case '=':
			NEXT();
			if (ACCEPT('=')) {
				if (ACCEPT('='))
					return TK_STRICTEQ;
				return TK_EQ;
			}
			return '=';

		case '!':
			NEXT();
			if (ACCEPT('=')) {
				if (ACCEPT('='))
					return TK_STRICTNE;
				return TK_NE;
			}
			return '!';

		case '+':
			NEXT();
			if (ACCEPT('+'))
				return TK_INC;
			if (ACCEPT('='))
				return TK_ADD_ASS;
			return '+';

		case '-':
			NEXT();
			if (ACCEPT('-'))
				return TK_DEC;
			if (ACCEPT('='))
				return TK_SUB_ASS;
			return '-';

		case '*':
			NEXT();
			if (ACCEPT('='))
				return TK_MUL_ASS;
			return '*';

		case '%':
			NEXT();
			if (ACCEPT('='))
				return TK_MOD_ASS;
			return '%';

		case '&':
			NEXT();
			if (ACCEPT('&'))
				return TK_AND;
			if (ACCEPT('='))
				return TK_AND_ASS;
			return '&';

		case '|':
			NEXT();
			if (ACCEPT('|'))
				return TK_OR;
			if (ACCEPT('='))
				return TK_OR_ASS;
			return '|';

		case '^':
			NEXT();
			if (ACCEPT('='))
				return TK_XOR_ASS;
			return '^';

		case 0:
			return 0; /* EOF */
		}

		/* Handle \uXXXX escapes in identifiers */
		jsY_unescape(J);
		if (jsY_isidentifierstart(PEEK)) {
			textinit(J);
			textpush(J, PEEK);

			NEXT();
			jsY_unescape(J);
			while (jsY_isidentifierpart(PEEK)) {
				textpush(J, PEEK);
				NEXT();
				jsY_unescape(J);
			}

			textend(J);

			return jsY_findkeyword(J, J->lexbuf.text);
		}

		if (PEEK >= 0x20 && PEEK <= 0x7E)
			jsY_error(J, "unexpected character: '%c'", PEEK);
		jsY_error(J, "unexpected character: \\u%04X", PEEK);
	}
}

void jsY_initlex(js_State *J, const char *filename, const char *source)
{
	J->filename = filename;
	J->source = source;
	J->line = 1;
	J->lasttoken = 0;
	NEXT(); /* load first lookahead character */
}

int jsY_lex(js_State *J)
{
	return J->lasttoken = jsY_lexx(J);
}

int jsY_lexjson(js_State *J)
{
	while (1) {
		J->lexline = J->line; /* save location of beginning of token */

		while (jsY_iswhite(PEEK) || PEEK == '\n')
			NEXT();

		if (PEEK >= '0' && PEEK <= '9') {
			return lexnumber(J);
		}

		switch (PEEK) {
		case ',': NEXT(); return ',';
		case ':': NEXT(); return ':';
		case '[': NEXT(); return '[';
		case ']': NEXT(); return ']';
		case '{': NEXT(); return '{';
		case '}': NEXT(); return '}';

		case '\'':
		case '"':
			return lexstring(J);

		case '.':
			return lexnumber(J);

		case 0:
			return 0; /* EOF */
		}

		if (PEEK >= 0x20 && PEEK <= 0x7E)
			jsY_error(J, "unexpected character: '%c'", PEEK);
		jsY_error(J, "unexpected character: \\u%04X", PEEK);
	}
}
