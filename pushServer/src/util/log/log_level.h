/// <summary>
/// 日志级别定义头文件
/// </summary>
/// <creater>wpj</creater>
/// <version>1.0</version>
/// <revision>
///	<record part="" date="" description=""  />
/// </revision>
#ifndef _LOG_LEVEL_H
#define _LOG_LEVEL_H

#ifdef __cplusplus
extern "C" {
#endif


// 日志级别常量
#define     LOG_TRACE       (&_logLevels[0])        /* 日志级别 TRACE */
#define     LOG_DEBUG       (&_logLevels[1])        /* 日志级别 DEBUG */
#define     LOG_INFO        (&_logLevels[2])        /* 日志级别 INFO  */
#define     LOG_WARN        (&_logLevels[3])        /* 日志级别 WARN */
#define     LOG_ERROR       (&_logLevels[4])        /* 日志级别 ERROR  */
#define     LOG_FATAL       (&_logLevels[5])        /* 日志级别 FATAL */
#define     LOG_NOTE        (&_logLevels[7])        /* 日志级别 NOTE */


// 日志级别定义
typedef struct {
    int level;									// 日志级别代码    
    char name[32];								// 日志级别名称
} LOG_LEVEL;

// 返回名称对应的日志级别
LOG_LEVEL* GetLogLevel(const char *levelName);

extern LOG_LEVEL    _logLevels[];

#ifdef __cplusplus
}
#endif

#endif  /* _LOG_LEVEL_H */
