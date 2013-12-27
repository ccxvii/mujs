#include "js.h"
#include "js-parse.h"

int jsP_parse(js_State *J)
{
	int t;
	do {
		t = jsP_lex(J);
		if (t == TK_ERROR)
			return 1;
	} while (t);
	return 0;
}
