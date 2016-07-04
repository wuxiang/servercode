#ifndef _INCLUDE_WP8_PUSH_H
#define _INCLUDE_WP8_PUSH_H
  
#include "SendToOuterBase.h"
#include "SSLCommon.h"
#include "../template/ThreadMutex.h"
#include "../../../util/common/common_types.h"
#include "../../../util/string/string_util.h"
#include "../../data_type.h"


const short MAX_WP8_GRANT_BODY_LEN = 768;		// 最大body长度
const short MAX_WP8_REQUEST_LEN = 1280;			// 最大请求长度
const short MAX_WP8_RESPONSE_LEN = 1280;		// 最大响应长度
const short MAX_WP8_TOKEN_TYPE_LEN = 10;		// 最大token_type长度
const short	MAX_WP8_ACCESS_TOKEN_LEN = 1280;	// Access token最大长度
const short MAX_REQ_XML_LEN = 512;				// 最大请求的xml长度
const short MAX_REQ_XML_TYPE_LEN = 64;			// 最大请求的xml类型长度
const short MAX_REQ_NUM = 4;					// 同时最大请求的个数

struct OuterPushServ;

#pragma pack(1)

struct OAuthToken
{
	char cTokenType[MAX_WP8_TOKEN_TYPE_LEN];
	char cToken[MAX_WP8_ACCESS_TOKEN_LEN];
};

struct WpReqXml
{
	// 请求类型
	char cType[MAX_REQ_XML_TYPE_LEN + 1];
	// 请求内容
	char cXml[MAX_REQ_XML_LEN + 1];
};

struct WpReqTag
{
	short ValidNum;
	WpReqXml ReqContent[MAX_REQ_NUM];
};

#pragma pack()

class CPushToWin8 : public CSendToOuterBase
{
public:
	CPushToWin8();
	~CPushToWin8();
	
private:
	SSL_Connection *m_pSSLConnection;
	static CThreadMutex m_muCriticalArea;
	// 证书
	char m_cSecret[MAX_PATH_LEN];
	// 密钥
	char m_cSid[MAX_PATH_LEN];
	// 地址
	char m_cAddress[MAX_PATH_LEN];
	// 端口
	int m_uPort;
	// Access token
	OAuthToken m_OAuthToken;

private:
	// ssl连接
	int Ssl_connect(const char *, int port, SSL_Connection&, SocketTag&);
	// 销毁ssl连接
	void ssl_disconnect();	
	// 释放已经建立的连接
	void Clear_SSL_Connection(SSL_Connection*, SocketTag*);
	const int GetPrivateAccessToken(const char *secret, const char *sid, OAuthToken &token);
	// 推送函数
	int PushNotification(const char *token, const void *data);
	// 初始化
	bool Initialize(const char*, const char*, const OuterPushServ *);
	// 发送到远程服务器
	int ssl_SendToRemote(const char *SendToken, const char* xml, const char *type, const OAuthToken &Acctoken);
};

#endif	/* _INCLUDE_WP8_PUSH_H */
