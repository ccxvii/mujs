#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <limits.h>

#include "regex.h"
#include "utf.h"

#define emit regemit
#define next regnext
#define accept regaccept

#define nelem(a) (sizeof (a) / sizeof (a)[0])

#define REPINF 255

typedef struct Reclass Reclass;
typedef struct Renode Renode;
typedef struct Reinst Reinst;

struct Reclass {
	Rune *end;
	Rune spans[64];
};

struct Reprog {
	Reinst *start, *end;
	int icase, newline;
	unsigned int ncap;
	Reclass cclass[16];
};

struct cstate {
	Reprog *prog;
	Renode *pstart, *pend;
	int flags;

	const char *source;
	unsigned int ncclass;
	unsigned int ncap;
	int nref[10];

	int lookahead;
	Rune yychar;
	Reclass *yycc;
	int yymin, yymax;

	const char *error;
	jmp_buf kaboom;
};

static void die(struct cstate *g, const char *message)
{
	g->error = message;
	longjmp(g->kaboom, 1);
}

static int canon(Rune c)
{
	Rune u = toupperrune(c);
	if (c >= 128 && u < 128)
		return c;
	return u;
}

/* Scan */

enum {
	L_CHAR = 256,
	L_CCLASS,	/* character class */
	L_NCCLASS,	/* negative character class */
	L_NC,		/* "(?:" no capture */
	L_PLA,		/* "(?=" positive lookahead */
	L_NLA,		/* "(?!" negative lookahead */
	L_WORD,		/* "\b" word boundary */
	L_NWORD,	/* "\B" non-word boundary */
	L_REF,		/* "\0" back-reference */
	L_COUNT,	/* {M,N} */
};

static int hex(struct cstate *g, int c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 0xA;
	if (c >= 'A' && c <= 'F') return c - 'A' + 0xA;
	die(g, "invalid escape sequence");
	return 0;
}

static int dec(struct cstate *g, int c)
{
	if (c >= '0' && c <= '9') return c - '0';
	die(g, "invalid quantifier");
	return 0;
}

static int nextrune(struct cstate *g)
{
	g->source += chartorune(&g->yychar, g->source);
	if (g->yychar == '\\') {
		g->source += chartorune(&g->yychar, g->source);
		switch (g->yychar) {
		case 0: die(g, "unterminated escape sequence");
		case 'f': g->yychar = '\f'; return 0;
		case 'n': g->yychar = '\n'; return 0;
		case 'r': g->yychar = '\r'; return 0;
		case 't': g->yychar = '\t'; return 0;
		case 'v': g->yychar = '\v'; return 0;
		case 'c':
			g->yychar = (*g->source++) & 31;
			return 0;
		case 'x':
			g->yychar = hex(g, *g->source++) << 4;
			g->yychar += hex(g, *g->source++);
			return 0;
		case 'u':
			g->yychar = hex(g, *g->source++) << 12;
			g->yychar += hex(g, *g->source++) << 8;
			g->yychar += hex(g, *g->source++) << 4;
			g->yychar += hex(g, *g->source++);
			return 0;
		}
		return 1;
	}
	return 0;
}

static int lexcount(struct cstate *g)
{
	nextrune(g);

	g->yymin = dec(g, g->yychar);
	nextrune(g);
	while (g->yychar != ',' && g->yychar != '}') {
		g->yymin = g->yymin * 10 + dec(g, g->yychar);
		nextrune(g);
	}
	if (g->yymin >= REPINF)
		die(g, "numeric overflow");

	if (g->yychar == ',') {
		nextrune(g);
		if (g->yychar == '}') {
			g->yymax = REPINF;
		} else {
			g->yymax = dec(g, g->yychar);
			nextrune(g);
			while (g->yychar != '}') {
				g->yymax = g->yymax * 10 + dec(g, g->yychar);
				nextrune(g);
			}
			if (g->yymax >= REPINF)
				die(g, "numeric overflow");
		}
	} else {
		g->yymax = g->yymin;
	}

	return L_COUNT;
}

static void newcclass(struct cstate *g)
{
	if (g->ncclass >= nelem(g->prog->cclass))
		die(g, "too many character classes");
	g->yycc = g->prog->cclass + g->ncclass++;
	g->yycc->end = g->yycc->spans;
}

