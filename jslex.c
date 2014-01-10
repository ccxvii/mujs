#include "js.h"
#include "jsstate.h"
#include "jslex.h"

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

static const char *futurewords[] = {
	"class", "const", "enum", "export", "extends", "import", "super",
};

static const char *strictfuturewords[] = {
	"implements", "interface", "let", "package", "private", "protected",
	"public", "static", "yield",
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

	if (findword(s, futurewords, nelem(futurewords)) >= 0)
		return jsP_error(J, "'%s' is a future reserved word", s);
	if (J->strict && findword(s, strictfuturewords, nelem(strictfuturewords)) >= 0)
		return jsP_error(J, "'%s' is a strict mode future reserved word", s);

	J->text = js_intern(J, s);
	return TK_IDENTIFIER;
}

#define GET() (*(*sp)++)
#define UNGET() ((*sp)--)
#define PEEK() (**sp)
#define NEXT() ((*sp)++)
#define NEXTPEEK() (NEXT(), PEEK())
#define LOOK(x) (PEEK() == x ? (NEXT(), 1) : 0)

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

static inline int lexnumber(js_State *J, const char **sp)
{
	double n;

	if ((*sp)[0] == '0' && ((*sp)[1] == 'x' || (*sp)[1] == 'X')) {
		*sp += 2;
		if (!ishex(PEEK()))
			return jsP_error(J, "0x not followed by hexademical digit");
		J->number = lexhex(sp);
		return TK_NUMBER;
	}

	if ((*sp)[0] == '0' && isdec((*sp)[1]))
		return jsP_error(J, "number with leading zero");

	n = lexinteger(sp);
	if (LOOK('.'))
		n += lexfraction(sp);
	n *= pow(10, lexexponent(sp));

	if (isidentifierstart(PEEK()))
		return jsP_error(J, "number with letter suffix");

	J->number = n;
	return TK_NUMBER;
}

static inline int lexescape(js_State *J, const char **sp)
{
	int c = GET();
	int x = 0;

	if (isnewline(c)) {
		if (c == '\r' && PEEK() == '\n')
			NEXT();
		return 0;
	}

	switch (c) {
	case 'u':
		if (!ishex(PEEK())) return 1; else x |= NEXTPEEK() << 12;
		if (!ishex(PEEK())) return 1; else x |= NEXTPEEK() << 8;
		if (!ishex(PEEK())) return 1; else x |= NEXTPEEK() << 4;
		if (!ishex(PEEK())) return 1; else x |= NEXTPEEK();
		textpush(J, x);
		break;
	case 'x':
		if (!ishex(PEEK())) return 1; else x |= NEXTPEEK() << 4;
		if (!ishex(PEEK())) return 1; else x |= NEXTPEEK();
		textpush(J, x);
		break;
	case '0': textpush(J, 0); break;
	case '\\': textpush(J, '\\'); break;
	case '\'': textpush(J, '\''); break;
	case '"': textpush(J, '"'); break;
	case 'b': textpush(J, '\b'); break;
	case 'f': textpush(J, '\f'); break;
	case 'n': textpush(J, '\n'); break;
	case 'r': textpush(J, '\r'); break;
	case 't': textpush(J, '\t'); break;
	case 'v': textpush(J, '\v'); break;
	default: textpush(J, c); break;
	}
	return 0;
}

