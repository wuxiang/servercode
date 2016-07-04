#ifndef _PN_MSG_H
#define _PN_MSG_H

#include <time.h>
#include <string.h>

#define MAXURLLEN			98
#define MAXCONTENTLEN		210
#define MAXCONTENTLEN_EX	90
#define MAXPLATFORMLEN		16
#define MAXPLATNUM			10							// ��������ƽ̨��
#define MAXVERSION_MER		128							// ���汾ƴ�ӳ���

#pragma pack(1)

struct PNmessage
{
	unsigned long MsgID;								// ���ݿ��ID
	time_t ttNewsTime;									// ��Ϣʱ��
	time_t ttAreaBegin;									// �û�ע��ʱ�����俪ʼ
	time_t ttAreaEnd;									// �û�ע��ʱ���������
	char strContent[MAXCONTENTLEN + 1];					// ��Ϣ����(��󲻳���210�ֽڣ��ʱ������90�ֽ�)
	char strURL[MAXURLLEN + 1];							// ��Ϣ��ַ(������98�ֽ�)
	unsigned short cPlatformMark[MAXPLATNUM];			// ƽ̨
	unsigned char PlatformNum;							// ƽ̨��
	unsigned short uVersionMin;							// ��С�汾��
	unsigned short uVersionMax;							// ���汾��
	unsigned char PublicNoticeType;						// ��������
	unsigned int ReceiveUserMark;						// ���յľ����û���ʶ
	
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