static void addrange(struct cstate *g, Rune a, Rune b)
{
	if (a > b)
		die(g, "invalid character class range");
	if (g->yycc->end + 2 == g->yycc->spans + nelem(g->yycc->spans))
		die(g, "too many character class ranges");
	*g->yycc->end++ = a;
	*g->yycc->end++ = b;
}

static void addranges_d(struct cstate *g)
{
	addrange(g, '0', '9');
}

static void addranges_D(struct cstate *g)
{
	addrange(g, 0, '0'-1);
	addrange(g, '9'+1, 0xFFFF);
}

static void addranges_s(struct cstate *g)
{
	addrange(g, 0x9, 0x9);
	addrange(g, 0xA, 0xD);
	addrange(g, 0x20, 0x20);
	addrange(g, 0xA0, 0xA0);
	addrange(g, 0x2028, 0x2029);
	addrange(g, 0xFEFF, 0xFEFF);
}

static void addranges_S(struct cstate *g)
{
	addrange(g, 0, 0x9-1);
	addrange(g, 0x9+1, 0xA-1);
	addrange(g, 0xD+1, 0x20-1);
	addrange(g, 0x20+1, 0xA0-1);
	addrange(g, 0xA0+1, 0x2028-1);
	addrange(g, 0x2029+1, 0xFEFF-1);
	addrange(g, 0xFEFF+1, 0xFFFF);
}

static void addranges_w(struct cstate *g)
{
	addrange(g, '0', '9');
	addrange(g, 'A', 'Z');
	addrange(g, '_', '_');
	addrange(g, 'a', 'z');
}

static void addranges_W(struct cstate *g)
{
	addrange(g, 0, '0'-1);
	addrange(g, '9'+1, 'A'-1);
	addrange(g, 'Z'+1, '_'-1);
	addrange(g, '_'+1, 'a'-1);
	addrange(g, 'z'+1, 0xFFFF);
}

static int lexclass(struct cstate *g)
{
	int type = L_CCLASS;
	int quoted, havesave, havedash;
	Rune save;

	newcclass(g);

	quoted = nextrune(g);
	if (!quoted && g->yychar == '^') {
		type = L_NCCLASS;
		quoted = nextrune(g);
	}

	havesave = havedash = 0;
	for (;;) {
		if (g->yychar == 0)
			die(g, "unterminated character class");
		if (!quoted && g->yychar == ']')
			break;

		if (!quoted && g->yychar == '-') {
			if (havesave) {
				if (havedash) {
					addrange(g, save, '-');
					havesave = havedash = 0;
				} else {
					havedash = 1;
				}
			} else {
				save = '-';
				havesave = 1;
			}
		} else if (quoted && strchr("DSWdsw", g->yychar)) {
			if (havesave) {
				addrange(g, save, save);
				if (havedash)
					addrange(g, '-', '-');
			}
			switch (g->yychar) {
			case 'd': addranges_d(g); break;
			case 's': addranges_s(g); break;
			case 'w': addranges_w(g); break;
			case 'D': addranges_D(g); break;
			case 'S': addranges_S(g); break;
			case 'W': addranges_W(g); break;
			}
			havesave = havedash = 0;
		} else {
			if (quoted && g->yychar == 'b')
				g->yychar = '\b';
			if (havesave) {
				if (havedash) {
					addrange(g, save, g->yychar);
					havesave = havedash = 0;
				} else {
					addrange(g, save, save);
					save = g->yychar;
				}
			} else {
				save = g->yychar;
				havesave = 1;
			}
		}

		quoted = nextrune(g);
	}

	if (havesave) {
		addrange(g, save, save);
		if (havedash)
			addrange(g, '-', '-');
	}

	return type;
}

