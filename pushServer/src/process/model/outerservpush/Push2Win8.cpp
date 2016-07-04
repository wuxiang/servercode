#include "Push2Win8.h"
#include "../../../util/log/log.h"
#include "../../../util/string/string_util.h"
#include "../../../controller/runtime.h"


CThreadMutex CPushToWin8::m_muCriticalArea;

CPushToWin8::CPushToWin8()
	: m_pSSLConnection(new SSL_Connection)
{
	bzero(m_cSecret, MAX_PATH_LEN);
	bzero(m_cSid, MAX_PATH_LEN);
	bzero(&m_OAuthToken, sizeof(OAuthToken));
}

CPushToWin8::~CPushToWin8()
{
	ssl_disconnect();
}

// ssl连接
int
CPushToWin8::Ssl_connect(const char *host, int port, SSL_Connection &conn, SocketTag &Sock)
{ 	
   	if (m_muCriticalArea.lock())
   	{
	    /* 创建一个 socket 用于 tcp 通信 */
		if ((Sock.sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			m_muCriticalArea.unlock();
			return -1;
		}
		
		/* 初始化服务器端（对方）的地址和端口信息 */
		bzero(&Sock.server_addr, sizeof(sockaddr_in));
		Sock.server_addr.sin_family = AF_INET;
		Sock.server_addr.sin_port = htons(port);
		Sock.host_info = gethostbyname(host);
		if(Sock.host_info)
	    {
	    	char **pptr;
	        int iCount = 0;
	        int iRandom = 0;
	        if (AF_INET == Sock.host_info->h_addrtype || AF_INET6 == Sock.host_info->h_addrtype)
	        {
	        	pptr = Sock.host_info->h_addr_list;   
				for(; *pptr != NULL; pptr++)
				{
					iCount++;
				}
				srand(time(NULL));
				iRandom = (rand() % iCount);
	        }
	        struct in_addr *address = (struct in_addr*)Sock.host_info->h_addr_list[iRandom];
	        Sock.server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*address)); /* Server IP */
	    }
	    else
	    {
	    	m_muCriticalArea.unlock();
	    	return -2;
		}
		
		/* 连接服务器 */
		if (connect(Sock.sock, (struct sockaddr *) &Sock.server_addr, sizeof(sockaddr_in)) != 0)
		{
			m_muCriticalArea.unlock();
	    	return -3;
		}
		
		conn.meth = SSLv23_client_method();
		conn.ctx = SSL_CTX_new(conn.meth);
		if (NULL == conn.ctx)
		{
			ERR_print_errors_fp(stdout);
			m_muCriticalArea.unlock();
			return -4;
		}
	
		/* 基于 ctx 产生一个新的 SSL */
		conn.ssl = SSL_new(conn.ctx);
		SSL_set_fd(conn.ssl, Sock.sock);
		/* 建立 SSL 连接 */
		if (SSL_connect(conn.ssl) == -1)
		{
			ERR_print_errors_fp(stderr);
			m_muCriticalArea.unlock();
			return -5;
		}
	    
	    m_muCriticalArea.unlock();
	}
	
    return 0;
}

// 释放已经建立的连接
void
CPushToWin8::Clear_SSL_Connection(SSL_Connection *pSSLConnection, SocketTag *pSocket)
{
	int err = 0;
	if (NULL != pSSLConnection)
	{
		if (NULL != pSSLConnection->ssl)
	    {
	    	/* Shutdown the client side of the SSL connection */
			err = SSL_shutdown(pSSLConnection->ssl);
			if (0 == err)
			{
				err = SSL_shutdown(pSSLConnection->ssl);
			}
			if(err == -1)
		    {
		        ERROR("Could not shutdown SSL\n");
		    }
		    /* Free the SSL structure */
	    	SSL_free(pSSLConnection->ssl);
		}
	    
	    if (NULL != pSSLConnection->ctx)
	    {
		    /* Free the SSL_CTX structure */
		    SSL_CTX_free(pSSLConnection->ctx);
		}
		
		bzero(pSSLConnection, sizeof(SSL_Connection));
	}
	
	if (NULL != pSocket)
	{
		if (pSocket->sock > 2)
		{
			close(pSocket->sock);
			pSocket->sock = -1;
		}
	}
}

