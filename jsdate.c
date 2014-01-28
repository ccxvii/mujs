#include "jsi.h"
#include "jsvalue.h"
#include "jsbuiltin.h"

#include <time.h>

static double Now(void)
{
	time_t now = time(NULL);
	return mktime(gmtime(&now)) * 1000;
}

static double LocalTZA(void)
{
	static int once = 1;
	static double tza = 0;
	if (once) {
		time_t now = time(NULL);
		time_t utc = mktime(gmtime(&now));
		time_t loc = mktime(localtime(&now));
		tza = (loc - utc) * 1000;
		once = 0;
	}
	return tza;
}

static double DaylightSavingTA(double t)
{
	return 0; // TODO
}

/* Helpers from the ECMA 262 specification */

#define HoursPerDay		24.0
#define MinutesPerDay		(HoursPerDay * MinutesPerHour)
#define MinutesPerHour		60.0
#define SecondsPerDay		(MinutesPerDay * SecondsPerMinute)
#define SecondsPerHour		(MinutesPerHour * SecondsPerMinute)
#define SecondsPerMinute	60.0

#define msPerDay	(SecondsPerDay * msPerSecond)
#define msPerHour	(SecondsPerHour * msPerSecond)
#define msPerMinute	(SecondsPerMinute * msPerSecond)
#define msPerSecond	1000.0

static int Day(double t)
{
	return floor(t / msPerDay);
}

static double TimeWithinDay(double t)
{
	return fmod(t, msPerDay);
}

static int DaysInYear(int y)
{
	return y % 4 == 0 && (y % 100 || (y % 400 == 0)) ? 366 : 365;
}

static int DayFromYear(int y)
{
	return 365 * (y - 1970) +
		floor((y - 1969) / 4.0) -
		floor((y - 1901) / 100.0) +
		floor((y - 1601) / 400.0);
}

static double TimeFromYear(int y)
{
	return DayFromYear(y) * msPerDay;
}

static int YearFromTime(double t)
{
	int y = floor(t / (msPerDay * 365.2425)) + 1970;
	double t2 = TimeFromYear(y);
	if (t2 > t)
		--y;
	else if (t2 + msPerDay * DaysInYear(y) <= t)
		++y;
	return y;
}

static int InLeapYear(int t)
{
	return DaysInYear(YearFromTime(t)) == 366;
}

static int DayWithinYear(double t)
{
	return Day(t) - DayFromYear(YearFromTime(t));
}

static int MonthFromTime(double t)
{
	int day = DayWithinYear(t);
	int leap = InLeapYear(t);
	if (day < 31) return 0;
	if (day < 59 + leap) return 1;
	if (day < 90 + leap) return 2;
	if (day < 120 + leap) return 3;
	if (day < 151 + leap) return 4;
	if (day < 181 + leap) return 5;
	if (day < 212 + leap) return 6;
	if (day < 243 + leap) return 7;
	if (day < 273 + leap) return 8;
	if (day < 304 + leap) return 9;
	if (day < 334 + leap) return 10;
	return 11;
}

static int DateFromTime(double t)
{
	int day = DayWithinYear(t);
	int leap = InLeapYear(t);
	switch (MonthFromTime(t)) {
	case 0: return day + 1;
	case 1: return day - 30;
	case 2: return day - 58 - leap;
	case 3: return day - 89 - leap;
	case 4: return day - 119 - leap;
	case 5: return day - 150 - leap;
	case 6: return day - 180 - leap;
	case 7: return day - 211 - leap;
	case 8: return day - 242 - leap;
	case 9: return day - 272 - leap;
	case 10: return day - 303 - leap;
	default : return day - 333 - leap;
	}
}

static int WeekDay(double t)
{
	return (Day(t) + 4) % 7;
}

static double LocalTime(double utc)
{
	return utc + LocalTZA() + DaylightSavingTA(utc);
}

static double UTC(double loc)
{
	return loc - LocalTZA() - DaylightSavingTA(loc - LocalTZA());
}

static int HourFromTime(double t)
{
	return fmod(floor(t / msPerHour), HoursPerDay);
}

static int MinFromTime(double t)
{
	return fmod(floor(t / msPerMinute), MinutesPerHour);
}

static int SecFromTime(double t)
{
	return fmod(floor(t / msPerSecond), SecondsPerMinute);
}

