#ifndef _PN_MSG_H
#define _PN_MSG_H

#include <time.h>
#include <string.h>

#define MAXURLLEN			98
#define MAXCONTENTLEN		210
#define MAXCONTENTLEN_EX	90
#define MAXPLATFORMLEN		16
#define MAXPLATNUM			10							// 最大关联的平台数
#define MAXVERSION_MER		128							// 最大版本拼接长度

#pragma pack(1)

struct PNmessage
{
	unsigned long MsgID;								// 数据库的ID
	time_t ttNewsTime;									// 消息时间
	time_t ttAreaBegin;									// 用户注册时间区间开始
	time_t ttAreaEnd;									// 用户注册时间区间结束
	char strContent[MAXCONTENTLEN + 1];					// 消息内容(最大不超过210字节，活动时不超过90字节)
	char strURL[MAXURLLEN + 1];							// 消息地址(不超过98字节)
	unsigned short cPlatformMark[MAXPLATNUM];			// 平台
	unsigned char PlatformNum;							// 平台数
	unsigned short uVersionMin;							// 最小版本号
	unsigned short uVersionMax;							// 最大版本号
	unsigned char PublicNoticeType;						// 公告类型
	unsigned int ReceiveUserMark;						// 接收的具体用户标识
	
public:
	PNmessage()
	{
		MsgID = (unsigned long)-1;
		ttNewsTime = 0;
		PlatformNum = 0;
		memset(cPlatformMark, 0, sizeof(short) * MAXPLATNUM);
	}
	
	PNmessage& operator = (const PNmessage &item)
	{
		MsgID = item.MsgID;
		ttNewsTime = item.ttNewsTime;
		ttAreaBegin = item.ttAreaBegin;
		ttAreaEnd = item.ttAreaEnd;
		strncpy(strContent, item.strContent, MAXCONTENTLEN);
		strncpy(strURL, item.strURL, MAXURLLEN);
		memcpy(cPlatformMark, item.cPlatformMark, sizeof(short) * MAXPLATNUM);
		PlatformNum = item.PlatformNum;
		uVersionMin = item.uVersionMin;
		uVersionMax = item.uVersionMax;
		PublicNoticeType = item.PublicNoticeType;
		ReceiveUserMark = item.ReceiveUserMark;
		return *this;
	}
};

#pragma pack()

#endif  /* _PN_MSG_H */

