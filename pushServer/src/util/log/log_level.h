/// <summary>
/// ��־������ͷ�ļ�
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


// ��־������
#define     LOG_TRACE       (&_logLevels[0])        /* ��־���� TRACE */
#define     LOG_DEBUG       (&_logLevels[1])        /* ��־���� DEBUG */
#define     LOG_INFO        (&_logLevels[2])        /* ��־���� INFO  */
#define     LOG_WARN        (&_logLevels[3])        /* ��־���� WARN */
#define     LOG_ERROR       (&_logLevels[4])        /* ��־���� ERROR  */
#define     LOG_FATAL       (&_logLevels[5])        /* ��־���� FATAL */
#define     LOG_NOTE        (&_logLevels[7])        /* ��־���� NOTE */


// ��־������
typedef struct {
    int level;									// ��־�������    
    char name[32];								// ��־��������
} LOG_LEVEL;

// �������ƶ�Ӧ����־����
LOG_LEVEL* GetLogLevel(const char *levelName);

extern LOG_LEVEL    _logLevels[];

#ifdef __cplusplus
}
#endif

#endif  /* _LOG_LEVEL_H */
