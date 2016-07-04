#ifndef _INCLUDE_SEND_TO_OUTER_H
#define _INCLUDE_SEND_TO_OUTER_H

#include "errno.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "unistd.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

typedef struct SocketTag
{
	struct sockaddr_in   server_addr;
    struct hostent      *host_info;
    int                  sock;

public:    
    SocketTag()
    {
    	memset(&server_addr, 0, sizeof(sockaddr_in));
    	host_info = NULL;
    	sock = -1;
    }
};

struct OuterPushServ;

class CSendToOuterBase
{
public:
	CSendToOuterBase();
	virtual ~CSendToOuterBase();

protected:
	// socket连接标识
	SocketTag m_Socket;
	
protected:
	// 关闭socket连接
	void CloseSocket();
	// 是否处于连接状态
	bool IsConnected();
	// 是否可读
	bool IsWriteAble();
	
public:
	// 初始化
	virtual bool Initialize(const char*, const char*, const OuterPushServ*);
	// 推送函数
	virtual int PushNotification(const char *token, const void *data) = 0;
};

typedef CSendToOuterBase* PTR_SendOuter;

#endif	/* _INCLUDE_SEND_TO_OUTER_H */

