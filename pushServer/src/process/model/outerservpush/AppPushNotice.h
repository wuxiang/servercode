#ifndef _INCLUDE_APPLE_PUSH_H
#define _INCLUDE_APPLE_PUSH_H

#include "SendToOuterBase.h"
#include "SSLCommon.h"
#include "../template/ThreadMutex.h"
#include "../../../util/common/common_types.h"
#include "../../../util/string/string_util.h"

enum PayloadLenEnum
{
	PL_MSG_LEN = 200,
	PL_SOUNDNAME_LEN = 16,
	PL_ACTIONKEYCAPTION_LEN = 16,
	PL_DICT_NUM = 5,
	PL_DICT_KEY_LEN = 16,
	PL_DICT_VALUE_LEN = 16,
};

#pragma pack(1)
typedef struct Payload {

    /* The Message that is displayed to the user */
    char message[PL_MSG_LEN + 1];
    /* The name of the Sound which will be played back */
    char soundName[PL_SOUNDNAME_LEN + 1];
    /* The Number which is plastered over the icon, 0 disables it */
    short badgeNumber;
    /* The Caption of the Action Key the user needs to press to launch the Application */
    char actionKeyCaption[PL_ACTIONKEYCAPTION_LEN + 1];
    /* Custom Message Dictionary, which is accessible from the Application */
    char dictKey[PL_DICT_NUM][PL_DICT_KEY_LEN + 1];
    char dictValue[PL_DICT_NUM][PL_DICT_VALUE_LEN + 1];
};
#pragma pack()

struct OuterPushServ;

class CApplePushNotice : public CSendToOuterBase
{
public:
	CApplePushNotice();
	~CApplePushNotice();
	
private:
	SSL_Connection *m_pSSLConnection;
	static CThreadMutex m_muCriticalArea;
	// 证书
	char m_cRsaClientCert[MAX_PATH_LEN];
	// 密钥
	char m_cRsaClientKey[MAX_PATH_LEN];
	// 地址
	char m_cAddress[MAX_PATH_LEN];
	// 端口
	int m_uPort;
private:
	// ssl连接
	int ssl_connect(const char *, int port,
			const char *, const char *keyfile, 
			const char* capath);
	// 销毁ssl连接
	void ssl_disconnect();
	// 发送payload数据
	int ssl_SendPayload(const char *, const char *,
			size_t);
	// 转换user token到二进制形式
	char * TransferTokenToIBinary(const char*, char*);
	// 发送到远程服务器
	int ssl_SendToRemote(const char *deviceTokenHex, Payload *payload);
	// 推送函数
	int PushNotification(const char *token, const void *data);
	// 释放已经建立的连接
	void Clear_SSL_Connection();
	// 初始化
	bool Initialize(const char*, const char*, const OuterPushServ *);
	
};


#endif	/* _INCLUDE_APPLE_PUSH_H */

