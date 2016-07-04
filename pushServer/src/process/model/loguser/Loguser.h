#ifndef _INCLUDE_LOG_USER_H
#define _INCLUDE_LOG_USER_H

// 打印日志
void LogUserPrint(const char *, int, const char *, 
	const char*, const unsigned char, const short, const char *, ...);
// 日志初始化
void LogUserInit(const char *);
// 关闭日志
void LogUserClose();
// 获取更新
void GetDynaUserList();


#endif	/* _INCLUDE_LOG_USER_H */

