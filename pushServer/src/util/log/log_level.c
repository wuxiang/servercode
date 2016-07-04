#include "log_level.h"
#include "../util.h"
#include "../common/common_lib.h"

// 日志级别定义表
LOG_LEVEL    _logLevels[] = {
    { 0,      "TRACE"       },                  /* 跟踪信息       */
    { 1,      "DEBUG"       },                  /* 调试信息       */
    { 2,      "INFO"        },                  /* 应用信息       */
    { 3,      "WARN"        },                  /* 警告信息       */
    { 4,      "ERROR"       },                  /* 错误信息       */
    { 5,      "FATAL"       },                  /* 致命错误信息   */
    { 6,      "NONE" 		},	        		/* 屏蔽所有日志   */
    { 7,      "NOTE" 		},	        		/* 强制记录的日志   */
};
// -------------------------           /

/// <summary>
/// 返回名称对应的日志级别
/// </summary>
/// <param name="levelName"></param>
/// <returns>LOG_LEVEL*</returns>
LOG_LEVEL*
GetLogLevel(const char *levelName) {
    int i = 0;
    if (NULL == levelName) {
    	return (LOG_LEVEL*) NULL;
    }

    for (i = 0; i < (int) DIM(_logLevels); i++) {
        if (0 == strncasecmp(_logLevels[i].name, levelName, sizeof(_logLevels[i].name))) {
            return &_logLevels[i];
        }
    }
    return (LOG_LEVEL*) NULL;
}
