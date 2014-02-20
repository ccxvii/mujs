#ifndef regex_h
#define regex_h

#include <regex.h>

typedef struct Reprog Reprog;
typedef struct {
	const char *sp;
	const char *ep;
} Resub;

Reprog *js_regcomp(const char *pattern, int cflags, const char **errorp);
int js_regexec(Reprog *prog, const char *string, int nmatch, Resub *pmatch, int eflags);
void js_regfree(Reprog *prog);

#endif
