#ifndef _SOCKET_UTIL_H
#define _SOCKET_UTIL_H

#include <sys/socket.h>

#define  DEFAULTWAITMINISEC		15
#define  DEFAULTWAITSNDSEC		15
#define  DEFAULTWAITRCVSEC		10

int SetNoBlockMode(int, bool);						// 设置成异步或者同步方式；返回上次的方式
int CanRead(int, int nSecond = 0, 					// 判断是否有数据到达
		int nMiniSec = DEFAULTWAITMINISEC); 	
int CanWrite(int, int nSecond = 0,					// 判断是否可写数据
		int nMiniSec = DEFAULTWAITMINISEC);

// 下面3个函数：返回0表示连接一定要关闭，-1表示失败，否则表示事件收或发的数据数量
int RecvUnknowLen(int, void *ptr, size_t nbytes, 	// 接收数据
		int nSecond = DEFAULTWAITRCVSEC, 
		int nMiniSec = DEFAULTWAITMINISEC);	
int Recv(int, void *ptr, size_t nbytes, 			// 接收数据
		int nMilliSecond = DEFAULTWAITRCVSEC);	
int Send(int, const void *ptr, size_t nbytes, 		// 发送数据
		int nMilliSecond = DEFAULTWAITSNDSEC);

int ReConnect(int, const struct sockaddr *sa, 		// 重连
		socklen_t salen, int nSecond = 0);
int Listen(const char *host,						// 监听
 		const char *serv, socklen_t *addrlenp, int);
int Connect(const char *host, 						// 连接
		const char *serv, int nSecond = 0);
		
void Close(int&);									// 关闭


#endif  /* _SOCKET_UTIL_H */
