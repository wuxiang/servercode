/// <summary>
/// 日志实现头文件
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

/// 获取应用程序执行绝对路径
const char* GetApplicationPath(char *buf, const int count);
///程序是否在运行
bool IsAppRunning(const char*, const int);
// 调用线程休眠
void SleepMs(const int);

#ifdef __cplusplus
}
#endif

#endif  /* _COMMON_ENV_H */
