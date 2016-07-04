#ifndef _TIME_UTIL_H
#define _TIME_UTIL_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../common/common_types.h"
#include "../util.h"


// 日期/时间格式定义
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

struct tm*      GetCurrentTime(struct tm *tm);                                  /* 返回当前时间 */
struct tm*		ExchangeTime(const time_t *timer);								/* time_t转换为tm */

struct timeval* GetTimeOfDay(struct timeval *tv);                               /* 返回精确的相对时间 */
float           DiffMillisecond(const struct timeval*, const struct timeval*);  /* 返回相差毫秒数 */
LLONG           DiffMicrosecond(const struct timeval*, const struct timeval*);  /* 返回相差微秒数 */
double 			DiffTimeSecond(time_t time1, time_t time0);						// 返回相差秒数

char*           FormatTime(char *, const struct tm*, const char*);              /* 格式化日期/时间值 */
char*           GetDate(char *);                                              	/* 返回形如"yyyymmdd"的四位年份的八位日期字符串 */
int				GetIntDate(const time_t *timer);								// 返回"yyyymmdd"的八位日期

char*           GetTime(char *);                                              	/* 返回形如"hhmmss"的六位时间字符串 */
char*           GetTimestamp(char *);                                         	/* 返回形如"yyyymmddhhmmss"的十四位时间戳字符串 */
int				GetIntTime(const time_t *timer);								// 返回"hhmmss"六位时间整数值

int             GetYear(const struct tm *);                                 	/* 返回年份 */
int             GetMonth(const struct tm *);                                	/* 返回月份 */
int             GetDay(const struct tm *);                                  	/* 返回日期(本月第几日) */
int             GetHour(const struct tm *);                                 	/* 返回小时数 */
int             GetMinute(const struct tm *);                               	/* 返回分钟数 */
int             GetSecond(const struct tm *);                               	/* 返回秒数 */

BOOL            IsValidDate(const char*, const char*);                      	/* 校验字符串是否是有效的日期型字符串 */
BOOL            IsLeapYear(int);                                            	/* 判断是否是闰年*/

time_t ExchangeSixTimeToTime_t(const int);										/* 将六位形式的时间转为对应的time_t值 */

#ifdef __cplusplus
}
#endif

#endif  /* _TIME_UTIL_H */