// 销毁ssl连接
void
CPushToWin8::ssl_disconnect()
{
	Clear_SSL_Connection(m_pSSLConnection, &m_Socket);
    /* Free the sslcon */
    if(NULL != m_pSSLConnection)
    {
        delete m_pSSLConnection;
        m_pSSLConnection = NULL;
    }
}

/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf = (char*)malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf = (char *)malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') { 
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

const int
CPushToWin8::GetPrivateAccessToken(const char *secret, const char *sid, OAuthToken &token)
{
	if (NULL == secret || NULL == sid)
	{
		return -1;
	}
	
	char *urlEncodedSecret = url_encode((char*)secret);
	char *urlEncodedSid = url_encode((char*)sid);
	char cBody[MAX_WP8_GRANT_BODY_LEN] = {0};
	int iRet = snprintf(cBody, MAX_WP8_GRANT_BODY_LEN, "grant_type=client_credentials&client_id=%s&client_secret=%s&scope=notify.windows.com",
				urlEncodedSid, urlEncodedSecret);
				
	free(urlEncodedSecret);
	free(urlEncodedSid);
	
	if (iRet >= MAX_WP8_GRANT_BODY_LEN || iRet <= 0)
	{
		return -2;
	}
	
	char cRequest[MAX_WP8_REQUEST_LEN] = {0};
	iRet = snprintf(cRequest, MAX_WP8_REQUEST_LEN, "POST /accesstoken.srf HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nHost: login.live.com\r\nContent-Length: %d\r\n\r\n%s",
				iRet, cBody);
	if (iRet >= MAX_WP8_REQUEST_LEN || iRet <= 0)
	{
		return -3;
	}
	
	SSL_Connection Conn;
	SocketTag Socket;
	
	if (0 != Ssl_connect("login.live.com", m_uPort, Conn, Socket))
	{
		Clear_SSL_Connection(&Conn, &Socket);
		return -4;
	}
	
	TRACE("Request AccessToken data");
	// send data
	int len = SSL_write(Conn.ssl, cRequest, iRet);
	if (len < 0)
	{
		Clear_SSL_Connection(&Conn, &Socket);
		return -5;
	}
	
	// read data
	int pos = 0;
	char recvbuf[MAX_WP8_RESPONSE_LEN] = {0};
	do 
	{
		len = SSL_read(Conn.ssl, &recvbuf[pos], MAX_WP8_RESPONSE_LEN - pos);
		if (len>0)
			pos += len;
		
	}while(len > 0);

	char tokentype[] = {"\"token_type\":\""};
	char * findstr = NULL;
	findstr = strstr(recvbuf, tokentype);
	
	if(NULL != findstr)
	{
		int i = 0;
		while( (*(findstr+strlen(tokentype)+i)) != '"')
		{
			i++;
		}
		
		memcpy(token.cTokenType, findstr+strlen(tokentype), i);
	}
	
	char accesstoken[] = {"\"access_token\":\""};
	char * findstr1 = NULL;
	findstr1 = strstr(recvbuf, accesstoken);
	
	if(NULL != findstr1)
	{
		int j = 0;
		while( (*(findstr1+strlen(accesstoken)+j)) != '"')
		{
			j++;
		}
		memcpy(token.cToken, findstr1+strlen(accesstoken), j);
	}
	
	Clear_SSL_Connection(&Conn, &Socket);
	return 0;
}

// 初始化
bool
CPushToWin8::Initialize(const char *sid, const char *secret, const OuterPushServ *Setting)
{
	if (NULL == StrCopy(m_cSecret, secret, MAX_PATH_LEN))
	{
		return false;
	}
	if (NULL == StrCopy(m_cSid, sid, MAX_PATH_LEN))
	{
		return false;
	}
	
	if (NULL == Setting || NULL == StrCopy(m_cAddress, Setting->HostAdd, MAX_PATH_LEN))
	{
		return false;
	}
	m_uPort = Setting->HostPort;
	
	return true;
}

