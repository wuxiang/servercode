#ifndef _INCLUDE_LOG_USER_H
#define _INCLUDE_LOG_USER_H

// ��ӡ��־
void LogUserPrint(const char *, int, const char *, 
	const char*, const unsigned char, const short, const char *, ...);
// ��־��ʼ��
void LogUserInit(const char *);
// �ر���־
void LogUserClose();
// ��ȡ����
void GetDynaUserList();


#endif	/* _INCLUDE_LOG_USER_H */