static int lex(struct cstate *g)
{
	int quoted = nextrune(g);
	if (quoted) {
		switch (g->yychar) {
		case 'b': return L_WORD;
		case 'B': return L_NWORD;
		case 'd': newcclass(g); addranges_d(g); return L_CCLASS;
		case 's': newcclass(g); addranges_s(g); return L_CCLASS;
		case 'w': newcclass(g); addranges_w(g); return L_CCLASS;
		case 'D': newcclass(g); addranges_d(g); return L_NCCLASS;
		case 'S': newcclass(g); addranges_s(g); return L_NCCLASS;
		case 'W': newcclass(g); addranges_w(g); return L_NCCLASS;
		}
		if (g->yychar >= '0' && g->yychar <= '9') {
			g->yychar -= '0';
			return L_REF;
		}
		return L_CHAR;
	}

	switch (g->yychar) {
	case '*': case '+': case '?': case '|':
	case ')': case '.': case '^': case '$':
	case 0:
		return g->yychar;
	}

	if (g->yychar == '{')
		return lexcount(g);
	if (g->yychar == '[')
		return lexclass(g);
	if (g->yychar == '(') {
		if (g->source[0] == '?') {
			if (g->source[1] == ':') {
				g->source += 2;
				return L_NC;
			}
			if (g->source[1] == '=') {
				g->source += 2;
				return L_PLA;
			}
			if (g->source[1] == '!') {
				g->source += 2;
				return L_NLA;
			}
		}
		return '(';
	}

	return L_CHAR;
}

/* Parse */

enum {
	P_CAT, P_ALT, P_REP,
	P_BOL, P_EOL, P_WORD, P_NWORD,
	P_PAR, P_PLA, P_NLA,
	P_ANY, P_CHAR, P_CCLASS, P_NCCLASS,
	P_REF,
};

struct Renode {
	unsigned char type;
	unsigned char ng, m, n;
	Rune c;
	Reclass *cc;
	Renode *x;
	Renode *y;
};

static Renode *newnode(struct cstate *g, int type)
{
	Renode *node = g->pend++;
	node->type = type;
	node->cc = NULL;
	node->c = 0;
	node->ng = 0;
	node->m = 0;
	node->n = 0;
	node->x = node->y = NULL;
	return node;
}

static Renode *newrep(struct cstate *g, Renode *atom, int ng, int min, int max)
{
	Renode *rep = newnode(g, P_REP);
	rep->ng = ng;
	rep->m = min;
	rep->n = max;
	rep->x = atom;
	return rep;
}

static void next(struct cstate *g)
{
	g->lookahead = lex(g);
}

static int accept(struct cstate *g, int t)
{
	if (g->lookahead == t) {
		next(g);
		return 1;
	}
	return 0;
}

static Renode *parsealt(struct cstate *g);

static Renode *parseatom(struct cstate *g)
{
	Renode *atom;
	if (g->lookahead == L_CHAR) {
		atom = newnode(g, P_CHAR);
		atom->c = g->yychar;
		next(g);
		return atom;
	}
	if (g->lookahead == L_CCLASS) {
		atom = newnode(g, P_CCLASS);
		atom->cc = g->yycc;
		next(g);
		return atom;
	}
	if (g->lookahead == L_NCCLASS) {
		atom = newnode(g, P_NCCLASS);
		atom->cc = g->yycc;
		next(g);
		return atom;
	}
	if (g->lookahead == L_REF) {
		atom = newnode(g, P_REF);
		if (g->yychar == 0 || g->yychar > g->ncap || !g->nref[g->yychar])
			die(g, "invalid back-reference");
		atom->n = g->yychar;
		next(g);
		return atom;
	}
	if (accept(g, '.'))
		return newnode(g, P_ANY);
	if (accept(g, '(')) {
		atom = newnode(g, P_PAR);
		if (++g->ncap == 10)
			die(g, "too many captures");
		atom->n = g->ncap;
		g->nref[atom->n] = 0;
		atom->x = parsealt(g);
		g->nref[atom->n] = 1;
		if (!accept(g, ')'))
			die(g, "unmatched '('");
		return atom;
	}
	if (accept(g, L_NC)) {
		atom = parsealt(g);
		if (!accept(g, ')'))
			die(g, "unmatched '('");
		return atom;
	}
	if (accept(g, L_PLA)) {
		atom = newnode(g, P_PLA);
		atom->x = parsealt(g);
		if (!accept(g, ')'))
			die(g, "unmatched '('");
		return atom;
	}
	if (accept(g, L_NLA)) {
		atom = newnode(g, P_NLA);
		atom->x = parsealt(g);
		if (!accept(g, ')'))
			die(g, "unmatched '('");
		return atom;
	}
	die(g, "syntax error");
	return NULL;
}

