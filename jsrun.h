#ifndef js_run_h
#define js_run_h

enum js_ValueType {
	JS_TUNDEFINED,
	JS_TNULL,
	JS_TBOOLEAN,
	JS_TNUMBER,
	JS_TSTRING,
	JS_TREGEXP,
	JS_TOBJECT,

	JS_TFUNCTION,
	JS_TCFUNCTION,
	JS_TCLOSURE,
	JS_TARGUMENTS,

	JS_TOBJSLOT,
};

struct js_Value
{
	union {
		double number;
		const char *string;
		int boolean;
		void *p;
	} u;
	int flag;
	js_ValueType type;
};

void jsR_runfunction(js_State *J, js_Function *F);

#endif
