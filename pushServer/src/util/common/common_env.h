/// <summary>
/// ��־ʵ��ͷ�ļ�
/// </summary>
/// <creater>wpj</creater>
/// <version>1.0</version>
/// <revision>
///	<record part="" date="" description=""  />
/// </revision>
#ifndef _COMMON_ENV_H
#define _COMMON_ENV_H

#ifdef __cplusplus
extern "C" {
#endif

/// ��ȡӦ�ó���ִ�о���·��
const char* GetApplicationPath(char *buf, const int count);
///�����Ƿ�������
bool IsAppRunning(const char*, const int);
// �����߳�����
void SleepMs(const int);

#ifdef __cplusplus
}
#endif

#endif  /* _COMMON_ENV_H */
