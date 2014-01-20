/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE
 * ANY REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
#include <stdlib.h>
#include <string.h>

#include "jsutf.h"

#define uchar jsU_uchar

typedef unsigned char uchar;

enum
{
	Bit1	= 7,
	Bitx	= 6,
	Bit2	= 5,
	Bit3	= 4,
	Bit4	= 3,

	T1	= ((1<<(Bit1+1))-1) ^ 0xFF,	/* 0000 0000 */
	Tx	= ((1<<(Bitx+1))-1) ^ 0xFF,	/* 1000 0000 */
	T2	= ((1<<(Bit2+1))-1) ^ 0xFF,	/* 1100 0000 */
	T3	= ((1<<(Bit3+1))-1) ^ 0xFF,	/* 1110 0000 */
	T4	= ((1<<(Bit4+1))-1) ^ 0xFF,	/* 1111 0000 */

	Rune1	= (1<<(Bit1+0*Bitx))-1,		/* 0000 0000 0111 1111 */
	Rune2	= (1<<(Bit2+1*Bitx))-1,		/* 0000 0111 1111 1111 */
	Rune3	= (1<<(Bit3+2*Bitx))-1,		/* 1111 1111 1111 1111 */

	Maskx	= (1<<Bitx)-1,			/* 0011 1111 */
	Testx	= Maskx ^ 0xFF,			/* 1100 0000 */

	Bad	= Runeerror,
};