static Renode *parserep(struct cstate *g)
{
	Renode *atom;

	if (accept(g, '^')) return newnode(g, P_BOL);
	if (accept(g, '$')) return newnode(g, P_EOL);
	if (accept(g, L_WORD)) return newnode(g, P_WORD);
	if (accept(g, L_NWORD)) return newnode(g, P_NWORD);

	atom = parseatom(g);
	if (g->lookahead == L_COUNT) {
		int min = g->yymin, max = g->yymax;
		next(g);
		if (max < min)
			die(g, "invalid quantifier");
		return newrep(g, atom, accept(g, '?'), min, max);
	}
	if (accept(g, '*')) return newrep(g, atom, accept(g, '?'), 0, REPINF);
	if (accept(g, '+')) return newrep(g, atom, accept(g, '?'), 1, REPINF);
	if (accept(g, '?')) return newrep(g, atom, accept(g, '?'), 0, 1);
	return atom;
}

static Renode *parsecat(struct cstate *g)
{
	Renode *cat, *x;
	if (g->lookahead && g->lookahead != '|' && g->lookahead != ')') {
		cat = parserep(g);
		while (g->lookahead && g->lookahead != '|' && g->lookahead != ')') {
			x = cat;
			cat = newnode(g, P_CAT);
			cat->x = x;
			cat->y = parserep(g);
		}
		return cat;
	}
	return NULL;
}

static Renode *parsealt(struct cstate *g)
{
	Renode *alt, *x;
	alt = parsecat(g);
	while (accept(g, '|')) {
		x = alt;
		alt = newnode(g, P_ALT);
		alt->x = x;
		alt->y = parsecat(g);
	}
	return alt;
}

/* Compile */

enum {
	I_END, I_JUMP, I_SPLIT, I_PLA, I_NLA,
	I_ANY, I_CHAR, I_CCLASS, I_NCCLASS, I_REF,
	I_BOL, I_EOL, I_WORD, I_NWORD,
	I_LPAR, I_RPAR
};

struct Reinst {
	unsigned char opcode;
	unsigned char n;
	Rune c;
	Reclass *cc;
	Reinst *x;
	Reinst *y;
	const char *p;
};

static unsigned int count(Renode *node)
{
	unsigned int min, max;
	if (!node) return 0;
	switch (node->type) {
	default: return 1;
	case P_CAT: return count(node->x) + count(node->y);
	case P_ALT: return count(node->x) + count(node->y) + 2;
	case P_REP:
		min = node->m;
		max = node->n;
		if (min == max) return count(node->x) * min;
		if (max < REPINF) return count(node->x) * max + (max - min);
		return count(node->x) * (min + 1) + 2;
	case P_PAR: return count(node->x) + 2;
	case P_PLA: return count(node->x) + 2;
	case P_NLA: return count(node->x) + 2;
	}
}

static Reinst *emit(Reprog *prog, int opcode)
{
	Reinst *inst = prog->end++;
	inst->opcode = opcode;
	inst->cc = NULL;
	inst->c = 0;
	inst->n = 0;
	inst->x = inst->y = NULL;
	inst->p = 0;
	return inst;
}

