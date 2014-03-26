/* The authors of this software are Rob Pike and Ken Thompson.
 * Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY. IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

enum { NSIGNIF	= 17 };

/*
 * first few powers of 10, enough for about 1/2 of the
 * total space for doubles.
 */
static double pows10[] =
{
	1e0,	1e1,	1e2,	1e3,	1e4,	1e5,	1e6,	1e7,	1e8,	1e9,
	1e10,	1e11,	1e12,	1e13,	1e14,	1e15,	1e16,	1e17,	1e18,	1e19,
	1e20,	1e21,	1e22,	1e23,	1e24,	1e25,	1e26,	1e27,	1e28,	1e29,
	1e30,	1e31,	1e32,	1e33,	1e34,	1e35,	1e36,	1e37,	1e38,	1e39,
	1e40,	1e41,	1e42,	1e43,	1e44,	1e45,	1e46,	1e47,	1e48,	1e49,
	1e50,	1e51,	1e52,	1e53,	1e54,	1e55,	1e56,	1e57,	1e58,	1e59,
	1e60,	1e61,	1e62,	1e63,	1e64,	1e65,	1e66,	1e67,	1e68,	1e69,
	1e70,	1e71,	1e72,	1e73,	1e74,	1e75,	1e76,	1e77,	1e78,	1e79,
	1e80,	1e81,	1e82,	1e83,	1e84,	1e85,	1e86,	1e87,	1e88,	1e89,
	1e90,	1e91,	1e92,	1e93,	1e94,	1e95,	1e96,	1e97,	1e98,	1e99,
	1e100,	1e101,	1e102,	1e103,	1e104,	1e105,	1e106,	1e107,	1e108,	1e109,
	1e110,	1e111,	1e112,	1e113,	1e114,	1e115,	1e116,	1e117,	1e118,	1e119,
	1e120,	1e121,	1e122,	1e123,	1e124,	1e125,	1e126,	1e127,	1e128,	1e129,
	1e130,	1e131,	1e132,	1e133,	1e134,	1e135,	1e136,	1e137,	1e138,	1e139,
	1e140,	1e141,	1e142,	1e143,	1e144,	1e145,	1e146,	1e147,	1e148,	1e149,
	1e150,	1e151,	1e152,	1e153,	1e154,	1e155,	1e156,	1e157,	1e158,	1e159,
};
#define	npows10 ((int)(sizeof(pows10)/sizeof(pows10[0])))
#define	pow10(x)  fmtpow10(x)

static double
pow10(int n)
{
	double d;
	int neg;

	neg = 0;
	if(n < 0){
		neg = 1;
		n = -n;
	}

	if(n < npows10)
		d = pows10[n];
	else{
		d = pows10[npows10-1];
		for(;;){
			n -= npows10 - 1;
			if(n < npows10){
				d *= pows10[n];
				break;
			}
			d *= pows10[npows10 - 1];
		}
	}
	if(neg)
		return 1./d;
	return d;
}

/*
 * add 1 to the decimal integer string a of length n.
 * if 99999 overflows into 10000, return 1 to tell caller
 * to move the virtual decimal point.
 */
static int
xadd1(char *a, int n)
{
	char *b;
	int c;

	if(n < 0 || n > NSIGNIF)
		return 0;
	for(b = a+n-1; b >= a; b--) {
		c = *b + 1;
		if(c <= '9') {
			*b = c;
			return 0;
		}
		*b = '0';
	}
	/*
	 * need to overflow adding digit.
	 * shift number down and insert 1 at beginning.
	 * decimal is known to be 0s or we wouldn't
	 * have gotten this far.  (e.g., 99999+1 => 00000)
	 */
	a[0] = '1';
	return 1;
}

/*
 * subtract 1 from the decimal integer string a.
 * if 10000 underflows into 09999, make it 99999
 * and return 1 to tell caller to move the virtual
 * decimal point.  this way, xsub1 is inverse of xadd1.
 */
static int
xsub1(char *a, int n)
{
	char *b;
	int c;

	if(n < 0 || n > NSIGNIF)
		return 0;
	for(b = a+n-1; b >= a; b--) {
		c = *b - 1;
		if(c >= '0') {
			if(c == '0' && b == a) {
				/*
				 * just zeroed the top digit; shift everyone up.
				 * decimal is known to be 9s or we wouldn't
				 * have gotten this far.  (e.g., 10000-1 => 09999)
				 */
				*b = '9';
				return 1;
			}
			*b = c;
			return 0;
		}
		*b = '9';
	}
	/*
	 * can't get here.  the number a is always normalized
	 * so that it has a nonzero first digit.
	 */
	return 0;
}

