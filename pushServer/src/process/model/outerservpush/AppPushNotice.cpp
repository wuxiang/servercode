#include "AppPushNotice.h"
#include "../../../util/log/log.h"
#include "../../../util/string/string_util.h"
#include "../../../controller/runtime.h"

#define DEVICE_BINARY_SIZE  32
#define MAXPAYLOAD_SIZE     256

#define CA_CERT_PATH    	"./pem"
#define APPLE_HOST          "gateway.push.apple.com"
#define APPLE_PORT          2195
#define APPLE_FEEDBACK_HOST "feedback.push.apple.com"
#define APPLE_FEEDBACK_PORT 2196

CThreadMutex CApplePushNotice::m_muCriticalArea;

CApplePushNotice::CApplePushNotice()
	: m_pSSLConnection(new SSL_Connection)
{
	bzero(m_pSSLConnection, sizeof(SSL_Connection));
	bzero(m_cRsaClientCert, MAX_PATH_LEN);
	bzero(m_cRsaClientKey, MAX_PATH_LEN);
}

CApplePushNotice::~CApplePushNotice()
{
	ssl_disconnect();
}

// ssl连接
int
CApplePushNotice::ssl_connect(const char *host, int port, 
		const char *certfile, const char *keyfile, 
		const char* capath) 
{
    int err;
    if (NULL == m_pSSLConnection)
    {
    	err = -1;
    	return err;
    }
   	
   	if (m_muCriticalArea.lock())
   	{
	    /* Create an SSL_METHOD structure (choose an SSL/TLS protocol version) */
	    m_pSSLConnection->meth = SSLv3_method();
	    /* Create an SSL_CTX structure */
	    m_pSSLConnection->ctx = SSL_CTX_new(m_pSSLConnection->meth);                        
	    if(!m_pSSLConnection->ctx)
	    {
	        ERROR("Could not get SSL Context\n");
	        err = -2;
	        m_muCriticalArea.unlock();
	    	return err;
	    }
	    /* Load the CA from the Path */
	    if(SSL_CTX_load_verify_locations(m_pSSLConnection->ctx, NULL, capath) <= 0)
	    {
	        /* Handle failed load here */
	        ERROR("Failed to set CA location...\n");
	        ERR_print_errors_fp(stderr);
	        err = -3;
	        m_muCriticalArea.unlock();
	    	return err;
	    } 
	    /* Load the client certificate into the SSL_CTX structure */
	    if (SSL_CTX_use_certificate_file(m_pSSLConnection->ctx, certfile, SSL_FILETYPE_PEM) <= 0) {
	        ERROR("Cannot use Certificate File\n");
	        ERR_print_errors_fp(stderr);
	        err = -4;
	        m_muCriticalArea.unlock();
	    	return err;
	    }
	    /* Load the private-key corresponding to the client certificate */
	    if (SSL_CTX_use_PrivateKey_file(m_pSSLConnection->ctx, keyfile, SSL_FILETYPE_PEM) <= 0) {
	        ERROR("Cannot use Private Key\n");
	        ERR_print_errors_fp(stderr);
	        err = -5;
	        m_muCriticalArea.unlock();
	    	return err;
	    }
	    /* Check if the client certificate and private-key matches */
	    if (!SSL_CTX_check_private_key(m_pSSLConnection->ctx)) {
	        ERROR("Private key does not match the certificate public key\n");
	        err = -6;
	        m_muCriticalArea.unlock();
	    	return err;
	    }
	
	    /* Set up a TCP socket */
	    m_Socket.sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);       
	    if(-1 == m_Socket.sock)
	    {
	        ERROR("Could not get Socket\n");
	        err = -7;
	        m_muCriticalArea.unlock();
	    	return err;
	    }
	    
	    m_Socket.server_addr.sin_family      = AF_INET;
	    m_Socket.server_addr.sin_port        = htons(port);       /* Server Port number */
	    m_Socket.host_info = gethostbyname(host);
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
	        ERROR("Could not resolve hostname %s\n", host);
	        err = -8;
	        m_muCriticalArea.unlock();
	    	return err;
	    }
	    /* Establish a TCP/IP connection to the SSL client */
	    err = connect(m_Socket.sock, (struct sockaddr*) &m_Socket.server_addr, sizeof(sockaddr_in)); 
	    if(err == -1)
	    {
	        ERROR("Could not connect\n");
	        err = -9;
	        m_muCriticalArea.unlock();
	    	return err;
	    }    
	    /* An SSL structure is created */
	    m_pSSLConnection->ssl = SSL_new(m_pSSLConnection->ctx);
	    if(!m_pSSLConnection->ssl)
	    {
	        ERROR("Could not get SSL Socket\n");
	        err = -10;
	        m_muCriticalArea.unlock();
	    	return err;
	    }    
	    /* Assign the socket into the SSL structure (SSL and socket without BIO) */
	    SSL_set_fd(m_pSSLConnection->ssl, m_Socket.sock);
	    /* Perform SSL Handshake on the SSL client */
	    err = SSL_connect(m_pSSLConnection->ssl);
	    if(err <= 0)
	    {
	        ERROR("Could not connect to SSL Server[%d]", err);
	        err = -11;
	        m_muCriticalArea.unlock();
	    	return err;
	    }
	    
	    m_muCriticalArea.unlock();
	}
    return 0;
}

