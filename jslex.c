#include "js.h"
#include "jslex.h"
#include "jsstate.h"

#define nelem(a) (sizeof (a) / sizeof (a)[0])

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

const char *jsP_tokenstring(int token)
{
	if (token >= 0 && token < nelem(tokenstring))
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

static inline int findword(const char *s, const char **list, int num)
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

static inline int findkeyword(js_State *J, const char *s)
{
	int i = findword(s, keywords, nelem(keywords));
	if (i >= 0) {
		J->text = keywords[i];
		return TK_BREAK + i; /* first keyword + i */
	}
	J->text = js_intern(J, s);
	return TK_IDENTIFIER;
}

static inline int iswhite(int c)
{
	return c == 0x9 || c == 0xB || c == 0xC || c == 0x20 || c == 0xA0 || c == 0xFEFF;
}

static inline int isnewline(c)
{
	return c == 0xA || c == 0xD || c == 0x2028 || c == 0x2029;
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
		return c - 'a' + 0xA;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 0xA;
	return 0;
}

#define PEEK (J->lexchar)
#define NEXT() next(J, sp)
#define NEXTPEEK (NEXT(), PEEK)
#define ACCEPT(x) (PEEK == x ? (NEXT(), 1) : 0)
#define EXPECT(x) (ACCEPT(x) || (jsP_error(J, "expected '%c'", x), 0))

static void next(js_State *J, const char **sp)
{
	int c = *(*sp)++;
	/* consume CR LF as one unit */
	if (c == '\r' && PEEK == '\n')
		(*sp)++;
	if (isnewline(c)) {
		J->line++;
		c = '\n';
	}
	J->lexchar = c;
}

static void textinit(js_State *J)
{
	if (!J->buf.text) {
		J->buf.cap = 4096;
		J->buf.text = malloc(J->buf.cap);
	}
	J->buf.len = 0;
}

static inline void textpush(js_State *J, int c)
{
	if (J->buf.len >= J->buf.cap) {
		J->buf.cap = J->buf.cap * 2;
		J->buf.text = realloc(J->buf.text, J->buf.cap);
	}
	J->buf.text[J->buf.len++] = c;
}

static inline char *textend(js_State *J)
{
	textpush(J, 0);
	return J->buf.text;
}

static inline void lexlinecomment(js_State *J, const char **sp)
{
	while (PEEK && !isnewline(PEEK))
		NEXT();
}

static inline int lexcomment(js_State *J, const char **sp)
{
	/* already consumed initial '/' '*' sequence */
	while (PEEK != 0) {
		if (ACCEPT('*')) {
			while (ACCEPT('*'))
				NEXT();
			if (ACCEPT('/'))
				return 0;
		}
		NEXT();
	}
	return -1;
}

static inline double lexhex(js_State *J, const char **sp)
{
	double n = 0;
	if (!ishex(PEEK))
		return jsP_error(J, "malformed hexadecimal number");
	while (ishex(PEEK)) {
		n = n * 16 + tohex(PEEK);
		NEXT();
	}
	return n;
}

static inline double lexinteger(js_State *J, const char **sp)
{
	double n = 0;
	if (!isdec(PEEK))
		return jsP_error(J, "malformed number");
	while (isdec(PEEK)) {
		n = n * 10 + (PEEK - '0');
		NEXT();
	}
	return n;
}

static inline double lexfraction(js_State *J, const char **sp)
{
	double n = 0;
	double d = 1;
	while (isdec(PEEK)) {
		n = n * 10 + (PEEK - '0');
		d = d * 10;
		NEXT();
	}
	return n / d;
}

static inline double lexexponent(js_State *J, const char **sp)
{
	double sign;
	if (ACCEPT('e') || ACCEPT('E')) {
		if (ACCEPT('-')) sign = -1;
		else if (ACCEPT('+')) sign = 1;
		else sign = 1;
		return sign * lexinteger(J, sp);
	}
	return 0;
}

static inline int lexnumber(js_State *J, const char **sp)
{
	double n;

	if (ACCEPT('0')) {
		if (ACCEPT('x') || ACCEPT('X')) {
			J->number = lexhex(J, sp);
			return TK_NUMBER;
		}
		if (isdec(PEEK))
			return jsP_error(J, "number with leading zero");
		n = 0;
		if (ACCEPT('.'))
			n += lexfraction(J, sp);
		n *= pow(10, lexexponent(J, sp));
	} else if (ACCEPT('.')) {
		if (!isdec(PEEK))
			return '.';
		n = lexfraction(J, sp);
		n *= pow(10, lexexponent(J, sp));
	} else {
		n = lexinteger(J, sp);
		if (ACCEPT('.'))
			n += lexfraction(J, sp);
		n *= pow(10, lexexponent(J, sp));
	}

	if (isidentifierstart(PEEK))
		return jsP_error(J, "number with letter suffix");

	J->number = n;
	return TK_NUMBER;
}

static inline int lexescape(js_State *J, const char **sp)
{
	int x = 0;

	/* already consumed '\' */

	if (isnewline(PEEK)) {
		NEXT();
		return 0;
	}

	switch (PEEK) {
	case 'u':
		NEXT();
		if (!ishex(PEEK)) return 1; else { x |= PEEK << 12; NEXT(); }
		if (!ishex(PEEK)) return 1; else { x |= PEEK << 8; NEXT(); }
		if (!ishex(PEEK)) return 1; else { x |= PEEK << 4; NEXT(); }
		if (!ishex(PEEK)) return 1; else { x |= PEEK; NEXT(); }
		textpush(J, x);
		break;
	case 'x':
		NEXT();
		if (!ishex(PEEK)) return 1; else { x |= PEEK << 4; NEXT(); }
		if (!ishex(PEEK)) return 1; else { x |= PEEK; NEXT(); }
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

static inline int lexstring(js_State *J, const char **sp)
{
	const char *s;

	int q = PEEK;
	NEXT();

	textinit(J);

	while (PEEK != q) {
		if (PEEK == 0 || isnewline(PEEK))
			return jsP_error(J, "string not terminated");
		if (ACCEPT('\\')) {
			if (lexescape(J, sp))
				return jsP_error(J, "malformed escape sequence");
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

static int lexregexp(js_State *J, const char **sp)
{
	const char *s;
	int g, m, i;
	int c;

	/* already consumed initial '/' */

	textinit(J);

	/* regexp body */
	while (PEEK != '/') {
		if (PEEK == 0 || isnewline(PEEK)) {
			return jsP_error(J, "regular expression not terminated");
		} else if (ACCEPT('\\')) {
			textpush(J, '\\');
			if (PEEK == 0 || isnewline(PEEK))
				return jsP_error(J, "regular expression not terminated");
			textpush(J, PEEK);
			NEXT();
		} else {
			textpush(J, PEEK);
			NEXT();
		}
	}
	EXPECT('/');

	s = textend(J);

	/* regexp flags */
	g = i = m = 0;

	c = PEEK;
	while (isidentifierpart(PEEK)) {
		if (ACCEPT('g')) ++g;
		else if (ACCEPT('i')) ++i;
		else if (ACCEPT('m')) ++m;
		else return jsP_error(J, "illegal flag in regular expression: %c", PEEK);
	}

	if (g > 1 || i > 1 || m > 1)
		return jsP_error(J, "duplicated flag in regular expression");

	J->text = js_intern(J, s);
	J->number = 0;
	if (g) J->number += JS_REGEXP_G;
	if (i) J->number += JS_REGEXP_I;
	if (m) J->number += JS_REGEXP_M;
	return TK_REGEXP;
}

/* simple "return [no Line Terminator here] ..." contexts */
static inline int isnlthcontext(int last)
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

static int lex(js_State *J, const char **sp)
{
	J->newline = 0;

	while (1) {
		J->lexline = J->line; /* save location of beginning of token */

		while (iswhite(PEEK))
			NEXT();

		if (isnewline(PEEK)) {
			NEXT();
			J->newline = 1;
			if (isnlthcontext(J->lasttoken))
				return ';';
			continue;
		}

		if (ACCEPT('/')) {
			if (ACCEPT('/')) {
				lexlinecomment(J, sp);
				continue;
			} else if (ACCEPT('*')) {
				if (lexcomment(J, sp))
					return jsP_error(J, "multi-line comment not terminated");
				continue;
			} else if (isregexpcontext(J->lasttoken)) {
				return lexregexp(J, sp);
			} else if (ACCEPT('=')) {
				return TK_DIV_ASS;
			} else {
				return '/';
			}
		}

		// TODO: \uXXXX escapes
		if (isidentifierstart(PEEK)) {
			textinit(J);
			textpush(J, PEEK);

			NEXT();
			while (isidentifierpart(PEEK)) {
				textpush(J, PEEK);
				NEXT();
			}

			textend(J);

			return findkeyword(J, J->buf.text);
		}

		if (PEEK >= '0' && PEEK <= '9') {
			return lexnumber(J, sp);
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
			return lexstring(J, sp);

		case '.':
			return lexnumber(J, sp);

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

		if (PEEK >= 0x20 && PEEK <= 0x7E)
			return jsP_error(J, "unexpected character: '%c'", PEEK);
		return jsP_error(J, "unexpected character: \\u%04X", PEEK);
	}
}

void jsP_initlex(js_State *J, const char *filename, const char *source)
{
	J->filename = filename;
	J->source = source;
	J->line = 1;
	J->lasttoken = 0;
	next(J, &J->source); /* load first lookahead character */
}

int jsP_lex(js_State *J)
{
	return J->lasttoken = lex(J, &J->source);
}