static inline int lexstring(js_State *J, const char **sp, int q)
{
	const char *s;
	int c = GET();

	textinit(J);

	while (c != q) {
		if (c == 0 || isnewline(c))
			return jsP_error(J, "string not terminated");
		if (c == '\\') {
			if (lexescape(J, sp))
				return jsP_error(J, "malformed escape sequence");
		} else {
			textpush(J, c);
		}
		c = GET();
	}

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
	int c;

	textinit(J);

	/* regexp body */
	c = GET();
	while (c != '/') {
		if (c == 0 || isnewline(c)) {
			return jsP_error(J, "regular expression not terminated");
		} else if (c == '\\') {
			textpush(J, c);
			c = GET();
			if (c == 0 || isnewline(c))
				return jsP_error(J, "regular expression not terminated");
			textpush(J, c);
			c = GET();
		} else {
			textpush(J, c);
			c = GET();
		}
	}

	s = textend(J);

	/* regexp flags */
	J->flags.g = J->flags.i = J->flags.m = 0;

	c = PEEK();
	while (isidentifierpart(c)) {
		if (c == 'g') J->flags.g ++;
		else if (c == 'i') J->flags.i ++;
		else if (c == 'm') J->flags.m ++;
		else return jsP_error(J, "illegal flag in regular expression: %c", c);
		c = NEXTPEEK();
	}

	if (J->flags.g > 1 || J->flags.i > 1 || J->flags.m > 1)
		return jsP_error(J, "duplicated flag in regular expression");

	J->text = js_intern(J, s);
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
		int c = GET();

		while (iswhite(c))
			c = GET();

		if (isnewline(c)) {
			/* consume CR LF as one unit */
			if (c == '\r' && PEEK() == '\n')
				NEXT();
			J->line++;
			J->newline = 1;
			if (isnlthcontext(J->lasttoken))
				return ';';
			continue;
		}

		if (c == '/') {
			if (LOOK('/')) {
				lexlinecomment(sp);
				continue;
			} else if (LOOK('*')) {
				if (lexcomment(sp))
					return jsP_error(J, "multi-line comment not terminated");
				continue;
			} else if (isregexpcontext(J->lasttoken)) {
				return lexregexp(J, sp);
			} else if (LOOK('=')) {
				return TK_DIV_ASS;
			} else {
				return '/';
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

			return findkeyword(J, J->buf.text);
		}

		if (c >= '0' && c <= '9') {
			UNGET();
			return lexnumber(J, sp);
		}

		switch (c) {
		case '(':
		case ')':
		case ',':
		case ':':
		case ';':
		case '?':
		case '[':
		case ']':
		case '{':
		case '}':
		case '~':
			return c;

		case '\'':
		case '"':
			return lexstring(J, sp, c);

		case '.':
			if (isdec(PEEK())) {
				UNGET();
				return lexnumber(J, sp);
			}
			return '.';

		case '<':
			if (LOOK('<')) {
				if (LOOK('='))
					return TK_SHL_ASS;
				return TK_SHL;
			}
			if (LOOK('='))
				return TK_LE;
			return '<';

		case '>':
			if (LOOK('>')) {
				if (LOOK('>')) {
					if (LOOK('='))
						return TK_USHR_ASS;
					return TK_USHR;
				}
				if (LOOK('='))
					return TK_SHR_ASS;
				return TK_SHR;
			}
			if (LOOK('='))
				return TK_GE;
			return '>';

		case '=':
			if (LOOK('=')) {
				if (LOOK('='))
					return TK_EQ3;
				return TK_EQ;
			}
			return '=';

		case '!':
			if (LOOK('=')) {
				if (LOOK('='))
					return TK_NE3;
				return TK_NE;
			}
			return '!';

		case '+':
			if (LOOK('+'))
				return TK_INC;
			if (LOOK('='))
				return TK_ADD_ASS;
			return '+';

		case '-':
			if (LOOK('-'))
				return TK_DEC;
			if (LOOK('='))
				return TK_SUB_ASS;
			return '-';

		case '*':
			if (LOOK('='))
				return TK_MUL_ASS;
			return '*';

		case '%':
			if (LOOK('='))
				return TK_MOD_ASS;
			return '%';

		case '&':
			if (LOOK('&'))
				return TK_AND;
			if (LOOK('='))
				return TK_AND_ASS;
			return '&';

		case '|':
			if (LOOK('|'))
				return TK_OR;
			if (LOOK('='))
				return TK_OR_ASS;
			return '|';

		case '^':
			if (LOOK('='))
				return TK_XOR_ASS;
			return '^';

		case 0:
			return 0; /* EOF */
		}

		if (c >= 0x20 && c <= 0x7E)
			return jsP_error(J, "unexpected character: '%c'", c);
		return jsP_error(J, "unexpected character: \\u%04X", c);
	}
}

void jsP_initlex(js_State *J, const char *filename, const char *source)
{
	J->filename = filename;
	J->source = source;
	J->line = 1;
	J->lasttoken = 0;
}

int jsP_lex(js_State *J)
{
	return J->lasttoken = lex(J, &J->source);
}
