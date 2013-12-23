#ifndef js_h
#define js_h

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

typedef struct js_State js_State;

typedef int (*js_CFunction)(js_State *J);

js_State *js_newstate(void);
void js_close(js_State *J);

int js_error(js_State *J, const char *fmt, ...);

int js_loadstring(js_State *J, const char *s);
int js_loadfile(js_State *J, const char *filename);

/* private */

typedef enum js_Token js_Token;

enum js_Token
{
	JS_ERROR,
	JS_EOF,

	JS_IDENTIFIER,
	JS_NULL,
	JS_TRUE,
	JS_FALSE,
	JS_NUMBER,
	JS_STRING,
	JS_REGEXP,
	JS_NEWLINE,

	/* punctuators */
	JS_LCURLY,
	JS_RCURLY,
	JS_LPAREN,
	JS_RPAREN,
	JS_LSQUARE,
	JS_RSQUARE,
	JS_PERIOD,
	JS_SEMICOLON,
	JS_COMMA,
	JS_LT,
	JS_GT,
	JS_LT_EQ,
	JS_GT_EQ,
	JS_EQ_EQ,
	JS_EXCL_EQ,
	JS_EQ_EQ_EQ,
	JS_EXCL_EQ_EQ,
	JS_PLUS,
	JS_MINUS,
	JS_STAR,
	JS_PERCENT,
	JS_PLUS_PLUS,
	JS_MINUS_MINUS,
	JS_LT_LT,
	JS_GT_GT,
	JS_GT_GT_GT,
	JS_AND,
	JS_BAR,
	JS_HAT,
	JS_EXCL,
	JS_TILDE,
	JS_AND_AND,
	JS_BAR_BAR,
	JS_QUESTION,
	JS_COLON,
	JS_EQ,
	JS_PLUS_EQ,
	JS_MINUS_EQ,
	JS_STAR_EQ,
	JS_PERCENT_EQ,
	JS_LT_LT_EQ,
	JS_GT_GT_EQ,
	JS_GT_GT_GT_EQ,
	JS_AND_EQ,
	JS_BAR_EQ,
	JS_HAT_EQ,
	JS_SLASH,
	JS_SLASH_EQ,

	/* keywords */
	JS_BREAK,
	JS_CASE,
	JS_CATCH,
	JS_CONTINUE,
	JS_DEFAULT,
	JS_DELETE,
	JS_DO,
	JS_ELSE,
	JS_FINALLY,
	JS_FOR,
	JS_FUNCTION,
	JS_IF,
	JS_IN,
	JS_INSTANCEOF,
	JS_NEW,
	JS_RETURN,
	JS_SWITCH,
	JS_THIS,
	JS_THROW,
	JS_TRY,
	JS_TYPEOF,
	JS_VAR,
	JS_VOID,
	JS_WHILE,
	JS_WITH,

	/* future reserved words */
	JS_ABSTRACT,
	JS_BOOLEAN,
	JS_BYTE,
	JS_CHAR,
	JS_CLASS,
	JS_CONST,
	JS_DEBUGGER,
	JS_DOUBLE,
	JS_ENUM,
	JS_EXPORT,
	JS_EXTENDS,
	JS_FINAL,
	JS_FLOAT,
	JS_GOTO,
	JS_IMPLEMENTS,
	JS_IMPORT,
	JS_INT,
	JS_INTERFACE,
	JS_LONG,
	JS_NATIVE,
	JS_PACKAGE,
	JS_PRIVATE,
	JS_PROTECTED,
	JS_PUBLIC,
	JS_SHORT,
	JS_STATIC,
	JS_SUPER,
	JS_SYNCHRONIZED,
	JS_THROWS,
	JS_TRANSIENT,
	JS_VOLATILE,

};

struct js_State
{
	char *yytext;
	size_t yylen, yycap;
	double yynumber;
};

js_Token js_lex(js_State *J, const char **sp);
const char *js_tokentostring(js_Token t);

#endif