// 推送函数
int
CPushToWin8::PushNotification(const char *token, const void *data)
{
	int iRes = 0;
	
	if (IsEmptyString(m_OAuthToken.cToken))
	{
		iRes = GetPrivateAccessToken(m_cSecret, m_cSid, m_OAuthToken);
		if (0 > iRes)
		{
			ERROR("GetPrivateAccessToken failed[%d]", iRes);
			return -1;
		}
	}
	
	if (!IsConnected())
	{
		Clear_SSL_Connection(m_pSSLConnection, &m_Socket);
		if (IsEmptyString(m_cSecret) || IsEmptyString(m_cSid)
			|| IsEmptyString(m_cAddress))
		{
			ERROR("Initialize first");
			return -2;
		}
		
		if (0 != (iRes = Ssl_connect(m_cAddress, m_uPort, *m_pSSLConnection, m_Socket)))
		{
			Clear_SSL_Connection(m_pSSLConnection, &m_Socket);
			ERROR("ssl_connect error[%d Address=%s]", iRes, inet_ntoa(m_Socket.server_addr.sin_addr));
			return iRes;
		}
		TRACE("Wpns m_cAddress=%s, port=%d", inet_ntoa(m_Socket.server_addr.sin_addr), m_uPort);
	}
	
	if (!IsWriteAble())
	{
		DEBUG("cache full[Address=%s]", inet_ntoa(m_Socket.server_addr.sin_addr));
		return -3;
	}
	
	WpReqTag *pWpReqTag = (WpReqTag*)data;
	for (int i = 0; i < pWpReqTag->ValidNum && i < MAX_REQ_NUM; i++)
	{
		iRes = ssl_SendToRemote(token, pWpReqTag->ReqContent[i].cXml, 
				pWpReqTag->ReqContent[i].cType, m_OAuthToken);
		if (-99 == iRes)
		{
			bzero(&m_OAuthToken, sizeof(OAuthToken));
			iRes = GetPrivateAccessToken(m_cSecret, m_cSid, m_OAuthToken);
			if (0 > iRes)
			{
				ERROR("GetPrivateAccessToken failed[%d]", iRes);
				return -99;
			}
		}
		
		iRes = ssl_SendToRemote(token, pWpReqTag->ReqContent[i].cXml, 
				pWpReqTag->ReqContent[i].cType, m_OAuthToken);
				
		if (-5 == iRes)
		{
			return iRes;
		}		
		else if (iRes < 0 )
		{
			Clear_SSL_Connection(m_pSSLConnection, &m_Socket);
			return iRes;
		}
	}
	
	return 0;
}

// 发送到远程服务器
int 
CPushToWin8::ssl_SendToRemote(const char *SendToken, const char* xml, const char *type, const OAuthToken &Acctoken)
{
	char cBody[MAX_WP8_GRANT_BODY_LEN] = {0};
	int iRet = snprintf(cBody, MAX_WP8_GRANT_BODY_LEN, "X-WNS-Type: %s\r\nAuthorization: Bearer %s",
				type, Acctoken.cToken);
	if (iRet >= MAX_WP8_GRANT_BODY_LEN || iRet <= 0)
	{
		return -2;
	}
	
	char cRequest[MAX_WP8_REQUEST_LEN] = {0};
	iRet = snprintf(cRequest, MAX_WP8_REQUEST_LEN, "POST /?token=%s HTTP/1.1\r\nContent-Type: text/xml\r\n%s\r\nHost: sin.notify.windows.com\r\nKeep-Alive: 115\r\nConnection: keep-alive\r\nContent-Length: %u\r\n\r\n%s",
				SendToken, cBody, (unsigned int)strlen(xml), xml);
	if (iRet >= MAX_WP8_REQUEST_LEN || iRet <= 0)
	{
		return -3;
	}
	
	// send data
	int len = SSL_write(m_pSSLConnection->ssl, cRequest, iRet);
	if (len < 0)
	{
		ERR_print_errors_fp(stderr);
		return -4;
	}
	
	// read data
	int pos = 0;
	char recvbuf[MAX_WP8_RESPONSE_LEN] = {0};
	do 
	{
		len = SSL_read(m_pSSLConnection->ssl, &recvbuf[pos], MAX_WP8_RESPONSE_LEN - pos);
		if (NULL != strstr(recvbuf, "Content-Length: 0"))
		{
			break;
		}
		if (len>0)
			pos += len;
		
	}while(len > 0);
	
	if (NULL != strstr(recvbuf, "410 Gone")
		|| NULL != strstr(recvbuf, "X-WNS-NOTIFICATIONSTATUS: dropped"))
	{
		ERROR("Push Token expired");
		return -5;
	}
	else if (NULL != strstr(recvbuf, "Token expired"))
	{
		ERROR("Access Token expired");
		return -99;
	}
	else if (NULL == strstr(recvbuf, "200 OK"))
	{
		return -6;
	}
	
	return 0;
}