static int msFromTime(double t)
{
	return fmod(t, msPerSecond);
}

static double MakeTime(double hour, double min, double sec, double ms)
{
	return ((hour * MinutesPerHour + min) * SecondsPerMinute + sec) * msPerSecond + ms;
}

static double MakeDay(double y, double m, double date)
{
	/*
	 * The following array contains the day of year for the first day of
	 * each month, where index 0 is January, and day 0 is January 1.
	 */
	static const double firstDayOfMonth[2][12] = {
		{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
		{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
	};

	double yd, md;

	y += floor(m / 12);
	m = fmod(m, 12);
	if (m < 0) m += 12;

	yd = floor(TimeFromYear(y) / msPerDay);
	md = firstDayOfMonth[InLeapYear(y)][(int)m];

	return yd + md + date - 1;
}

static double MakeDate(double day, double time)
{
	return day * msPerDay + time;
}

static double TimeClip(double t)
{
	if (!isfinite(t))
		return NAN;
	if (abs(t) > 8.64e15)
		return NAN;
	return t < 0 ? -floor(-t) : floor(t);
}

static double parseDate(const char *str)
{
	const char *tz;
	int y = 1970, m = 0, d = 1, H = 0, M = 0, S = 0, ms = 0;
	double t;
	sscanf(str, "%04d-%02d-%02d%*[T ]%02d:%02d:%02d.%d", &y, &m, &d, &H, &M, &S, &ms);
	t = MakeDate(MakeDay(y, m-1, d), MakeTime(H, M, S, ms));
	if ((tz = strstr(str, "UTC")) && !strcmp(tz, "UTC")) return t;
	if ((tz = strstr(str, "GMT")) && !strcmp(tz, "GMT")) return t;
	if ((tz = strstr(str, "Z")) && !strcmp(tz, "Z")) return t;
	return UTC(t);
}

/* strftime based date formatting */

#define FMT_DATE "%Y-%m-%d"
#define FMT_TIME "%H:%M:%S"
#define FMT_DATETIME "%Y-%m-%d %H:%M:%S %Z"
#define FMT_DATETIME_ISO "%Y-%m-%dT%H:%M:%SZ"

static const char *fmtlocal(const char *fmt, double t)
{
	static char buf[64];
	time_t tt = t * 0.001;
	strftime(buf, sizeof buf, fmt, localtime(&tt));
	return buf;
}

static const char *fmtutc(const char *fmt, double t)
{
	static char buf[64];
	time_t tt = t * 0.001;
	strftime(buf, sizeof buf, fmt, gmtime(&tt));
	return buf;
}

/* Date functions */

static double js_todate(js_State *J, int idx)
{
	js_Object *self = js_toobject(J, idx);
	if (self->type != JS_CDATE)
		js_typeerror(J, "not a date");
	return self->u.number;
}

static int js_setdate(js_State *J, int idx, double t)
{
	js_Object *self = js_toobject(J, idx);
	if (self->type != JS_CDATE)
		js_typeerror(J, "not a date");
	self->u.number = TimeClip(t);
	js_pushnumber(J, self->u.number);
	return 1;
}

static int D_parse(js_State *J, int argc)
{
	double t = parseDate(js_tostring(J, 1));
	js_pushnumber(J, t);
	return 1;
}

static int D_UTC(js_State *J, int argc)
{
	double y, m, d, H, M, S, ms, t;
	y = js_tonumber(J, 1);
	if (y < 100) y += 1900;
	m = js_tonumber(J, 2);
	d = argc > 2 ? js_tonumber(J, 3) : 1;
	H = argc > 3 ? js_tonumber(J, 4) : 0;
	M = argc > 4 ? js_tonumber(J, 5) : 0;
	S = argc > 5 ? js_tonumber(J, 6) : 0;
	ms = argc > 6 ? js_tonumber(J, 7) : 0;
	t = MakeDate(MakeDay(y, m, d), MakeTime(H, M, S, ms));
	t = TimeClip(t);
	js_pushnumber(J, t);
	return 1;
}

static int D_now(js_State *J, int argc)
{
	js_pushnumber(J, Now());
	return 1;
}

static int jsB_Date(js_State *J, int argc)
{
	js_pushstring(J, fmtlocal(FMT_DATETIME, Now()));
	return 1;
}

static int jsB_new_Date(js_State *J, int argc)
{
	js_Object *obj;
	js_Value v;
	double t;

	if (argc == 0)
		t = Now();
	else if (argc == 1) {
		v = js_toprimitive(J, 1, JS_HNONE);
		if (v.type == JS_TSTRING)
			t = parseDate(v.u.string);
		else
			t = TimeClip(jsV_tonumber(J, &v));
	} else {
		double y, m, d, H, M, S, ms;
		y = js_tonumber(J, 1);
		if (y < 100) y += 1900;
		m = js_tonumber(J, 2);
		d = argc > 2 ? js_tonumber(J, 3) : 1;
		H = argc > 3 ? js_tonumber(J, 4) : 0;
		M = argc > 4 ? js_tonumber(J, 5) : 0;
		S = argc > 5 ? js_tonumber(J, 6) : 0;
		ms = argc > 6 ? js_tonumber(J, 7) : 0;
		t = MakeDate(MakeDay(y, m, d), MakeTime(H, M, S, ms));
		t = TimeClip(UTC(t));
	}

	obj = jsV_newobject(J, JS_CDATE, J->Date_prototype);
	obj->u.number = t;

	js_pushobject(J, obj);
	return 1;
}

static int Dp_valueOf(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, t);
	return 1;
}

static int Dp_toString(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushstring(J, fmtlocal(FMT_DATETIME, t));
	return 1;
}

static int Dp_toDateString(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushstring(J, fmtlocal(FMT_DATE, t));
	return 1;
}

static int Dp_toTimeString(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushstring(J, fmtlocal(FMT_TIME, t));
	return 1;
}

static int Dp_toUTCString(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushstring(J, fmtutc(FMT_DATETIME, t));
	return 1;
}

static int Dp_toISOString(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushstring(J, fmtutc(FMT_DATETIME_ISO, t));
	return 1;
}

static int Dp_getFullYear(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, YearFromTime(LocalTime(t)));
	return 1;
}

