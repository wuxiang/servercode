#include "Push2WP.h"
#include "../../../util/log/log.h"
#include "../../../util/string/string_util.h"

#include <iostream>
#include <sstream>

using namespace std;

Push2WP::Push2WP()
{
	
}

Push2WP::~Push2WP()
{

}

void 
Push2WP::processUri(string &uri)
{
	//保存完整的uri，比如"http://db3.notify.live.net/throttledthirdparty/01.00/AAEj2xatFBJVSZTDulDtwdwOAgAAAAADAQAAAAQUZm52OjIzOEQ2NDJDRkI5MEVFMEQ"
	setUri(uri);
	string httphead = "http://";
	string httpshead = "https://";
	if (uri.compare(0, httphead.length(), httphead) == 0)
	{
		int pos = uri.find('/', httphead.length());
		//保存host，比如"db3.notify.live.net"
		m_host = uri.substr(httphead.length(), pos-httphead.length());
		//保存token，比如"/throttledthirdparty/01.00/AAEj2xatFBJVSZTDulDtwdwOAgAAAAADAQAAAAQUZm52OjIzOEQ2NDJDRkI5MEVFMEQ"
		m_token = uri.substr(pos, uri.length()-pos);
	}
	else if (uri.compare(0, httpshead.length(), httpshead) == 0)
	{
		int pos = uri.find('/', httpshead.length());
		m_host = uri.substr(httpshead.length(), pos-httpshead.length());
		m_token = uri.substr(pos, uri.length()-pos);
	}
	else
	{
		//uri format is informal
	}
}
	
void 
Push2WP::setUri(string &uri)
{
	m_uri = uri;
}

string& 
Push2WP::getUri()
{
	return m_uri;
}

void 
Push2WP::setHost(string &host)
{
	m_host = host;
}

string& 
Push2WP::getHost()
{
	return m_host;
}
			
void 
Push2WP::setToken(string &token)
{
	m_token = token;
}

string& 
Push2WP::getToken()
{
	return m_token;
}

bool 
Push2WP::mpns_connect(string &host)
{
	m_Socket.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_Socket.sock == -1)
    {
        ERROR("Could not get Socket");
        return false;
    }
  
    m_Socket.server_addr.sin_family = AF_INET;
    m_Socket.server_addr.sin_port = htons(80);
    m_Socket.host_info = gethostbyname(host.c_str());
    if(m_Socket.host_info)
    {
    	char **pptr;
        int iCount = 0;
        int iRandom = 0;
        if (AF_INET == m_Socket.host_info->h_addrtype || AF_INET6 == m_Socket.host_info->h_addrtype)
        {
        	pptr = m_Socket.host_info->h_addr_list;   
			for(; *pptr != NULL; pptr++)
			{
				iCount++;
			}
			srand(time(NULL));
			iRandom = (rand() % iCount);
        }
        struct in_addr *address = (struct in_addr*)m_Socket.host_info->h_addr_list[iRandom];
	    m_Socket.server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*address)); /* Server IP */
    }
    else
    {
        ERROR("Could not resolve hostname %s", host.c_str());
        return false;
    }
  
    int err = connect(m_Socket.sock, (struct sockaddr*) &m_Socket.server_addr, sizeof(sockaddr_in));
    if(err == -1)
    {
        ERROR("Could not connect");
        return false;
    }
    
    return true;
}

