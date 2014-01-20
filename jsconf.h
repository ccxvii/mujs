#ifndef js_conf_h
#define js_conf_h

#define JS_STACKSIZE 256	/* value stack size */
#define JS_MINSTACK 20		/* at least this much available when entering a function */
#define JS_TRYLIMIT 64		/* exception stack size */
#define JS_GCLIMIT 10000	/* run gc cycle every N allocations */

/* noreturn is a GCC extension */
#ifdef __GNUC__
#define JS_NORETURN __attribute__((noreturn))
#else
#ifdef _MSC_VER
#define JS_NORETURN __declspec(noreturn)
#else
#define JS_NORETURN
#endif
#endif

/* GCC can do type checking of printf strings */
#ifdef __printflike
#define JS_PRINTFLIKE __printflike
#else
#if __GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ >= 7
#define JS_PRINTFLIKE(fmtarg, firstvararg) \
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#else
#define JS_PRINTFLIKE(fmtarg, firstvararg)
#endif
#endif

#endif
