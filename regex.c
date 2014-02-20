#include <stdlib.h>
#include "regex.h"

Reprog *js_regcomp(const char *pattern, int cflags, const char **errorp)
{
	static char msg[256];
	regex_t *prog = malloc(sizeof *prog);
	int status = regcomp(prog, pattern, cflags);
	if (status) {
		free(prog);
		if (errorp) {
			regerror(status, prog, msg, sizeof msg);
			*errorp = msg;
		}
		return NULL;
	}
	if (errorp)
		*errorp = NULL;
	return (Reprog*)prog;
}

int js_regexec(Reprog *prog, const char *string, int nmatch, Resub *pmatch, int eflags)
{
	regmatch_t m[10];
	int i, status;
	status = regexec((regex_t*)prog, string, 10, m, eflags);
	for (i = 0; i < nmatch; ++i) {
		if (m[i].rm_so >= 0) {
			pmatch[i].sp = string + m[i].rm_so;
			pmatch[i].ep = string + m[i].rm_eo;
		} else {
			pmatch[i].sp = NULL;
			pmatch[i].ep = NULL;
		}
	}
	return status;
}

void js_regfree(Reprog *prog)
{
	if (prog) {
		regfree((regex_t*)prog);
		free(prog);
	}
}