static void compile(Reprog *prog, Renode *node)
{
	Reinst *inst, *split, *jump;
	unsigned int i;

	if (!node)
		return;

	switch (node->type) {
	case P_CAT:
		compile(prog, node->x);
		compile(prog, node->y);
		break;

	case P_ALT:
		split = emit(prog, I_SPLIT);
		compile(prog, node->x);
		jump = emit(prog, I_JUMP);
		compile(prog, node->y);
		split->x = split + 1;
		split->y = jump + 1;
		jump->x = prog->end;
		break;

	case P_REP:
		for (i = 0; i < node->m; ++i) {
			inst = prog->end;
			compile(prog, node->x);
		}
		if (node->m == node->n)
			break;
		if (node->n < REPINF) {
			for (i = node->m; i < node->n; ++i) {
				split = emit(prog, I_SPLIT);
				compile(prog, node->x);
				if (node->ng) {
					split->y = split + 1;
					split->x = prog->end;
				} else {
					split->x = split + 1;
					split->y = prog->end;
				}
			}
		} else if (node->m == 0) {
			split = emit(prog, I_SPLIT);
			compile(prog, node->x);
			jump = emit(prog, I_JUMP);
			if (node->ng) {
				split->y = split + 1;
				split->x = prog->end;
			} else {
				split->x = split + 1;
				split->y = prog->end;
			}
			jump->x = split;
		} else {
			split = emit(prog, I_SPLIT);
			if (node->ng) {
				split->y = inst;
				split->x = prog->end;
			} else {
				split->x = inst;
				split->y = prog->end;
			}
		}
		break;

	case P_BOL: emit(prog, I_BOL); break;
	case P_EOL: emit(prog, I_EOL); break;
	case P_WORD: emit(prog, I_WORD); break;
	case P_NWORD: emit(prog, I_NWORD); break;

	case P_PAR:
		inst = emit(prog, I_LPAR);
		inst->n = node->n;
		compile(prog, node->x);
		inst = emit(prog, I_RPAR);
		inst->n = node->n;
		break;
	case P_PLA:
		split = emit(prog, I_PLA);
		compile(prog, node->x);
		emit(prog, I_END);
		split->x = split + 1;
		split->y = prog->end;
		break;
	case P_NLA:
		split = emit(prog, I_NLA);
		compile(prog, node->x);
		emit(prog, I_END);
		split->x = split + 1;
		split->y = prog->end;
		break;

	case P_ANY:
		emit(prog, I_ANY);
		break;
	case P_CHAR:
		inst = emit(prog, I_CHAR);
		inst->c = prog->icase ? canon(node->c) : node->c;
		break;
	case P_CCLASS:
		inst = emit(prog, I_CCLASS);
		inst->cc = node->cc;
		break;
	case P_NCCLASS:
		inst = emit(prog, I_NCCLASS);
		inst->cc = node->cc;
		break;
	case P_REF:
		inst = emit(prog, I_REF);
		inst->n = node->n;
		break;
	}
}

#ifdef TEST
static void dumpnode(Renode *node)
{
	Rune *p;
	if (!node) { printf("Empty"); return; }
	switch (node->type) {
	case P_CAT: printf("Cat("); dumpnode(node->x); printf(", "); dumpnode(node->y); printf(")"); break;
	case P_ALT: printf("Alt("); dumpnode(node->x); printf(", "); dumpnode(node->y); printf(")"); break;
	case P_REP:
		printf(node->ng ? "NgRep(%d,%d," : "Rep(%d,%d,", node->m, node->n);
		dumpnode(node->x);
		printf(")");
		break;
	case P_BOL: printf("Bol"); break;
	case P_EOL: printf("Eol"); break;
	case P_WORD: printf("Word"); break;
	case P_NWORD: printf("NotWord"); break;
	case P_PAR: printf("Par(%d,", node->n); dumpnode(node->x); printf(")"); break;
	case P_PLA: printf("PLA("); dumpnode(node->x); printf(")"); break;
	case P_NLA: printf("NLA("); dumpnode(node->x); printf(")"); break;
	case P_ANY: printf("Any"); break;
	case P_CHAR: printf("Char(%c)", node->c); break;
	case P_CCLASS:
		printf("Class(");
		for (p = node->cc->spans; p < node->cc->end; p += 2) printf("%02X-%02X,", p[0], p[1]);
		printf(")");
		break;
	case P_NCCLASS:
		printf("NotClass(");
		for (p = node->cc->spans; p < node->cc->end; p += 2) printf("%02X-%02X,", p[0], p[1]);
		printf(")");
		break;
	case P_REF: printf("Ref(%d)", node->n); break;
	}
}

