#ifndef _TIME_UTIL_H
#define _TIME_UTIL_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../common/common_types.h"
#include "../util.h"


// ����/ʱ���ʽ����
#define     TIME_FORMAT_DATE                    "%Y%m%d"
#define     TIME_FORMAT_FORMATTED_DATE          "%Y-%m-%d"
#define     TIME_FORMAT_SHORT_DATE              "%y%m%d"
#define     TIME_FORMAT_FORMATTED_SHORT_DATE    "%y-%m-%d"
#define     TIME_FORMAT_TIME                    "%H%M%S"
#define     TIME_FORMAT_FORMATTED_TIME          "%H:%M:%S"
#define     TIME_FORMAT_TIMESTAMP               "%Y%m%d%H%M%S"
#define     TIME_FORMAT_FORMATTED_TIMESTAMP     "%Y-%m-%d %H:%M:%S"
#define		TIME_FORMAT_FORMATTED_MD			"%m-%d"
#define		TIME_FORMAT_FORMATTED_MDHM			"%m-%d %H:%M"
//---------------------------------------------------					/

struct tm*      GetCurrentTime(struct tm *tm);                                  /* ���ص�ǰʱ�� */
struct tm*		ExchangeTime(const time_t *timer);								/* time_tת��Ϊtm */

struct timeval* GetTimeOfDay(struct timeval *tv);                               /* ���ؾ�ȷ�����ʱ�� */
float           DiffMillisecond(const struct timeval*, const struct timeval*);  /* ������������ */
LLONG           DiffMicrosecond(const struct timeval*, const struct timeval*);  /* �������΢���� */
double 			DiffTimeSecond(time_t time1, time_t time0);						// �����������

char*           FormatTime(char *, const struct tm*, const char*);              /* ��ʽ������/ʱ��ֵ */
char*           GetDate(char *);                                              	/* ��������"yyyymmdd"����λ��ݵİ�λ�����ַ��� */
int				GetIntDate(const time_t *timer);								// ����"yyyymmdd"�İ�λ����

char*           GetTime(char *);                                              	/* ��������"hhmmss"����λʱ���ַ��� */
char*           GetTimestamp(char *);                                         	/* ��������"yyyymmddhhmmss"��ʮ��λʱ����ַ��� */
int				GetIntTime(const time_t *timer);								// ����"hhmmss"��λʱ������ֵ

int             GetYear(const struct tm *);                                 	/* ������� */
int             GetMonth(const struct tm *);                                	/* �����·� */
int             GetDay(const struct tm *);                                  	/* ��������(���µڼ���) */
int             GetHour(const struct tm *);                                 	/* ����Сʱ�� */
int             GetMinute(const struct tm *);                               	/* ���ط����� */
int             GetSecond(const struct tm *);                               	/* �������� */

BOOL            IsValidDate(const char*, const char*);                      	/* У���ַ����Ƿ�����Ч���������ַ��� */
BOOL            IsLeapYear(int);                                            	/* �ж��Ƿ�������*/

time_t ExchangeSixTimeToTime_t(const int);										/* ����λ��ʽ��ʱ��תΪ��Ӧ��time_tֵ */

#ifdef __cplusplus
}
#endif

#endif  /* _TIME_UTIL_H */

