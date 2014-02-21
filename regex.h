#ifndef regex_h
#define regex_h

#define regcomp js_regcomp
#define regexec js_regexec
#define regfree js_regfree

typedef struct Reprog Reprog;
typedef struct Resub Resub;
struct Resub {
	const char *sp;
	const char *ep;
};

Reprog *regcomp(const char *pattern, int cflags, const char **errorp);
int regexec(Reprog *prog, const char *string, int nmatch, Resub *pmatch, int eflags);
void regfree(Reprog *prog);

enum {
	/* regcomp flags */
	REG_ICASE = 1,
	REG_NEWLINE = 2
};

enum {
	/* regexec flags */
	REG_NOTBOL = 1,
	REG_NOTEOL = 2
};

#endif