static void dumpprog(Reprog *prog)
{
	Reinst *inst;
	int i;
	for (i = 0, inst = prog->start; inst < prog->end; ++i, ++inst) {
		printf("% 5d: ", i);
		switch (inst->opcode) {
		case I_END: puts("end"); break;
		case I_JUMP: printf("jump %d\n", (int)(inst->x - prog->start)); break;
		case I_SPLIT: printf("split %d %d\n", (int)(inst->x - prog->start), (int)(inst->y - prog->start)); break;
		case I_PLA: printf("pla %d %d\n", (int)(inst->x - prog->start), (int)(inst->y - prog->start)); break;
		case I_NLA: printf("nla %d %d\n", (int)(inst->x - prog->start), (int)(inst->y - prog->start)); break;
		case I_ANY: puts("any"); break;
		case I_CHAR: printf(inst->c >= 32 && inst->c < 127 ? "char '%c'\n" : "char U+%04X\n", inst->c); break;
		case I_CCLASS: puts("cclass"); break;
		case I_NCCLASS: puts("ncclass"); break;
		case I_REF: printf("ref %d\n", inst->n); break;
		case I_BOL: puts("bol"); break;
		case I_EOL: puts("eol"); break;
		case I_WORD: puts("word"); break;
		case I_NWORD: puts("nword"); break;
		case I_LPAR: printf("lpar %d\n", inst->n); break;
		case I_RPAR: printf("rpar %d\n", inst->n); break;
		}
	}
}
#endif

Reprog *regcomp(const char *pattern, int cflags, const char **errorp)
{
	struct cstate g;
	Renode *node;
	int i;

	g.prog = malloc(sizeof (Reprog));
	g.pstart = g.pend = malloc(sizeof (Renode) * strlen(pattern) * 2);

	if (setjmp(g.kaboom)) {
		if (errorp) *errorp = g.error;
		free(g.pstart);
		free(g.prog);
		return NULL;
	}

	g.source = pattern;
	g.ncclass = 0;
	g.ncap = 0;
	for (i = 0; i < 10; ++i)
		g.nref[i] = 0;

	g.prog->icase = cflags & REG_ICASE;
	g.prog->newline = cflags & REG_NEWLINE;

	next(&g);
	node = parsealt(&g);
	if (g.lookahead == ')')
		die(&g, "unmatched ')'");
	if (g.lookahead != 0)
		die(&g, "syntax error");

	g.prog->ncap = g.ncap;
	g.prog->start = g.prog->end = malloc((count(node) + 3) * sizeof (Reinst));
	emit(g.prog, I_LPAR);
	compile(g.prog, node);
	emit(g.prog, I_RPAR);
	emit(g.prog, I_END);

#ifdef TEST
	dumpnode(node);
	putchar('\n');
	dumpprog(g.prog);
#endif

	free(g.pstart);

	if (errorp) *errorp = NULL;
	return g.prog;
}

void regfree(Reprog *prog)
{
	if (prog) {
		free(prog->start);
		free(prog);
	}
}

/* Match */

struct estate {
	int icase, newline, notbol;
	int nla;
	const char *bol;
	Resub *m;
};

static int isnewline(int c)
{
	return c == 0xA || c == 0xD || c == 0x2028 || c == 0x2029;
}

static int iswordchar(int c)
{
	return c == '_' ||
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9');
}

static int incclass(Reclass *cc, Rune c)
{
	Rune *p;
	for (p = cc->spans; p < cc->end; p += 2)
		if (p[0] <= c && c <= p[1])
			return 1;
	return 0;
}

static int incclasscanon(Reclass *cc, Rune c)
{
	Rune *p, r;
	for (p = cc->spans; p < cc->end; p += 2)
		for (r = p[0]; r <= p[1]; ++r)
			if (c == canon(r))
				return 1;
	return 0;
}

static int strncmpcanon(const char *a, const char *b, unsigned int n)
{
	Rune ra, rb;
	int c;
	while (n--) {
		if (!*a) return -1;
		if (!*b) return 1;
		a += chartorune(&ra, a);
		b += chartorune(&rb, b);
		c = canon(ra) - canon(rb);
		if (c)
			return c;
	}
	return 0;
}

