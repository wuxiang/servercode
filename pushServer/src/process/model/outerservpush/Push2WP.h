#ifndef PUSH2WP_H
#define PUSH2WP_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

#include <string>

#include "SendToOuterBase.h"
#include "../../config.h"
#include "../../data_type.h"

enum NotificationType
{
	RAW = 0,
	TOAST = 1,
	TILE = 2,
};

enum WPmessageLenEnum
{
	WPL_PARAM1_LEN = 16,
	WPL_PARAM2_LEN = 200,
	WPL_URL_LEN = 220
};

struct WPmessage
{
	char text1[WPL_PARAM1_LEN + 1];
	char text2[WPL_PARAM2_LEN + 1];
	char stockcode[MAX_STKCODE_LEN + 1];
	char url[WPL_URL_LEN + 1];//如果是url不能超过256
	char displaytype;//
	char notifytype;//0预警, 1地雷, 2公共
};

class Push2WP: public CSendToOuterBase
{
	public:
		Push2WP();
		~Push2WP();

	private:
		void setUri(std::string &uri);
		void setHost(std::string &host);
		void setToken(std::string &token);
		std::string& getUri();
		std::string& getToken();
		std::string& getHost();
	
	private:
		std::string m_uri;
		std::string m_host;
		std::string m_token;

	public:
		void processUri(std::string &uri);
		bool mpns_connect(std::string &host);
		int PushNotification(const char *uri, const void *data);
};

#endif

