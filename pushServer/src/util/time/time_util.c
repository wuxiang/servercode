#include "time_util.h"
#include "../string/string_util.h"

// ���ص�ǰʱ��
struct tm*
GetCurrentTime(struct tm *tm) {
    time_t              t = 0;

    t = time((time_t*) NULL);
    memcpy(tm, localtime(&t), sizeof(struct tm));

    return tm;
}

// time_tת��Ϊtm
struct tm*
ExchangeTime(const time_t *timer) {
	return localtime(timer);
}

// ���ؾ�ȷ�����ʱ��
struct timeval*
GetTimeOfDay(struct timeval *tv) {
    gettimeofday(tv, (struct timezone*) NULL);
    return tv;
}

// �����������(time1 - time0)
double
DiffTimeSecond(time_t time1, time_t time0)
{
	return difftime(time1, time0);
}

// ������������
float
DiffMillisecond(const struct timeval *tv1, const struct timeval *tv2) {
    float   diff = 0;

    diff = (tv2->tv_sec - tv1->tv_sec) * 1000;
    diff += (float) (tv2->tv_usec - tv1->tv_usec) / 1000;

    return diff;
}

// �������΢����
LLONG
DiffMicrosecond(const struct timeval *tv1, const struct timeval *tv2) {
    LLONG   diff = 0;

    diff = (LLONG) (tv2->tv_sec - tv1->tv_sec) * 1000000;
    diff += tv2->tv_usec - tv1->tv_usec;

    return diff;
}

// ��������"yyyymmdd"����λ��ݵİ�λ�����ַ���
char*
GetDate(char *buf) {
    struct tm tm;
    return FormatTime(buf, GetCurrentTime(&tm), TIME_FORMAT_DATE);
}

// ����"yyyymmdd"�İ�λ����
int
GetIntDate(const time_t *timer)
{
	char buf[16] = {0};
	struct tm *pChangeTm = ExchangeTime(timer);
	FormatTime(buf, pChangeTm, TIME_FORMAT_DATE);
	return atoi(buf);
}

// ��ʽ������/ʱ��ֵ
char*
FormatTime(char *buf, const struct tm *tm1, const char *format) {
    strftime(buf, 31, format, tm1);
    return buf;
}

// ��������"hhmmss"����λʱ���ַ���
char*
GetTime(char *buf) {
    struct tm tm;
    return FormatTime(buf, GetCurrentTime(&tm), TIME_FORMAT_TIME);
}

// ����"hhmmss"��λʱ������ֵ
int
GetIntTime(const time_t *timer)
{
	char buf[8] = {0};
	struct tm *pChangeTm = ExchangeTime(timer);
	FormatTime(buf, pChangeTm, TIME_FORMAT_TIME);
	return atoi(buf);
}

// ��������"yyyymmddhhmmss"��ʮ��λʱ����ַ���
char*
GetTimestamp(char *buf) {
    struct tm tm;
    return FormatTime(buf, GetCurrentTime(&tm), TIME_FORMAT_TIMESTAMP);
}

// �������
int
GetYear(const struct tm *tm1) {
    return tm1->tm_year + 1900;
}

// �����·�
int
GetMonth(const struct tm *tm1) {
    return tm1->tm_mon + 1;
}

// ��������(���µڼ���)
int
GetDay(const struct tm *tm1) {
    return tm1->tm_mday;
}

// ����Сʱ��
int
GetHour(const struct tm *tm1) {
    return tm1->tm_hour;
}

// ���ط�����
int
GetMinute(const struct tm *tm1) {
    return tm1->tm_min;
}

// ��������
int
GetSecond(const struct tm *tm1) {
    return tm1->tm_sec;
}

// У���ַ����Ƿ�����Ч���������ַ���
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

// �ж��Ƿ�������
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

