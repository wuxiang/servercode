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
	// socket���ӱ�ʶ
	SocketTag m_Socket;
	
protected:
	// �ر�socket����
	void CloseSocket();
	// �Ƿ�������״̬
	bool IsConnected();
	// �Ƿ�ɶ�
	bool IsWriteAble();
	
public:
	// ��ʼ��
	virtual bool Initialize(const char*, const char*, const OuterPushServ*);
	// ���ͺ���
	virtual int PushNotification(const char *token, const void *data) = 0;
};

typedef CSendToOuterBase* PTR_SendOuter;

#endif	/* _INCLUDE_SEND_TO_OUTER_H */