int
chartorune(Rune *rune, const char *str)
{
	int c, c1, c2;
	long l;

	/*
	 * one character sequence
	 *	00000-0007F => T1
	 */
	c = *(uchar*)str;
	if(c < Tx) {
		*rune = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 */
	c1 = *(uchar*)(str+1) ^ Tx;
	if(c1 & Testx)
		goto bad;
	if(c < T3) {
		if(c < T2)
			goto bad;
		l = ((c << Bitx) | c1) & Rune2;
		if(l <= Rune1)
			goto bad;
		*rune = l;
		return 2;
	}

	/*
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 */
	c2 = *(uchar*)(str+2) ^ Tx;
	if(c2 & Testx)
		goto bad;
	if(c < T4) {
		l = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
		if(l <= Rune2)
			goto bad;
		*rune = l;
		return 3;
	}

	/*
	 * bad decoding
	 */
bad:
	*rune = Bad;
	return 1;
}

int
runetochar(char *str, const Rune *rune)
{
	long c;

	/*
	 * one character sequence
	 *	00000-0007F => 00-7F
	 */
	c = *rune;
	if(c <= Rune1) {
		str[0] = c;
		return 1;
	}

	/*
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 */
	if(c <= Rune2) {
		str[0] = T2 | (c >> 1*Bitx);
		str[1] = Tx | (c & Maskx);
		return 2;
	}

	/*
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 */
	str[0] = T3 |  (c >> 2*Bitx);
	str[1] = Tx | ((c >> 1*Bitx) & Maskx);
	str[2] = Tx |  (c & Maskx);
	return 3;
}

int
runelen(long c)
{
	Rune rune;
	char str[10];

	rune = c;
	return runetochar(str, &rune);
}

int
runenlen(const Rune *r, int nrune)
{
	int nb, c;

	nb = 0;
	while(nrune--) {
		c = *r++;
		if(c <= Rune1)
			nb++;
		else
		if(c <= Rune2)
			nb += 2;
		else
			nb += 3;
	}
	return nb;
}

int
fullrune(const char *str, int n)
{
	int c;

	if(n > 0) {
		c = *(uchar*)str;
		if(c < Tx)
			return 1;
		if(n > 1)
			if(c < T3 || n > 2)
				return 1;
	}
	return 0;
}

Rune*
runestrcat(Rune *s1, const Rune *s2)
{
	runestrcpy(runestrchr(s1, 0), s2);
	return s1;
}

Rune*
runestrchr(const Rune *s, Rune c)
{
	Rune c0 = c;
	Rune c1;

	if(c == 0) {
		while(*s++)
			;
		return (Rune*) s-1;
	}

	while((c1 = *s++))
		if(c1 == c0)
			return (Rune*) s-1;
	return 0;
}

int
runestrcmp(const Rune *s1, const Rune *s2)
{
	Rune c1, c2;

	for(;;) {
		c1 = *s1++;
		c2 = *s2++;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			return 0;
	}
}

Rune*
runestrcpy(Rune *s1, const Rune *s2)
{
	Rune *os1;

	os1 = s1;
	while((*s1++ = *s2++))
		;
	return os1;
}

Rune*
runestrdup(const Rune *s)
{
	Rune *ns;

	ns = malloc(sizeof(Rune)*(runestrlen(s) + 1));
	if(ns == 0)
		return 0;

	return runestrcpy(ns, s);
}

Rune*
runestrecpy(Rune *s1, Rune *es1, const Rune *s2)
{
	if(s1 >= es1)
		return s1;

	while((*s1++ = *s2++)){
		if(s1 == es1){
			*--s1 = '\0';
			break;
		}
	}
	return s1;
}

long
runestrlen(const Rune *s)
{

	return runestrchr(s, 0) - s;
}

Rune*
runestrncat(Rune *s1, const Rune *s2, long n)
{
	Rune *os1;

	os1 = s1;
	s1 = runestrchr(s1, 0);
	while((*s1++ = *s2++))
		if(--n < 0) {
			s1[-1] = 0;
			break;
		}
	return os1;
}

int
runestrncmp(const Rune *s1, const Rune *s2, long n)
{
	Rune c1, c2;

	while(n > 0) {
		c1 = *s1++;
		c2 = *s2++;
		n--;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			break;
	}
	return 0;
}

Rune*
runestrncpy(Rune *s1, const Rune *s2, long n)
{
	int i;
	Rune *os1;

	os1 = s1;
	for(i = 0; i < n; i++)
		if((*s1++ = *s2++) == 0) {
			while(++i < n)
				*s1++ = 0;
			return os1;
		}
	return os1;
}

Rune*
runestrrchr(const Rune *s, Rune c)
{
	Rune *r;

	if(c == 0)
		return runestrchr(s, 0);
	r = 0;
	while((s = runestrchr(s, c)))
		r = (Rune*) s++;
	return r;
}

/*
 * Return pointer to first occurrence of s2 in s1,
 * 0 if none
 */
Rune*
runestrstr(const Rune *s1, const Rune *s2)
{
	const Rune *p, *pa, *pb;
	int c0, c;

	c0 = *s2;
	if(c0 == 0)
		return (Rune*) s1;
	s2++;
	for(p=runestrchr(s1, c0); p; p=runestrchr(p+1, c0)) {
		pa = p;
		for(pb=s2;; pb++) {
			c = *pb;
			if(c == 0)
				return (Rune*) p;
			if(c != *++pa)
				break;
		}
	}
	return 0;
}

char*
utfecpy(char *to, char *e, const char *from)
{
	char *end;

	if(to >= e)
		return to;
	end = memccpy(to, from, '\0', e - to);
	if(end == 0){
		end = e-1;
		while(end>to && (*--end&0xC0)==0x80)
			;
		*end = '\0';
	}else{
		end--;
	}
	return end;
}

int
utflen(const char *s)
{
	int c;
	long n;
	Rune rune;

	n = 0;
	for(;;) {
		c = *(uchar*)s;
		if(c < Runeself) {
			if(c == 0)
				return n;
			s++;
		} else
			s += chartorune(&rune, s);
		n++;
	}
}

int
utfnlen(const char *s, long m)
{
	int c;
	long n;
	Rune rune;
	const char *es;

	es = s + m;
	for(n = 0; s < es; n++) {
		c = *(uchar*)s;
		if(c < Runeself){
			if(c == '\0')
				break;
			s++;
			continue;
		}
		if(!fullrune(s, es-s))
			break;
		s += chartorune(&rune, s);
	}
	return n;
}

char*
utfrrune(const char *s, long c)
{
	long c1;
	Rune r;
	const char *s1;

	if(c < Runesync)		/* not part of utf sequence */
		return strrchr(s, c);

	s1 = 0;
	for(;;) {
		c1 = *(uchar*)s;
		if(c1 < Runeself) {	/* one byte rune */
			if(c1 == 0)
				return (char*) s1;
			if(c1 == c)
				s1 = s;
			s++;
			continue;
		}
		c1 = chartorune(&r, s);
		if(r == c)
			s1 = s;
		s += c1;
	}
}

char*
utfrune(const char *s, long c)
{
	long c1;
	Rune r;
	int n;

	if(c < Runesync)		/* not part of utf sequence */
		return strchr(s, c);

	for(;;) {
		c1 = *(uchar*)s;
		if(c1 < Runeself) {	/* one byte rune */
			if(c1 == 0)
				return 0;
			if(c1 == c)
				return (char*) s;
			s++;
			continue;
		}
		n = chartorune(&r, s);
		if(r == c)
			return (char*) s;
		s += n;
	}
}

/*
 * Return pointer to first occurrence of s2 in s1,
 * 0 if none
 */
char*
utfutf(const char *s1, const char *s2)
{
	const char *p;
	long f, n1, n2;
	Rune r;

	n1 = chartorune(&r, s2);
	f = r;
	if(f <= Runesync)		/* represents self */
		return strstr(s1, s2);

	n2 = strlen(s2);
	for(p=s1; (p=utfrune(p, f)); p+=n1)
		if(strncmp(p, s2, n2) == 0)
			return (char*) p;
	return 0;
}
