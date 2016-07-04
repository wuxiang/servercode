#include "socket_util.h"
#include "../util.h"
#ifdef _USEPOLLWAIT
#include <poll.h>
#endif

int ConnectTimeOut(int, const struct sockaddr*, socklen_t, int);

int 
SetNoBlockMode(int SocketFd, bool bNoBlock) {
	int value = fcntl(SocketFd, F_GETFL, 0);
	int ret = value&O_NONBLOCK;
	if (bNoBlock)
	{
		value |= O_NONBLOCK;
	}
	else
	{
		int temp = O_NONBLOCK;
		temp = ~temp;
		value &= temp;
	}
	fcntl(SocketFd, F_SETFL, value);

	return ret;
}

int 
CanRead(int SocketFd, int nSecond, int nMiniSec) {
#ifdef _USEPOLLWAIT
	struct pollfd   fds;
	fds.fd = SocketFd;
	fds.events = POLLRDNORM;		
	return poll(&fds, 1, nSecond * 1000 + nMiniSec);
#else
	fd_set          set;
	struct timeval  waitTime;

	FD_ZERO(&set);
	FD_SET(SocketFd, &set);	    
	waitTime.tv_sec = nSecond;
	waitTime.tv_usec = nMiniSec * 1000;
	return select(SocketFd + 1, &set, NULL, NULL, &waitTime);
#endif
}

int
CanWrite(int SocketFd, int nSecond, int nMiniSec) {
#ifdef _USEPOLLWAIT
	struct pollfd   fds;
	fds.fd = SocketFd;
	fds.events = POLLWRNORM;
	
	return poll(&fds, 1, nSecond * 1000 + nMiniSec);
#else
	fd_set          set;
	struct timeval  waitTime;

	FD_ZERO(&set);
	FD_SET(SocketFd, &set);	    
	waitTime.tv_sec = nSecond;
	waitTime.tv_usec = nMiniSec * 1000;
	return select(SocketFd + 1, NULL, &set, NULL, &waitTime);
#endif
}

// 接收数据
int RecvUnknowLen(int SocketFd, void *ptr, size_t nbytes, int nSecond, int nMiniSec) {
	ssize_t	  n = -1;	

	if (nbytes > 0)
	{
		while ((n = recv(SocketFd, (char *)ptr, nbytes, 0)) < 0)
		{
			if (EWOULDBLOCK == errno)
			{
				if (nSecond || nMiniSec)
				{
					if (CanRead(SocketFd, nSecond, nMiniSec) <= 0)
					{
						break;
					}
					nSecond = 0;//nMiniSec只等一次
					nMiniSec = 0;
				}
				else
				{
					break;
				}
			}
			else
			{
				//连接需要关闭
				n = 0;	
				break;
			}
		}
	}

	return n;
}