static int Dp_getMonth(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, MonthFromTime(LocalTime(t)));
	return 1;
}

static int Dp_getDate(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, DateFromTime(LocalTime(t)));
	return 1;
}

static int Dp_getDay(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, WeekDay(LocalTime(t)));
	return 1;
}

static int Dp_getHours(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, HourFromTime(LocalTime(t)));
	return 1;
}

static int Dp_getMinutes(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, MinFromTime(LocalTime(t)));
	return 1;
}

static int Dp_getSeconds(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, SecFromTime(LocalTime(t)));
	return 1;
}

static int Dp_getMilliseconds(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, msFromTime(LocalTime(t)));
	return 1;
}

static int Dp_getUTCFullYear(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, YearFromTime(t));
	return 1;
}

static int Dp_getUTCMonth(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, MonthFromTime(t));
	return 1;
}

static int Dp_getUTCDate(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, DateFromTime(t));
	return 1;
}

static int Dp_getUTCDay(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, WeekDay(t));
	return 1;
}

static int Dp_getUTCHours(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, HourFromTime(t));
	return 1;
}

static int Dp_getUTCMinutes(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, MinFromTime(t));
	return 1;
}

static int Dp_getUTCSeconds(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, SecFromTime(t));
	return 1;
}

static int Dp_getUTCMilliseconds(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, msFromTime(t));
	return 1;
}

static int Dp_getTimezoneOffset(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	js_pushnumber(J, (t - LocalTime(t)) / msPerMinute);
	return 1;
}

static int Dp_setTime(js_State *J, int argc)
{
	return js_setdate(J, 0, js_tonumber(J, 1));
}

static int Dp_setMilliseconds(js_State *J, int argc)
{
	double t = LocalTime(js_todate(J, 0));
	double h = HourFromTime(t);
	double m = MinFromTime(t);
	double s = SecFromTime(t);
	double ms = js_tonumber(J, 1);
	return js_setdate(J, 0, UTC(MakeDate(Day(t), MakeTime(h, m, s, ms))));
}

static int Dp_setSeconds(js_State *J, int argc)
{
	double t = LocalTime(js_todate(J, 0));
	double h = HourFromTime(t);
	double m = MinFromTime(t);
	double s = js_tonumber(J, 1);
	double ms = argc > 1 ? js_tonumber(J, 2) : msFromTime(t);
	return js_setdate(J, 0, UTC(MakeDate(Day(t), MakeTime(h, m, s, ms))));
}

