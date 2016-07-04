/// <summary>
/// ��־ʵ��ͷ�ļ�
/// </summary>
/// <creater>wpj</creater>
/// <version>1.0</version>
/// <revision>
///	<record part="" date="" description=""  />
/// </revision>
#ifndef _LOG_H
#define _LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../util.h"
#include "log_level.h"

// ��־�궨��
#define     Log(...)        LogImpl(__FILE__, __LINE__, __FUNCTION__, LOG_TRACE, __VA_ARGS__)
#define     TRACE(...)      LogImpl(__FILE__, __LINE__, __FUNCTION__, LOG_TRACE, __VA_ARGS__)
#define     DEBUG(...)      LogImpl(__FILE__, __LINE__, __FUNCTION__, LOG_DEBUG, __VA_ARGS__)
#define     INFO(...)       LogImpl(__FILE__, __LINE__, __FUNCTION__, LOG_INFO, __VA_ARGS__)
#define     WARN(...)       LogImpl(__FILE__, __LINE__, __FUNCTION__, LOG_WARN, __VA_ARGS__)
#define     ERROR(...)      LogImpl(__FILE__, __LINE__, __FUNCTION__, LOG_ERROR, __VA_ARGS__)
#define     FATAL(...)      LogImpl(__FILE__, __LINE__, __FUNCTION__, LOG_FATAL, __VA_ARGS__)
#define     NOTE(...)       LogImpl(__FILE__, __LINE__, __FUNCTION__, LOG_NOTE, __VA_ARGS__)
//-----------------					/

void LogImpl(const char *, int, const char *,			// ��־�ǼǺ���
        const LOG_LEVEL *, const char *, ...);

bool InitialLog(int *, const char *);					// ��ʼ����־
void ReleaseLog();										// �ͷ���־ռ�õ���Դ

#ifdef __cplusplus
}
#endif

#endif  /* _LOG_H */