int
Push2WP::PushNotification(const char *uri, const void *data)
{
	string strUri(uri);

	processUri(strUri);

	if(!IsConnected())
	{
		CloseSocket();
		if (!mpns_connect(m_host))
		{
			ERROR("ssl_connect error[Address=%s]", inet_ntoa(m_Socket.server_addr.sin_addr));
			return -1;
		}
		TRACE("Wpns m_cAddress=%s", inet_ntoa(m_Socket.server_addr.sin_addr));
	}
	
	if (!IsWriteAble())
	{
		DEBUG("cache full[Address=%s]", inet_ntoa(m_Socket.server_addr.sin_addr));
		return -3;
	}
	
	char httpBuf[1280] = {0};
	char wpxml[1280] = {0};
	char recvBuf[1280] = {0};
	if(0 == ((struct WPmessage*)data)->displaytype)
	{
		
	}
	else if(1 == ((struct WPmessage*)data)->displaytype)
	{
		if( PTE_EWARNING == ((struct WPmessage*)data)->notifytype ) // pricealarm
		{
			sprintf(wpxml, "<?xml version=\"1.0\" encoding=\"utf-8\"?><wp:Notification xmlns:wp=\"WPNotification\"><wp:Toast><wp:Text1>%s</wp:Text1><wp:Text2>%s</wp:Text2><wp:Param>/HQPages/StockAlarm.xaml?StockCode=%s</wp:Param></wp:Toast></wp:Notification>",(((struct WPmessage*)data)->text1), (((struct WPmessage*)data)->text2), (((struct WPmessage*)data)->stockcode));
			sprintf(httpBuf, "POST %s HTTP/1.1\r\nUser-Agent: curl/7.12.1 (i386-redhat-linux-gnu) libcurl/7.12.1 OpenSSL/0.9.7a zlib/1.2.1.2 libidn/0.5.6\r\nHost: %s\r\nPragma: no-cache\r\nAccept: */*\r\nContent-Type: text/xml;charset=utf-8\r\nX-WindowsPhone-Target: toast\r\nX-NotificationClass: 2\r\nKeep-Alive: 115\r\nConnection: keep-alive\r\nContent-Length: %d\r\n\r\n%s", m_token.c_str(), m_host.c_str(),(int)strlen(wpxml),wpxml);
		}
		else if( PTE_INFOMINE == ((struct WPmessage*)data)->notifytype ) // infomine
		{
			sprintf(wpxml, "<?xml version=\"1.0\" encoding=\"utf-8\"?><wp:Notification xmlns:wp=\"WPNotification\"><wp:Toast><wp:Text1>%s</wp:Text1><wp:Text2>%s</wp:Text2><wp:Param>/HQPages/StockAlarm.xaml?StockCode=%s</wp:Param></wp:Toast></wp:Notification>",(((struct WPmessage*)data)->text1), (((struct WPmessage*)data)->text2), (((struct WPmessage*)data)->stockcode));
			sprintf(httpBuf, "POST %s HTTP/1.1\r\nUser-Agent: curl/7.12.1 (i386-redhat-linux-gnu) libcurl/7.12.1 OpenSSL/0.9.7a zlib/1.2.1.2 libidn/0.5.6\r\nHost: %s\r\nPragma: no-cache\r\nAccept: */*\r\nContent-Type: text/xml;charset=utf-8\r\nX-WindowsPhone-Target: toast\r\nX-NotificationClass: 2\r\nKeep-Alive: 115\r\nConnection: keep-alive\r\nContent-Length: %d\r\n\r\n%s", m_token.c_str(), m_host.c_str(),(int)strlen(wpxml),wpxml);
		}
		else if( PTE_NOTICE == ((struct WPmessage*)data)->notifytype ) // publicnews
		{
			if(IsEmptyString(((struct WPmessage*)data)->url))
			{
				sprintf(wpxml, "<?xml version=\"1.0\" encoding=\"utf-8\"?><wp:Notification xmlns:wp=\"WPNotification\"><wp:Toast><wp:Text1>%s</wp:Text1><wp:Text2>%s</wp:Text2><wp:Param>/HQPages/FirstPage.xaml?Push=310</wp:Param></wp:Toast></wp:Notification>",(((struct WPmessage*)data)->text1), (((struct WPmessage*)data)->text2));
				sprintf(httpBuf, "POST %s HTTP/1.1\r\nUser-Agent: curl/7.12.1 (i386-redhat-linux-gnu) libcurl/7.12.1 OpenSSL/0.9.7a zlib/1.2.1.2 libidn/0.5.6\r\nHost: %s\r\nPragma: no-cache\r\nAccept: */*\r\nContent-Type: text/xml;charset=utf-8\r\nX-WindowsPhone-Target: toast\r\nX-NotificationClass: 2\r\nKeep-Alive: 115\r\nConnection: keep-alive\r\nContent-Length: %d\r\n\r\n%s", m_token.c_str(), m_host.c_str(),(int)strlen(wpxml),wpxml);
			}
			else
			{
				sprintf(wpxml, "<?xml version=\"1.0\" encoding=\"utf-8\"?><wp:Notification xmlns:wp=\"WPNotification\"><wp:Toast><wp:Text1>%s</wp:Text1><wp:Text2>%s</wp:Text2><wp:Param>/HQPages/BrowserPage.xaml?URL=%s</wp:Param></wp:Toast></wp:Notification>",(((struct WPmessage*)data)->text1), (((struct WPmessage*)data)->text2), (((struct WPmessage*)data)->url));
				sprintf(httpBuf, "POST %s HTTP/1.1\r\nUser-Agent: curl/7.12.1 (i386-redhat-linux-gnu) libcurl/7.12.1 OpenSSL/0.9.7a zlib/1.2.1.2 libidn/0.5.6\r\nHost: %s\r\nPragma: no-cache\r\nAccept: */*\r\nContent-Type: text/xml;charset=utf-8\r\nX-WindowsPhone-Target: toast\r\nX-NotificationClass: 2\r\nKeep-Alive: 115\r\nConnection: keep-alive\r\nContent-Length: %d\r\n\r\n%s", m_token.c_str(), m_host.c_str(),(int)strlen(wpxml),wpxml);
			}
		}
	}
	else if(2 == ((struct WPmessage*)data)->displaytype)
	{
		
	}
	//printf("%s",httpBuf);
	int iret = send(m_Socket.sock, httpBuf, strlen(httpBuf), 0);//just send exact what you send, otherwise error;
	
	if(iret <= 0)
	{
		if (EAGAIN != errno && EWOULDBLOCK != errno && EINTR != errno)
		{
			CloseSocket();
		}
		return -1;
		//Upon  successful  completion,  send() shall return the number of bytes sent.
		//Otherwise, -1 shall be returned and errno set to indicate the error.
	}
	
	iret = recv(m_Socket.sock, recvBuf, 1280, 0);//use whole buf to hold recv data
	if(iret <= 0)
	{
		if (EAGAIN != errno && EWOULDBLOCK != errno && EINTR != errno)
		{
			CloseSocket();
		}
		return -2;
	}
	
	return 0;
}

