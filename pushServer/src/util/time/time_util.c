#include "time_util.h"
#include "../string/string_util.h"

// 返回当前时间
struct tm*
GetCurrentTime(struct tm *tm) {
    time_t              t = 0;

    t = time((time_t*) NULL);
    memcpy(tm, localtime(&t), sizeof(struct tm));

    return tm;
}

// time_t转换为tm
struct tm*
ExchangeTime(const time_t *timer) {
	return localtime(timer);
}

// 返回精确的相对时间
struct timeval*
GetTimeOfDay(struct timeval *tv) {
    gettimeofday(tv, (struct timezone*) NULL);
    return tv;
}

// 返回相差秒数(time1 - time0)
double
DiffTimeSecond(time_t time1, time_t time0)
{
	return difftime(time1, time0);
}

// 返回相差毫秒数
float
DiffMillisecond(const struct timeval *tv1, const struct timeval *tv2) {
    float   diff = 0;

    diff = (tv2->tv_sec - tv1->tv_sec) * 1000;
    diff += (float) (tv2->tv_usec - tv1->tv_usec) / 1000;

    return diff;
}

// 返回相差微秒数
LLONG
DiffMicrosecond(const struct timeval *tv1, const struct timeval *tv2) {
    LLONG   diff = 0;

    diff = (LLONG) (tv2->tv_sec - tv1->tv_sec) * 1000000;
    diff += tv2->tv_usec - tv1->tv_usec;

    return diff;
}

// 返回形如"yyyymmdd"的四位年份的八位日期字符串
char*
GetDate(char *buf) {
    struct tm tm;
    return FormatTime(buf, GetCurrentTime(&tm), TIME_FORMAT_DATE);
}

// 返回"yyyymmdd"的八位日期
int
GetIntDate(const time_t *timer)
{
	char buf[16] = {0};
	struct tm *pChangeTm = ExchangeTime(timer);
	FormatTime(buf, pChangeTm, TIME_FORMAT_DATE);
	return atoi(buf);
}

// 格式化日期/时间值
char*
FormatTime(char *buf, const struct tm *tm1, const char *format) {
    strftime(buf, 31, format, tm1);
    return buf;
}

// 返回形如"hhmmss"的六位时间字符串
char*
GetTime(char *buf) {
    struct tm tm;
    return FormatTime(buf, GetCurrentTime(&tm), TIME_FORMAT_TIME);
}

// 返回"hhmmss"六位时间整数值
int
GetIntTime(const time_t *timer)
{
	char buf[8] = {0};
	struct tm *pChangeTm = ExchangeTime(timer);
	FormatTime(buf, pChangeTm, TIME_FORMAT_TIME);
	return atoi(buf);
}

// 返回形如"yyyymmddhhmmss"的十四位时间戳字符串
char*
GetTimestamp(char *buf) {
    struct tm tm;
    return FormatTime(buf, GetCurrentTime(&tm), TIME_FORMAT_TIMESTAMP);
}

// 返回年份
int
GetYear(const struct tm *tm1) {
    return tm1->tm_year + 1900;
}

// 返回月份
int
GetMonth(const struct tm *tm1) {
    return tm1->tm_mon + 1;
}

// 返回日期(本月第几日)
int
GetDay(const struct tm *tm1) {
    return tm1->tm_mday;
}

// 返回小时数
int
GetHour(const struct tm *tm1) {
    return tm1->tm_hour;
}

// 返回分钟数
int
GetMinute(const struct tm *tm1) {
    return tm1->tm_min;
}

// 返回秒数
int
GetSecond(const struct tm *tm1) {
    return tm1->tm_sec;
}

// 校验字符串是否是有效的日期型字符串
BOOL
IsValidDate(const char *s, const char *format) {
    struct tm   tm;
    char        buf[128];
    char        *ptr;

    if (s == (const char*) NULL) {
        return FALSE;
    }

    strncpy(buf, s, sizeof(buf));
    ptr = (char*) TrimRude(buf);

    if (strlen(ptr) < 1) {
        return FALSE;
    }

    ptr = (char*) strftime(buf, sizeof(buf), format, &tm);
    return ptr != NULL ? TRUE : FALSE;
}

// 判断是否是闰年
BOOL
IsLeapYear(int year){
    return (year % 4 == 0 && year % 100 == 0) || year % 400 == 0 ? TRUE : FALSE;
}

time_t ExchangeSixTimeToTime_t(const int TimeValue) {
	time_t lt = time(NULL);
	struct tm * pTm = localtime(&lt);
	pTm->tm_sec = TimeValue % 100;
	pTm->tm_min = (TimeValue -  TimeValue / 10000 * 10000 - TimeValue % 100) / 100;
	pTm->tm_hour = TimeValue / 10000;
	lt = mktime(pTm);
	return lt;
}

