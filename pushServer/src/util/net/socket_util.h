#ifndef _SOCKET_UTIL_H
#define _SOCKET_UTIL_H

#include <sys/socket.h>

#define  DEFAULTWAITMINISEC		15
#define  DEFAULTWAITSNDSEC		15
#define  DEFAULTWAITRCVSEC		10

int SetNoBlockMode(int, bool);						// ���ó��첽����ͬ����ʽ�������ϴεķ�ʽ
int CanRead(int, int nSecond = 0, 					// �ж��Ƿ������ݵ���
		int nMiniSec = DEFAULTWAITMINISEC); 	
int CanWrite(int, int nSecond = 0,					// �ж��Ƿ��д����
		int nMiniSec = DEFAULTWAITMINISEC);

// ����3������������0��ʾ����һ��Ҫ�رգ�-1��ʾʧ�ܣ������ʾ�¼��ջ򷢵���������
int RecvUnknowLen(int, void *ptr, size_t nbytes, 	// ��������
		int nSecond = DEFAULTWAITRCVSEC, 
		int nMiniSec = DEFAULTWAITMINISEC);	
int Recv(int, void *ptr, size_t nbytes, 			// ��������
		int nMilliSecond = DEFAULTWAITRCVSEC);	
int Send(int, const void *ptr, size_t nbytes, 		// ��������
		int nMilliSecond = DEFAULTWAITSNDSEC);

int ReConnect(int, const struct sockaddr *sa, 		// ����
		socklen_t salen, int nSecond = 0);
int Listen(const char *host,						// ����
 		const char *serv, socklen_t *addrlenp, int);
int Connect(const char *host, 						// ����
		const char *serv, int nSecond = 0);
		
void Close(int&);									// �ر�


#endif  /* _SOCKET_UTIL_H */
