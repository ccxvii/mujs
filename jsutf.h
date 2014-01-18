#ifndef js_utf_h
#define js_utf_h

typedef unsigned short Rune;	/* 16 bits */

#define chartorune	jsU_chartorune
#define fullrune	jsU_fullrune
#define isalpharune	jsU_isalpharune
#define islowerrune	jsU_islowerrune
#define isspacerune	jsU_isspacerune
#define istitlerune	jsU_istitlerune
#define isupperrune	jsU_isupperrune
#define runelen		jsU_runelen
#define runenlen	jsU_runenlen
#define runestrcat	jsU_runestrcat
#define runestrchr	jsU_runestrchr
#define runestrcmp	jsU_runestrcmp
#define runestrcpy	jsU_runestrcpy
#define runestrdup	jsU_runestrdup
#define runestrecpy	jsU_runestrecpy
#define runestrlen	jsU_runestrlen
#define runestrncat	jsU_runestrncat
#define runestrncmp	jsU_runestrncmp
#define runestrncpy	jsU_runestrncpy
#define runestrrchr	jsU_runestrrchr
#define runestrstr	jsU_runestrstr
#define runetochar	jsU_runetochar
#define tolowerrune	jsU_tolowerrune
#define totitlerune	jsU_totitlerune
#define toupperrune	jsU_toupperrune
#define utfecpy		jsU_utfecpy
#define utflen		jsU_utflen
#define utfnlen		jsU_utfnlen
#define utfrrune	jsU_utfrrune
#define utfrune		jsU_utfrune
#define utfutf		jsU_utfutf

enum
{
	UTFmax		= 3,		/* maximum bytes per rune */
	Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
	Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
	Runeerror	= 0xFFFD,	/* decoding error in UTF */
};

int		chartorune(Rune *rune, char *str);
int		fullrune(char *str, int n);
int		isalpharune(Rune c);
int		islowerrune(Rune c);
int		isspacerune(Rune c);
int		istitlerune(Rune c);
int		isupperrune(Rune c);
int		runelen(long c);
int		runenlen(Rune *r, int nrune);
Rune*		runestrcat(Rune *s1, Rune *s2);
Rune*		runestrchr(Rune *s, Rune c);
int		runestrcmp(Rune *s1, Rune *s2);
Rune*		runestrcpy(Rune *s1, Rune *s2);
Rune*		runestrdup(Rune *s) ;
Rune*		runestrecpy(Rune *s1, Rune *es1, Rune *s2);
long		runestrlen(Rune *s);
Rune*		runestrncat(Rune *s1, Rune *s2, long n);
int		runestrncmp(Rune *s1, Rune *s2, long n);
Rune*		runestrncpy(Rune *s1, Rune *s2, long n);
Rune*		runestrrchr(Rune *s, Rune c);
Rune*		runestrstr(Rune *s1, Rune *s2);
int		runetochar(char *str, Rune *rune);
Rune		tolowerrune(Rune c);
Rune		totitlerune(Rune c);
Rune		toupperrune(Rune c);
char*		utfecpy(char *to, char *e, char *from);
int		utflen(char *s);
int		utfnlen(char *s, long m);
char*		utfrrune(char *s, long c);
char*		utfrune(char *s, long c);
char*		utfutf(char *s1, char *s2);

#endif