static int Dp_setMinutes(js_State *J, int argc)
{
	double t = LocalTime(js_todate(J, 0));
	double h = HourFromTime(t);
	double m = js_tonumber(J, 1);
	double s = argc > 1 ? js_tonumber(J, 2) : SecFromTime(t);
	double ms = argc > 2 ? js_tonumber(J, 3) : msFromTime(t);
	return js_setdate(J, 0, UTC(MakeDate(Day(t), MakeTime(h, m, s, ms))));
}

static int Dp_setHours(js_State *J, int argc)
{
	double t = LocalTime(js_todate(J, 0));
	double h = js_tonumber(J, 1);
	double m = argc > 1 ? js_tonumber(J, 2) : HourFromTime(t);
	double s = argc > 2 ? js_tonumber(J, 3) : SecFromTime(t);
	double ms = argc > 3 ? js_tonumber(J, 4) : msFromTime(t);
	return js_setdate(J, 0, UTC(MakeDate(Day(t), MakeTime(h, m, s, ms))));
}

static int Dp_setDate(js_State *J, int argc)
{
	double t = LocalTime(js_todate(J, 0));
	double y = YearFromTime(t);
	double m = MonthFromTime(t);
	double d = js_tonumber(J, 1);
	return js_setdate(J, 0, UTC(MakeDate(MakeDay(y, m, d), TimeWithinDay(t))));
}

static int Dp_setMonth(js_State *J, int argc)
{
	double t = LocalTime(js_todate(J, 0));
	double y = YearFromTime(t);
	double m = js_tonumber(J, 1);
	double d = argc > 1 ? js_tonumber(J, 3) : DateFromTime(t);
	return js_setdate(J, 0, UTC(MakeDate(MakeDay(y, m, d), TimeWithinDay(t))));
}

static int Dp_setFullYear(js_State *J, int argc)
{
	double t = LocalTime(js_todate(J, 0));
	double y = js_tonumber(J, 1);
	double m = argc > 1 ? js_tonumber(J, 2) : MonthFromTime(t);
	double d = argc > 2 ? js_tonumber(J, 3) : DateFromTime(t);
	return js_setdate(J, 0, UTC(MakeDate(MakeDay(y, m, d), TimeWithinDay(t))));
}

static int Dp_setUTCMilliseconds(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	double h = HourFromTime(t);
	double m = MinFromTime(t);
	double s = SecFromTime(t);
	double ms = js_tonumber(J, 1);
	return js_setdate(J, 0, MakeDate(Day(t), MakeTime(h, m, s, ms)));
}

static int Dp_setUTCSeconds(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	double h = HourFromTime(t);
	double m = MinFromTime(t);
	double s = js_tonumber(J, 1);
	double ms = argc > 1 ? js_tonumber(J, 2) : msFromTime(t);
	return js_setdate(J, 0, MakeDate(Day(t), MakeTime(h, m, s, ms)));
}

static int Dp_setUTCMinutes(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	double h = HourFromTime(t);
	double m = js_tonumber(J, 1);
	double s = argc > 1 ? js_tonumber(J, 2) : SecFromTime(t);
	double ms = argc > 2 ? js_tonumber(J, 3) : msFromTime(t);
	return js_setdate(J, 0, MakeDate(Day(t), MakeTime(h, m, s, ms)));
}

static int Dp_setUTCHours(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	double h = js_tonumber(J, 1);
	double m = argc > 1 ? js_tonumber(J, 2) : HourFromTime(t);
	double s = argc > 2 ? js_tonumber(J, 3) : SecFromTime(t);
	double ms = argc > 3 ? js_tonumber(J, 4) : msFromTime(t);
	return js_setdate(J, 0, MakeDate(Day(t), MakeTime(h, m, s, ms)));
}

static int Dp_setUTCDate(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	double y = YearFromTime(t);
	double m = MonthFromTime(t);
	double d = js_tonumber(J, 1);
	return js_setdate(J, 0, MakeDate(MakeDay(y, m, d), TimeWithinDay(t)));
}

static int Dp_setUTCMonth(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	double y = YearFromTime(t);
	double m = js_tonumber(J, 1);
	double d = argc > 1 ? js_tonumber(J, 3) : DateFromTime(t);
	return js_setdate(J, 0, MakeDate(MakeDay(y, m, d), TimeWithinDay(t)));
}

