#include "SendToOuterBase.h"
#include "../../../util/time/time_util.h"
#include "../../../controller/runtime.h"

CSendToOuterBase::CSendToOuterBase()
{

}

CSendToOuterBase::~CSendToOuterBase()
{
	CloseSocket();
}

// 关闭socket连接
void
CSendToOuterBase::CloseSocket()
{
	if(m_Socket.sock >= 2)
	{
		close(m_Socket.sock);
		m_Socket.sock = -1;
	}
}

// 是否处于连接状态
bool 
CSendToOuterBase::IsConnected()
{	
    if(m_Socket.sock < 2)
    {
    	return false;
    }
    
    // Modified by yqs
	int err = 0;
	socklen_t len = sizeof(err);
	int result = getsockopt(m_Socket.sock, SOL_SOCKET, SO_ERROR, (char*)&err,&len);
	if(result < 0 || err != 0)
	    return false;
	return true;
}

// 初始化
bool
CSendToOuterBase::Initialize(const char *Key1, const char *Key2, const OuterPushServ *Setting)
{
	return true;
}

bool
CSendToOuterBase::IsWriteAble()
{
	if(m_Socket.sock < 2)
    {
    	return false;
    }
  
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    fd_set fdwrite;
    fd_set fdexcept;
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcept);
    FD_SET(m_Socket.sock, &fdwrite);
    FD_SET(m_Socket.sock, &fdexcept);
    int ret = select(m_Socket.sock + 1, NULL, &fdwrite, &fdexcept, &timeout);
    if(-1 == ret)
    {
        return false;
    }
    
    if(ret > 0)
    {
        if(FD_ISSET(m_Socket.sock, &fdexcept))
            return false;
        else if(FD_ISSET(m_Socket.sock, &fdwrite))
        {
        	return true;
        }
    }
    
    return false;
}