// 接收数据
int Recv(int SocketFd, void *ptr, size_t nbytes, int nMilliSecond){
	int		n;
	int		num = 0;
	bool	bClose = false;

	while (nbytes > 0)
	{
		if ((n = recv(SocketFd, (char *)ptr, nbytes, 0)) < 0)
		{
			if (EWOULDBLOCK == errno)
			{
				if (nMilliSecond)
				{
					if (CanRead(SocketFd, 0, nMilliSecond) <= 0)
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			else
			{
				//连接需要关闭
				bClose = true;	
				break;
			}
		}
		else if (n)
		{
			num += n;
			nbytes -= n;
			ptr = (char *)ptr + n;
		}
		else
		{
			//连接需要关闭
			bClose = true;	
			break;
		}
	}
	if (bClose)
	{
		num = 0;
	}
	else if (num)
	{
	}
	else
	{
		num = -1;
	}

	return num;
}
		
// 发送数据	
int Send(int SocketFd, const void *ptr, size_t nbytes, int nMilliSecond) {
	int  	n;
	int     num = 0;
	bool	bClose = false;

	///////////////
	while (nbytes > 0)
	{
		if ((n = send(SocketFd, (const char *)ptr, nbytes, 0)) <= 0)
		{			
			if (EWOULDBLOCK == errno)
			{
				if (nMilliSecond)
				{
					if (CanWrite(SocketFd, 0, nMilliSecond) <= 0)
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			else
			{
				//连接需要关闭
				bClose = true;
				break;
			}
		}
		else
		{
			num += n;		
			nbytes -= n;
			ptr = (const char *)ptr + n;
		}
	}
	
	if (bClose)
	{
		num = 0;
	}
	else if (num)
	{
	}
	else
	{
		num = -1;
	}

	return num;
}

int ConnectTimeOut(int SocketFd, const struct sockaddr* pAddr, 
	socklen_t salen, int nSecond) {
	//设置非阻塞方式连接
	int prestate = SetNoBlockMode(SocketFd, true);
	int ret = connect(SocketFd, pAddr, salen);
	if(ret < 0)
	{
		if(errno == EINPROGRESS)
		{
			//正在建立连接
#ifdef _USEPOLLWAIT
			struct pollfd   fds;

			fds.fd = SocketFd;
			fds.events = POLLWRNORM;    
			ret = poll(&fds, 1, nSecond*1000)==1&&(fds.revents&POLLWRNORM)==POLLWRNORM ? 0 : -1;
#else
			fd_set          set;
			struct timeval  waitTime;

			FD_ZERO(&set);
			FD_SET(SocketFd, &set);
			waitTime.tv_sec = nSecond?nSecond:3;
			waitTime.tv_usec = 0;
			ret = select(SocketFd + 1, NULL, &set, NULL, &waitTime)==1 ? 0 : -1;
#endif			
		}
		
	}	
	if(prestate)
	{
	}
	else
	{
		SetNoBlockMode(SocketFd, false);
	}
	return ret;
}

// 连接
int ReConnect(int SocketFd, const struct sockaddr *sa, socklen_t salen, int nSecond) {
	int ret;

	if ((ret = connect(SocketFd, sa, salen)) < 0 && errno == EINTR)
	{
		if (nSecond == 0)
			nSecond = 20;

#ifdef _USEPOLLWAIT
		struct pollfd   fds;

		fds.fd = SocketFd;
		fds.events = POLLWRNORM;    
		if (poll(&fds, 1, nSecond*1000)==1&&(fds.revents&POLLWRNORM)==POLLWRNORM)
#else
		fd_set          set;
		struct timeval  waitTime;

		FD_ZERO(&set);
		FD_SET(SocketFd, &set);	    
		waitTime.tv_sec = nSecond;
		waitTime.tv_usec = 0;
		if (select(SocketFd + 1, NULL, &set, NULL, &waitTime)==1)
#endif
			ret = 0;
	}
	return ret;
}

// 监听
int Listen(const char *host, const char *serv, socklen_t *addrlenp, int backlog) {
	int ret = -1;
	int	on = 1;
	struct sockaddr_in local;

    int SocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);    
	if (SocketFd > 0 && setsockopt(SocketFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))==0)
	{
		local.sin_addr.s_addr = htonl(INADDR_ANY);
		local.sin_family = AF_INET;
		local.sin_port = htons(atoi(serv));

		if (bind(SocketFd, (struct sockaddr *)&local, sizeof(local))==0 && listen(SocketFd, backlog)==0)
		{
			ret = 0;
		}			
	}	
	if (ret == -1)
	{
		Close(SocketFd);
		return 0;
	}

	return SocketFd;
}

// 连接
int Connect(const char *host, const char *serv, int nSecond) {
	struct sockaddr_in server;
	int SocketFd = -1;

	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(serv));

    server.sin_addr.s_addr = inet_addr(host);
	if (server.sin_addr.s_addr == INADDR_NONE)
    {
        struct hostent *inhost = gethostbyname(host);
        if (inhost)
        {
			for (int i = 0; inhost->h_addr_list[i]; i++)
			{
				SocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				memcpy(&server.sin_addr, inhost->h_addr_list[i], inhost->h_length);
			    if (SocketFd >= 0)
				{
					if (nSecond > 0)
					{
						if (ConnectTimeOut(SocketFd, (struct sockaddr *)&server, sizeof(server), nSecond) == 0)
							break;
					}
					else
					{
						if (ReConnect(SocketFd, (struct sockaddr *)&server, sizeof(server)) == 0)
							break;
					}
				}
			    Close(SocketFd);
			} 
        }
		      
    }
	else
	{
		SocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (SocketFd >= 0)
		{
			if (nSecond > 0)
			{
				if (ConnectTimeOut(SocketFd, (struct sockaddr *)&server, sizeof(server), nSecond) < 0)
					Close(SocketFd);
			}
			else
			{
				if (ReConnect(SocketFd, (struct sockaddr *)&server, sizeof(server)) < 0)
					Close(SocketFd);
			}
		}
	}
	
	return SocketFd;
}

void Close(int &SocketFd) {	
	if (SocketFd >= 0)
	{
		close(SocketFd);
		SocketFd = -1;
	}
}