static int Dp_setUTCFullYear(js_State *J, int argc)
{
	double t = js_todate(J, 0);
	double y = js_tonumber(J, 1);
	double m = argc > 1 ? js_tonumber(J, 2) : MonthFromTime(t);
	double d = argc > 2 ? js_tonumber(J, 3) : DateFromTime(t);
	return js_setdate(J, 0, MakeDate(MakeDay(y, m, d), TimeWithinDay(t)));
}

void jsB_initdate(js_State *J)
{
	J->Date_prototype->u.number = 0;

	js_pushobject(J, J->Date_prototype);
	{
		jsB_propf(J, "valueOf", Dp_valueOf, 0);
		jsB_propf(J, "toString", Dp_toString, 0);
		jsB_propf(J, "toDateString", Dp_toDateString, 0);
		jsB_propf(J, "toTimeString", Dp_toTimeString, 0);
		jsB_propf(J, "toLocaleString", Dp_toString, 0);
		jsB_propf(J, "toLocaleDateString", Dp_toDateString, 0);
		jsB_propf(J, "toLocaleTimeString", Dp_toTimeString, 0);
		jsB_propf(J, "toUTCString", Dp_toUTCString, 0);

		jsB_propf(J, "getTime", Dp_valueOf, 0);
		jsB_propf(J, "getFullYear", Dp_getFullYear, 0);
		jsB_propf(J, "getUTCFullYear", Dp_getUTCFullYear, 0);
		jsB_propf(J, "getMonth", Dp_getMonth, 0);
		jsB_propf(J, "getUTCMonth", Dp_getUTCMonth, 0);
		jsB_propf(J, "getDate", Dp_getDate, 0);
		jsB_propf(J, "getUTCDate", Dp_getUTCDate, 0);
		jsB_propf(J, "getDay", Dp_getDay, 0);
		jsB_propf(J, "getUTCDay", Dp_getUTCDay, 0);
		jsB_propf(J, "getHours", Dp_getHours, 0);
		jsB_propf(J, "getUTCHours", Dp_getUTCHours, 0);
		jsB_propf(J, "getMinutes", Dp_getMinutes, 0);
		jsB_propf(J, "getUTCMinutes", Dp_getUTCMinutes, 0);
		jsB_propf(J, "getSeconds", Dp_getSeconds, 0);
		jsB_propf(J, "getUTCSeconds", Dp_getUTCSeconds, 0);
		jsB_propf(J, "getMilliseconds", Dp_getMilliseconds, 0);
		jsB_propf(J, "getUTCMilliseconds", Dp_getUTCMilliseconds, 0);
		jsB_propf(J, "getTimezoneOffset", Dp_getTimezoneOffset, 0);

		jsB_propf(J, "setTime", Dp_setTime, 1);
		jsB_propf(J, "setMilliseconds", Dp_setMilliseconds, 1);
		jsB_propf(J, "setUTCMilliseconds", Dp_setUTCMilliseconds, 1);
		jsB_propf(J, "setSeconds", Dp_setSeconds, 2);
		jsB_propf(J, "setUTCSeconds", Dp_setUTCSeconds, 2);
		jsB_propf(J, "setMinutes", Dp_setMinutes, 3);
		jsB_propf(J, "setUTCMinutes", Dp_setUTCMinutes, 3);
		jsB_propf(J, "setHours", Dp_setHours, 4);
		jsB_propf(J, "setUTCHours", Dp_setUTCHours, 4);
		jsB_propf(J, "setDate", Dp_setDate, 1);
		jsB_propf(J, "setUTCDate", Dp_setUTCDate, 1);
		jsB_propf(J, "setMonth", Dp_setMonth, 2);
		jsB_propf(J, "setUTCMonth", Dp_setUTCMonth, 2);
		jsB_propf(J, "setFullYear", Dp_setFullYear, 3);
		jsB_propf(J, "setUTCFullYear", Dp_setUTCFullYear, 3);

		/* ES5 */
		jsB_propf(J, "toISOString", Dp_toISOString, 0);
	}
	js_newcconstructor(J, jsB_Date, jsB_new_Date, 1);
	{
		jsB_propf(J, "parse", D_parse, 1);
		jsB_propf(J, "UTC", D_UTC, 7);

		/* ES5 */
		jsB_propf(J, "now", D_now, 0);
	}
	js_defglobal(J, "Date", JS_DONTENUM);
}