// 销毁ssl连接
void
CApplePushNotice::ssl_disconnect()
{
	Clear_SSL_Connection();
    /* Free the sslcon */
    if(NULL != m_pSSLConnection)
    {
        delete m_pSSLConnection;
        m_pSSLConnection = NULL;
    }
}

// 释放已经建立的连接
void
CApplePushNotice::Clear_SSL_Connection()
{
	int err = 0;
    if(NULL == m_pSSLConnection)
    {
        return;
    }
    
    if (NULL != m_pSSLConnection->ssl)
    {
    	/* Shutdown the client side of the SSL connection */
		int count = 1;
		err = SSL_shutdown(m_pSSLConnection->ssl);
		while(err != 1) {
		   err=SSL_shutdown(m_pSSLConnection->ssl);
		   if(err != 1)
		     count++;
		   if (count == 5)
		     break;
		   usleep(1);
		}
		if(err == -1)
	    {
	        ERROR("Could not shutdown SSL\n");
	    }
	    /* Free the SSL structure */
    	SSL_free(m_pSSLConnection->ssl);
	}
    
    if (NULL != m_pSSLConnection->ctx)
    {
	    /* Free the SSL_CTX structure */
	    SSL_CTX_free(m_pSSLConnection->ctx);
	}
	
	bzero(m_pSSLConnection, sizeof(SSL_Connection));
	
	CloseSocket();
}

// 转换user token到二进制形式
char * 
CApplePushNotice::TransferTokenToIBinary(const char* deviceTokenHex, char *pOut)
{
	/* Convert the Device Token */
    int i = 0;
    int j = 0;
    int tmpi;
    char tmp[3];
    
    if (NULL == deviceTokenHex || NULL == pOut)
    {
    	return NULL;
    }
    
    while(i < (int)strlen(deviceTokenHex))
    {
        if(deviceTokenHex[i] == ' ')
        {
            i++;
        }
        else
        {
            tmp[0] = deviceTokenHex[i];
            tmp[1] = deviceTokenHex[i + 1];
            tmp[2] = '\0';
            sscanf(tmp, "%x", &tmpi);
            if (j < DEVICE_BINARY_SIZE)
            {
        		pOut[j] = tmpi;
        	}
        	else
        	{
        		return NULL;
        	}
            i += 2;
            j++;
        }
    }
    
    return pOut;
}

// 发送payload数据
int
CApplePushNotice::ssl_SendPayload(const char *deviceTokenBinary,
	const char *payloadBuff, size_t payloadLength)
{
    int rtn = 0;
    if(NULL == m_pSSLConnection)
    {
    	ERROR("m_pSSLConnection not alloc");
        rtn = -1;
        return rtn;
    }
    
    if (NULL == deviceTokenBinary || NULL == payloadBuff
    	|| 0 == payloadLength)
    {
    	rtn = -2;
        return rtn;
    }
    
    /* command number */
    uint8_t command = 0;
    char binaryMessageBuff[sizeof(uint8_t) + sizeof(uint16_t) + DEVICE_BINARY_SIZE + sizeof(uint16_t) + MAXPAYLOAD_SIZE];
    /* message format is, |COMMAND|TOKENLEN|TOKEN|PAYLOADLEN|PAYLOAD| */
    char *binaryMessagePt = binaryMessageBuff;
    uint16_t networkOrderTokenLength = htons(DEVICE_BINARY_SIZE);
    uint16_t networkOrderPayloadLength = htons(payloadLength);
    
    /* command */
    *binaryMessagePt++ = command;
    
    /* token length network order */
    memcpy(binaryMessagePt, &networkOrderTokenLength, sizeof(uint16_t));
    binaryMessagePt += sizeof(uint16_t);
    
    /* device token */
    memcpy(binaryMessagePt, deviceTokenBinary, DEVICE_BINARY_SIZE);
    binaryMessagePt += DEVICE_BINARY_SIZE;
    
    /* payload length network order */
    memcpy(binaryMessagePt, &networkOrderPayloadLength, sizeof(uint16_t));
    binaryMessagePt += sizeof(uint16_t);
    /* payload */
    memcpy(binaryMessagePt, payloadBuff, payloadLength);
    
    binaryMessagePt += payloadLength;
    rtn = SSL_write(m_pSSLConnection->ssl, binaryMessageBuff, (binaryMessagePt - binaryMessageBuff));
	if(rtn < 1) {
		rtn = SSL_get_error(m_pSSLConnection->ssl, rtn);
		ERROR("#%d in write, program terminated\n", rtn);
		/********************************/
		/* If err=6 it means the Server */
		/* issued an SSL_shutdown. You  */
		/* must respond with a shutdown */
		/* to complete a graceful       */
		/* shutdown                     */
		/********************************/
		if(rtn==6)
		{
			DEBUG("Server issued an SSL_shutdown. \n");
			Clear_SSL_Connection();
		}
		
		return -3;
	}
	
    return 0;
}

