#include "log_level.h"
#include "../util.h"
#include "../common/common_lib.h"

// ��־�������
LOG_LEVEL    _logLevels[] = {
    { 0,      "TRACE"       },                  /* ������Ϣ       */
    { 1,      "DEBUG"       },                  /* ������Ϣ       */
    { 2,      "INFO"        },                  /* Ӧ����Ϣ       */
    { 3,      "WARN"        },                  /* ������Ϣ       */
    { 4,      "ERROR"       },                  /* ������Ϣ       */
    { 5,      "FATAL"       },                  /* ����������Ϣ   */
    { 6,      "NONE" 		},	        		/* ����������־   */
    { 7,      "NOTE" 		},	        		/* ǿ�Ƽ�¼����־   */
};
// -------------------------           /

/// <summary>
/// �������ƶ�Ӧ����־����
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
