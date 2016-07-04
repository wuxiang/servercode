#ifndef _INCLUDE_RESP_MSG_H
#define _INCLUDE_RESP_MSG_H

#include "../../data_type.h"
#include "../../../util/common/common_types.h"
#include <string.h>

#pragma pack(1)
// ������Ϣ��¼
typedef struct PushMsg
{
	unsigned char MsgType;										// ��Ϣ����(PTE_EWARNING:Ԥ����Ϣ PTE_INFOMINE:��Ϣ���� PTE_NOTICE:������Ϣ)
	unsigned char MsgSubType;									// ��Ϣ������(MsgType==PTE_EWARNINGʱ��ӦWarningTypeEnum; MsgType==PTE_INFOMINEʱ��Ӧ��Ϣ���ף�Ŀǰ��0; MsgType==PTE_NOTICEʱ��ӦPublicNoticeTypeEnum;)
	TIME_T32 DateTime;											// ����ʱ��
	unsigned int LatestValue;									// �����������ֵ(MsgType==0ʱ��Ч ������Ч)
	unsigned int SettingRecordID;								// ��Ӧ�ڴ���Ϣ���趨��¼�����к�(MsgType==PTE_EWARNINGʱ��ӦԤ����¼��ID;  MsgType==PTE_INFOMINEʱ��Ӧ��Ϣ���׵�ID; MsgType==PTE_NOTICEʱ��Ӧ������ϢID)
	char strStkCode[MAX_STKCODE_LEN];							// ֤ȯ����(ֻ�е�����SettingRecordID�Ҳ�����Ӧ�ļ�¼ʱ���ڲ�����ʷ)
	
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

