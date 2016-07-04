#ifndef _INCLUDE_RESP_MSG_H
#define _INCLUDE_RESP_MSG_H

#include "../../data_type.h"
#include "../../../util/common/common_types.h"
#include <string.h>

#pragma pack(1)
// 推送消息记录
typedef struct PushMsg
{
	unsigned char MsgType;										// 消息类型(PTE_EWARNING:预警消息 PTE_INFOMINE:信息地雷 PTE_NOTICE:公告信息)
	unsigned char MsgSubType;									// 消息子类型(MsgType==PTE_EWARNING时对应WarningTypeEnum; MsgType==PTE_INFOMINE时对应信息地雷，目前用0; MsgType==PTE_NOTICE时对应PublicNoticeTypeEnum;)
	TIME_T32 DateTime;											// 触发时间
	unsigned int LatestValue;									// 触发点的最新值(MsgType==0时有效 否则无效)
	unsigned int SettingRecordID;								// 对应于此消息的设定记录的序列号(MsgType==PTE_EWARNING时对应预警记录的ID;  MsgType==PTE_INFOMINE时对应信息地雷的ID; MsgType==PTE_NOTICE时对应公告消息ID)
	char strStkCode[MAX_STKCODE_LEN];							// 证券代码(只有当根据SettingRecordID找不到对应的记录时用于查找历史)
	
	PushMsg & operator = (const PushMsg &item)
	{
		MsgType = item.MsgType;
		MsgSubType = item.MsgSubType;
		DateTime = item.DateTime;
		LatestValue = item.LatestValue;
		SettingRecordID = item.SettingRecordID;
		memcpy(strStkCode, item.strStkCode, MAX_STKCODE_LEN);
		return *this;
	}
	
	void Clear()
	{
		MsgType = (unsigned char)-1;
		MsgSubType = (unsigned char)-1;
		DateTime = 0;
		LatestValue = 0;
		SettingRecordID = (unsigned int)-1;
		memset(strStkCode, 0, MAX_STKCODE_LEN);
	}

}*pPushMsg;

#pragma pack()

#endif  /* _INCLUDE_RESP_MSG_H */