/*
 * format exponent like sprintf(p, "e%+d", e)
 */
void
jsV_fmtexp(char *p, int e)
{
	char se[9];
	int i;

	*p++ = 'e';
	if(e < 0) {
		*p++ = '-';
		e = -e;
	} else
		*p++ = '+';
	i = 0;
	while(e) {
		se[i++] = e % 10 + '0';
		e /= 10;
	}
	while(i < 1)
		se[i++] = '0';
	while(i > 0)
		*p++ = se[--i];
	*p++ = '\0';
}

/*
 * compute decimal integer m, exp such that:
 *	f = m*10^exp
 *	m is as short as possible with losing exactness
 * assumes special cases (NaN, +Inf, -Inf) have been handled.
 */
void
jsV_dtoa(double f, char *s, int *exp, int *neg, int *ns)
{
	int c, d, e2, e, ee, i, ndigit, oerrno;
	char tmp[NSIGNIF+10];
	double g;

	oerrno = errno; /* in case strtod smashes errno */

	/*
	 * make f non-negative.
	 */
	*neg = 0;
	if(f < 0) {
		f = -f;
		*neg = 1;
	}

	/*
	 * must handle zero specially.
	 */
	if(f == 0){
		*exp = 0;
		s[0] = '0';
		s[1] = '\0';
		*ns = 1;
		return;
	}

	/*
	 * find g,e such that f = g*10^e.
	 * guess 10-exponent using 2-exponent, then fine tune.
	 */
	frexp(f, &e2);
	e = (int)(e2 * .301029995664);
	g = f * pow10(-e);
	while(g < 1) {
		e--;
		g = f * pow10(-e);
	}
	while(g >= 10) {
		e++;
		g = f * pow10(-e);
	}

	/*
	 * convert NSIGNIF digits as a first approximation.
	 */
	for(i=0; i<NSIGNIF; i++) {
		d = (int)g;
		s[i] = d+'0';
		g = (g-d) * 10;
	}
	s[i] = 0;

	/*
	 * adjust e because s is 314159... not 3.14159...
	 */
	e -= NSIGNIF-1;
	jsV_fmtexp(s+NSIGNIF, e);

	/*
	 * adjust conversion until strtod(s) == f exactly.
	 */
	for(i=0; i<10; i++) {
		g = strtod(s, NULL);
		if(f > g) {
			if(xadd1(s, NSIGNIF)) {
				/* gained a digit */
				e--;
				jsV_fmtexp(s+NSIGNIF, e);
			}
			continue;
		}
		if(f < g) {
			if(xsub1(s, NSIGNIF)) {
				/* lost a digit */
				e++;
				jsV_fmtexp(s+NSIGNIF, e);
			}
			continue;
		}
		break;
	}

	/*
	 * play with the decimal to try to simplify.
	 */

	/*
	 * bump last few digits up to 9 if we can
	 */
	for(i=NSIGNIF-1; i>=NSIGNIF-3; i--) {
		c = s[i];
		if(c != '9') {
			s[i] = '9';
			g = strtod(s, NULL);
			if(g != f) {
				s[i] = c;
				break;
			}
		}
	}

	/*
	 * add 1 in hopes of turning 9s to 0s
	 */
	if(s[NSIGNIF-1] == '9') {
		strcpy(tmp, s);
		ee = e;
		if(xadd1(tmp, NSIGNIF)) {
			ee--;
			jsV_fmtexp(tmp+NSIGNIF, ee);
		}
		g = strtod(tmp, NULL);
		if(g == f) {
			strcpy(s, tmp);
			e = ee;
		}
	}

	/*
	 * bump last few digits down to 0 as we can.
	 */
	for(i=NSIGNIF-1; i>=NSIGNIF-3; i--) {
		c = s[i];
		if(c != '0') {
			s[i] = '0';
			g = strtod(s, NULL);
			if(g != f) {
				s[i] = c;
				break;
			}
		}
	}

	/*
	 * remove trailing zeros.
	 */
	ndigit = NSIGNIF;
	while(ndigit > 1 && s[ndigit-1] == '0'){
		e++;
		--ndigit;
	}
	s[ndigit] = 0;
	*exp = e;
	*ns = ndigit;
	errno = oerrno;
}