// 发送到远程服务器
int
CApplePushNotice::ssl_SendToRemote(const char *deviceTokenHex, Payload *payload)
{
	int iRes = 0;
	char messageBuff[MAXPAYLOAD_SIZE];
    char tmpBuff[MAXPAYLOAD_SIZE];
    char badgenumBuff[3];
    char binaryToken[DEVICE_BINARY_SIZE + 1] = {0};
   
    if (NULL == payload->message)
    {
    	ERROR("payload->message error");
    	return -1;
    }
    
    if (NULL == TransferTokenToIBinary(deviceTokenHex, binaryToken))
    {
    	ERROR("Error Token Transfer");
    	return -2;
    }
    
    // {"aps":{"alert":"This is some fany message.","badge":1,"sound": "default.wav", "stkCode": "SH601519"}}
    /* Json format */
    strcpy(messageBuff, "{\"aps\":{");
    strcat(messageBuff, "\"alert\":");
    if (!IsEmptyString(payload->actionKeyCaption))
    {
    	sprintf(tmpBuff, "\"%s\",\"action-loc-key\":\"%s\",", payload->message, payload->actionKeyCaption);
    	strcat(messageBuff, tmpBuff);
    }
    else
    {
    	sprintf(tmpBuff, "\"%s\",", payload->message);
		strcat(messageBuff, tmpBuff);
    }
    
    if(payload->badgeNumber > 99 || payload->badgeNumber < 0)
    {
    	payload->badgeNumber = 1;
    }
	sprintf(badgenumBuff, "%d", payload->badgeNumber);
	strcat(messageBuff, "\"badge\":");
	strcat(messageBuff, badgenumBuff);
	strcat(messageBuff, ",\"sound\":\"");
	strcat(messageBuff, IsEmptyString(payload->soundName) ? "default" : payload->soundName);
	strcat(messageBuff, "\"");
	
	int i = 0;
	while(!IsEmptyString(payload->dictKey[i]) && i < 5)
	{
		sprintf(tmpBuff, ",\"%s\":\"%s\"", payload->dictKey[i], payload->dictValue[i]);
		strcat(messageBuff, tmpBuff);
        i++;
	}
	strcat(messageBuff, "}");
	strcat(messageBuff, "}");

	if (0 == (iRes = ssl_SendPayload(binaryToken, messageBuff, strlen(messageBuff))))
	{
		//DEBUG("Send Apple push Msg : %s", messageBuff);
	}
	return iRes;
}

// 推送函数
int
CApplePushNotice::PushNotification(const char *token, const void *data)
{
	int iRes = 0;
	
	if (!IsConnected())
	{
		Clear_SSL_Connection();
		if (IsEmptyString(m_cRsaClientCert) || IsEmptyString(m_cRsaClientKey)
			|| IsEmptyString(m_cAddress))
		{
			ERROR("Initialize first");
			return -1;
		}
		
		if (0 != (iRes = ssl_connect(m_cAddress, m_uPort, m_cRsaClientCert, m_cRsaClientKey, CA_CERT_PATH)))
		{
			Clear_SSL_Connection();
			ERROR("ssl_connect error[%d Address=%s]", iRes, inet_ntoa(m_Socket.server_addr.sin_addr));
			return iRes;
		}
		TRACE("Apns connect Address=%s, port=%d", inet_ntoa(m_Socket.server_addr.sin_addr), m_uPort);
	}
	
	if (!IsWriteAble())
	{
		DEBUG("cache full[Address=%s]", inet_ntoa(m_Socket.server_addr.sin_addr));
		return false;
	}
	iRes = ssl_SendToRemote(token, (Payload*)data);
	if (0 != iRes)
	{
		Clear_SSL_Connection();
	}
	
	return iRes;
}

// 初始化
bool
CApplePushNotice::Initialize(const char *Key1, const char *Key2, const OuterPushServ *Setting)
{
	if (NULL == StrCopy(m_cRsaClientCert, Key1, MAX_PATH_LEN))
	{
		return false;
	}
	if (NULL == StrCopy(m_cRsaClientKey, Key2, MAX_PATH_LEN))
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

