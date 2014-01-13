#ifndef js_value_h
#define js_value_h

enum js_ValueType {
	JS_TUNDEFINED,
	JS_TNULL,
	JS_TBOOLEAN,
	JS_TNUMBER,
	JS_TSTRING,
	JS_TREGEXP,
	JS_TOBJECT,
	JS_TCLOSURE,
	JS_TCFUNCTION,
	JS_TREFERENCE,	/* l-value from aval/aindex/amember */
};

struct js_Value
{
	union {
		int boolean;
		double number;
		const char *string;
		struct {
			const char *prog;
			unsigned char flags;
		} regexp;
		js_Object *object;
		js_Closure *closure;
		js_CFunction *cfunction;
		js_Property *reference;
	} u;
	js_ValueType type;
};

void jsC_dumpvalue(js_State *J, js_Value v);

#endif