static int match(struct estate *g, Reinst *pc, const char *s)
{
	const char *p;
	Rune c;
	unsigned int n;
	int v;

	for (;;) {
		switch (pc->opcode) {
		case I_END:
			return 1;
		case I_JUMP:
			pc = pc->x;
			continue;
		case I_SPLIT:
#if 0
			if (match(g, pc->x, s))
				return 1;
			pc = pc->y;
			continue;
#endif
			if (pc->p == s)
				return 0;
			p = pc->p;
			pc->p = s;
			v = match(g, pc->x, s);
			pc->p = p;
			if (v)
				return 1;
			pc = pc->y;
			continue;
		case I_PLA:
			if (!match(g, pc->x, s))
				return 0;
			pc = pc->y;
			continue;
		case I_NLA:
			++g->nla;
			v = match(g, pc->x, s);
			--g->nla;
			if (v)
				return 0;
			pc = pc->y;
			continue;
		case I_ANY:
			s += chartorune(&c, s);
			if (c == 0)
				return 0;
			if (isnewline(c))
				return 0;
			break;
		case I_CHAR:
			s += chartorune(&c, s);
			if (c == 0)
				return 0;
			if (g->icase)
				c = canon(c);
			if (c != pc->c)
				return 0;
			break;
		case I_CCLASS:
			s += chartorune(&c, s);
			if (c == 0)
				return 0;
			if (g->icase) {
				if (!incclasscanon(pc->cc, canon(c)))
					return 0;
			} else {
				if (!incclass(pc->cc, c))
					return 0;
			}
			break;
		case I_NCCLASS:
			s += chartorune(&c, s);
			if (c == 0)
				return 0;
			if (g->icase) {
				if (incclasscanon(pc->cc, canon(c)))
					return 0;
			} else {
				if (incclass(pc->cc, c))
					return 0;
			}
			break;
		case I_REF:
			n = g->m[pc->n].ep - g->m[pc->n].sp;
			if (g->icase) {
				if (strncmpcanon(s, g->m[pc->n].sp, n))
					return 0;
			} else {
				if (strncmp(s, g->m[pc->n].sp, n))
					return 0;
			}
			if (n > 0)
				s += n;
			break;
		case I_BOL:
			if (s == g->bol && !g->notbol)
				break;
			if (g->newline)
				if (s > g->bol && isnewline(s[-1]))
					break;
			return 0;
		case I_EOL:
			if (*s == 0)
				break;
			if (g->newline)
				if (isnewline(*s))
					break;
			return 0;
		case I_WORD:
			v = s > g->bol && iswordchar(s[-1]);
			v ^= iswordchar(s[0]);
			if (v)
				break;
			return 0;
		case I_NWORD:
			v = s > g->bol && iswordchar(s[-1]);
			v ^= iswordchar(s[0]);
			if (!v)
				break;
			return 0;
		case I_LPAR:
			p = g->m[pc->n].sp;
			g->m[pc->n].sp = s;
			if (match(g, pc + 1, s)) {
				if (g->nla) g->m[pc->n].sp = p;
				return 1;
			}
			g->m[pc->n].sp = p;
			return 0;
		case I_RPAR:
			p = g->m[pc->n].ep;
			g->m[pc->n].ep = s;
			if (match(g, pc + 1, s)) {
				if (g->nla) g->m[pc->n].ep = p;
				return 1;
			}
			g->m[pc->n].ep = p;
			return 0;
		default:
			return 0;
		}
		pc = pc + 1;
	}
}

int regexec(Reprog *prog, const char *s, int n, Resub *m, int eflags)
{
	struct estate g;
	Resub gm[10];
	Rune c;
	unsigned int i;

	g.icase = prog->icase;
	g.newline = prog->newline;
	g.notbol = eflags & REG_NOTBOL;
	g.bol = s;
	g.nla = 0;
	g.m = m ? m : gm;
	for (i = 0; i < 10; ++i)
		g.m[i].sp = g.m[i].ep = i <= prog->ncap ? s : NULL;

	do {
		if (match(&g, prog->start, s))
			return 0;
		s += chartorune(&c, s);
	} while (c);

	return 1;
}

#ifdef TEST
int main(int argc, char **argv)
{
	const char *error;
	const char *s;
	Reprog *p;
	Resub m[10];
	int i;

	if (argc > 1) {
		p = regcomp(argv[1], 0, &error);
		if (!p) {
			fprintf(stderr, "regcomp: %s\n", error);
			return 1;
		}

		if (argc > 2) {
			s = argv[2];
			printf("ncap = %d\n", p->ncap);
			if (!regexec(p, s, 10, m, 0)) {
				for (i = 0; i < 10; ++i)
					if (m[i].sp) {
						int n = m[i].ep - m[i].sp;
						printf("match %d: s=%d e=%d n=%d '%.*s'\n", i, (int)(m[i].sp - s), (int)(m[i].ep - s), n, n, m[i].sp);
					}
			} else {
				printf("no match\n");
			}
		}
	}

	return 0;
}
#endif
