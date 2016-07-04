#include "PushUser.h"
#include "../../../util/util.h"
#include "../../../util/log/log.h"
#include "../../../util/time/time_util.h"
#include "../../../util/net/socket_util.h"
#include "../../../util/common/common_lib.h"
#include "../../../util/time/time_util.h"
#include "../../../util/string/string_util.h"
#include "../../../controller/runtime.h"
#include "../../config.h"
#include "../../data_type.h"
#include "../data/BuffServer.h"
#include "../ewarning/EarlyWarning.h"
#include "../ewarning/PlatformEWarnManage.h"
#include "../ewarning/EarlyWarningManage.h"
#include "../hqlink/HqLinkNode.h"
#include "../../quota/QuotesSnapshot.h"
#include "../ErrorMsg/ErrorMsg.h"
#include "../history/RespMsg.h"
#include "../history/PushMsgHistory.h"
#include "../history/PushMsgHistoryManage.h"
#include "../outerservpush/AppPushNotice.h"
#include "../outerservpush/Push2WP.h"
#include "../../thread/CalOperatingThread.h"
#include "../../thread/ThreadsManage.h"
#include "../../thread/NetThread.h"
#include "../db/PublicNews.h"
#include "../db/InfoMine.h"
#include "../db/DatabaseSnapshot.h"
#include "../selfstock/SelfSelectStockManage.h"
#include "../selfstock/SelfSelectStock.h"

using namespace std;

// 预警记录格式
static char sEarlyWarningFormat[5][256] = {
	{"在 %s 达到您的预警条件,股价高于:"},
	{"在 %s 达到您的预警条件,股价低于:"},
	{"在 %s 达到您的预警条件,日涨幅度:"},
	{"在 %s 达到您的预警条件,日跌幅度:"},
	{"在 %s 达到您的预警条件,日换手率:"}
};

// 全预警记录格式
static char sTotalEarlyWarningFormat[5][256] = {
	{"%s(%s) 在 %s 达到您的预警条件,股价高于:"},
	{"%s(%s) 在 %s 达到您的预警条件,股价低于:"},
	{"%s(%s) 在 %s 达到您的预警条件,日涨幅度:"},
	{"%s(%s) 在 %s 达到您的预警条件,日跌幅度:"},
	{"%s(%s) 在 %s 达到您的预警条件,日换手率:"}
};

// tile定义
static char sTitle[][128] = {
	{"股价预警"},
	{"未读自选股信息地雷"}
};

// 自选股信息地雷统计格式
static char sSelfStkInfoMineStatis[] = {"您今日未读的自选股信息地雷为%u条."};

UserLogInfor::UserLogInfor()
{
	bzero(m_strUserID, MAX_USER_ID_LEN + 1);
	bzero(m_strUserName, MAX_USER_NAME_LEN + 1);
	bzero(m_strUserPwd, MAX_USER_PWD_LEN + 1);
	bzero(m_strTelNum, MAX_TEL_NUMBER_LEN + 1);
	bzero(m_strTelID, MAX_TEL_ID_LEN + 1);
}

UserLogInfor::UserLogInfor(const char *strUserID, const char *strUserName, const char *strUserPwd,
	const char *strLocalVersion, unsigned char cPlatformCode, const char *strTelNum, const char *strTelID)
{
	bzero(m_strUserID, MAX_USER_ID_LEN + 1);
	bzero(m_strUserName, MAX_USER_NAME_LEN + 1);
	bzero(m_strUserPwd, MAX_USER_PWD_LEN + 1);
	bzero(m_strTelNum, MAX_TEL_NUMBER_LEN + 1);
	bzero(m_strTelID, MAX_TEL_ID_LEN + 1);
	
	StrCopy(m_strUserID, strUserID, MAX_USER_ID_LEN);
	StrCopy(m_strUserName, strUserName, MAX_USER_NAME_LEN);
	StrCopy(m_strUserPwd, strUserPwd, MAX_USER_PWD_LEN);
	m_uLocalVersion = (unsigned short)(atof(strLocalVersion) * 100);
	StrCopy(m_strTelNum, strTelNum, MAX_TEL_NUMBER_LEN);
	StrCopy(m_strTelID, strTelID, MAX_TEL_ID_LEN);
	m_cPlatformCode = cPlatformCode;
}

UserLogInfor::~UserLogInfor()
{
}

CPushUser::CPushUser(void)
{
	bzero(m_strUserID, MAX_USER_ID_LEN + 1);
	bzero(m_strUserName, MAX_USER_NAME_LEN + 1);
	bzero(m_strUserPwd, MAX_USER_PWD_LEN + 1);
	bzero(m_strTelNum, MAX_TEL_NUMBER_LEN + 1);
}

CPushUser::CPushUser(const UserLogInfor *LogInfo)
	: m_iUserProperty(0),
	m_uRegTime(0),
	m_tLastestAliveTime(0),
	m_uExpandInfoValue(0),
	m_uUserBasicCalc(0),

	m_uRecordSetMarkID((unsigned int)-1),
	m_iPushWarningTotal(0),
	m_iPushWarningNum(0),
	m_iPushWarningDBNum(0),
	m_iRebackWarningNum((unsigned int)-1),

	m_iInfoMineTotal(0),
	m_iInfoMineNum(0),
	m_iInfoMineDBNum(0),

	m_iPublicNoticeTotal(0),
	m_iPublicNoticeNum(0),
	m_iPublicNoticeDBNum(0)
{
	bzero(m_strUserID, MAX_USER_ID_LEN + 1);
	bzero(m_strUserName, MAX_USER_NAME_LEN + 1);
	bzero(m_strUserPwd, MAX_USER_PWD_LEN + 1);
	bzero(m_strTelNum, MAX_TEL_NUMBER_LEN + 1);
	
	// 登录信息
	StrCopy(m_strUserID, LogInfo->m_strUserID, MAX_USER_ID_LEN);
	StrCopy(m_strUserName, LogInfo->m_strUserName, MAX_USER_NAME_LEN);
	StrCopy(m_strUserPwd, LogInfo->m_strUserPwd, MAX_USER_PWD_LEN);
	m_uLocalVersion = LogInfo->m_uLocalVersion;
	StrCopy(m_strTelNum, LogInfo->m_strTelNum, MAX_TEL_NUMBER_LEN);
	m_cPlatformCode = LogInfo->m_cPlatformCode;

	SetUserActiveProperty(PU_ACTIVE);
	UpdateLastestAliveTime(GetNowTime());
}

CPushUser::~CPushUser()
{

}

// 获取当前用户的最大允许预警数目
const unsigned char
CPushUser::GetCurrentUserPermitWarningNum(const CPlatformEWarnManage *manage)const
{
	return manage->GetDefaultEWarnPerUser();
}

// 添加预警记录(预警记录ID -2：超过最大允许 -3：超过当前用户允许 -4:未知错误 -32:平台出错)
const int
CPushUser::AddEarlyWarningRecord(CEarlyWarningConditionRecord &record)
{
	CPlatformEWarnManage *pCurrentManage = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentManage)
	{
		DEBUG("m_uRecordSetMarkID=%u 传输的平台类型尚未实现，请等待升级！", m_uRecordSetMarkID);
		return -32;
	}

	unsigned char iCount = ((unsigned int)-1 == m_uRecordSetMarkID ? 0 : pCurrentManage->GetEarlyWarningNum(m_uRecordSetMarkID));
	if (iCount >= pCurrentManage->GetMaxEWarnPerUser())
	{
		return -2;
	}
	else if (iCount >= GetCurrentUserPermitWarningNum(pCurrentManage))
	{
		if (NULL == pCurrentManage->GetStkConditionRecord(m_uRecordSetMarkID,
			GetCurrentUserPermitWarningNum(pCurrentManage), record.GetStkCode()))
		{
			return -3;
		}
	}

	// 添加记录
	enum StaticNodeManageState ResType = pCurrentManage->GetManageStatus();
	if (SNMS_NORMAL != ResType)
	{
		FATAL("!!!!!!!!EwarningManage is in abnormal state[%d].!!!!!!!!!!", ResType);
		return -14;
	}

	int iAddPos = (unsigned int)-1 == m_uRecordSetMarkID ? -1 : (int)m_uRecordSetMarkID;
	int iRes = pCurrentManage->AddNewNode(record, iAddPos);
	if (iRes < 0)
	{
		DEBUG("AddEarlyWarningRecord failed.[%d]", iRes);
		return -4;
	}
	m_uRecordSetMarkID = (unsigned int)iRes;

	CEwarningManage::UpdateSetEwarnOccupy(GetUserPlatform(), m_uRecordSetMarkID);

	return pCurrentManage->GetAttachedPos();
}

// 移除指定ID的预警记录(-11:尚未设定预警记录  -12:指定ID的预警记录不存在！)
const int
CPushUser::RemoveEarlyWarningRecord(const unsigned int RecordMarkID)
{
	CPlatformEWarnManage *pCurrentManage = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentManage)
	{
		DEBUG("m_uRecordSetMarkID=%u 传输的平台类型尚未实现，请等待升级！", m_uRecordSetMarkID);
		return -32;
	}

	unsigned char iCount = ((unsigned int)-1 == m_uRecordSetMarkID ? 0 : pCurrentManage->GetEarlyWarningNum(m_uRecordSetMarkID));
	if (iCount < 1)
	{
		return -11;
	}

	CEarlyWarningConditionRecord *pRecord = pCurrentManage->GetConditionRecord(m_uRecordSetMarkID, RecordMarkID);
	if (NULL == pRecord || WARNINGSET_DELDB == pRecord->GetOperToDb())
	{
		return -12;
	}

	TRACE("Remove RecordMarkID=%u code=%s iCount=%d", RecordMarkID, pRecord->GetStkCode(), iCount);

	// 设置预警记录状态
	pRecord->UpdateSetOperToDb(WARNINGSET_DELDB);

	// 更新状态
	CEarlyWarningConditionRecord *pHead = pCurrentManage->GetIndexNode(m_uRecordSetMarkID);
	iCount--;
	if (iCount < 1)
	{
		SetHaveSetPrivateWarningProperty(PU_NO_VALID_EARLY_WARNING_SET);
		SetOperDbProperty(PU_DB_UPDATE);
		pCurrentManage->ClearGroupRecord(m_uRecordSetMarkID);
		m_uRecordSetMarkID = (unsigned int)-1;
	}
	else
	{
		SetHaveSetPrivateWarningProperty(PU_VALID_EARLY_WARNING_SET);
		SetOperDbProperty(PU_DB_UPDATE);
		pHead->UpdateSetEGroupUseFlag(WARNINGSET_USE);
		pHead->UpdateSetEWarnCount(iCount);
	}

	return 0;
}

// 编辑指定ID的预警记录
const int
CPushUser::EditEarlyWarningRecord(const unsigned int RecordMarkID,
	CEarlyWarningConditionRecord *record)
{
	CPlatformEWarnManage *pCurrentManage = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentManage)
	{
		DEBUG("m_uRecordSetMarkID=%u 传输的平台类型尚未实现，请等待升级！", m_uRecordSetMarkID);
		return -32;
	}

	unsigned char iCount = ((unsigned int)-1 == m_uRecordSetMarkID ? 0 : pCurrentManage->GetEarlyWarningNum(m_uRecordSetMarkID));
	if (iCount < 1)
	{
		return -12;
	}

	CEarlyWarningConditionRecord *pRecord = pCurrentManage->GetConditionRecord(m_uRecordSetMarkID, RecordMarkID);
	if (NULL == pRecord
		|| WARNINGSET_DELDB == pRecord->GetOperToDb()
		|| 0 != StrNoCaseCmp(record->GetStkCode(), pRecord->GetStkCode(), MAX_STKCODE_LEN))
	{
		return -12;
	}

	pRecord->UpdateValidRecord(record);
	// 更新属性
	pRecord->UpdateSetOperToDb(WARNINGSET_UPDATEDB);
	return 0;
}

// 生成预警记录的序列号
int
CPushUser::GetEarlyWarningMsgID()
{
	return  m_iPushWarningTotal++;
}

// 设置预警记录ID
void
CPushUser::SetEarlyWarningMsgID(const unsigned int NewID)
{
	if (NewID >= m_iPushWarningTotal)
	{
		m_iPushWarningTotal = NewID;
	}
}

// 形成推送记录
bool
CPushUser::GenerateEarlyWarningMsg(const int MatchBit, const struct PriceAlarm *CurrentSnapItem,
			const unsigned char RecordID, CEarlyWarningConditionRecord *CurrentWarning, PushMsg *msg)
{
	if (NULL == CurrentSnapItem || NULL == CurrentWarning)
	{
		return false;
	}

	// 预警类型
	enum WarningTypeEnum iWarningType = CurrentWarning->GetWarningTypeByBit(MatchBit);

	// 预警条件
	CEarlyWarningCondition* pCondition = CurrentWarning->GetCondition(iWarningType);
	if (NULL == pCondition)
	{
		return false;
	}

	GetEarlyWarningMsgID();
	msg->MsgType = PTE_EWARNING;
	msg->MsgSubType = (unsigned char)iWarningType;
	msg->DateTime = CurrentSnapItem->m_time;
	msg->LatestValue = pCondition->GetTriggerValue();
	msg->SettingRecordID = RecordID;
	memcpy(msg->strStkCode, CurrentWarning->GetStkCode(), MAX_STKCODE_LEN);

	return true;
}

// 转换为字符串部分记录形式
const char*
CPushUser::ChanageToPartEarlyMsg(const char *format, const PushMsg *msg, char *ReceiveCache,
		const unsigned int BufSize, const unsigned char Precision)
{
	if (NULL == format || NULL == msg || NULL == ReceiveCache)
	{
		TRACE("1");
		return NULL;
	}

	char timer[35] = {0};
	snprintf(ReceiveCache, BufSize, format,
			GetGivenTimeStamp(msg->DateTime, TIME_FORMAT_FORMATTED_MDHM, timer, 35),
			msg->LatestValue / powf(10, Precision));

	return ReceiveCache;
}

// 转换为记录全格式
const char*
CPushUser::ChangeToTotalEarlyMsg(const char *format, const PushMsg *msg,
		char *ReceiveCache,
		const unsigned int BufSize,
		const std::string &Code,
		const std::string &Name, const unsigned char Precision)
{
	if (NULL == format || NULL == msg || NULL == ReceiveCache)
	{
		TRACE("1");
		return NULL;
	}

	char timer[35] = {0};
	snprintf(ReceiveCache, BufSize, format,
			Name.c_str(),
			Code.c_str(),
			GetGivenTimeStamp(msg->DateTime, TIME_FORMAT_FORMATTED_MDHM, timer, 35),
			msg->LatestValue / powf(10, Precision) );

	return ReceiveCache;
}

// 获取预警记录Format
const char*
CPushUser::GetEalyMsgFormat(const PushMsg *pPushMsgRec, const unsigned char Precision, char *pReceive,
	const unsigned int ReceivSize, const int type)
{
	// 记录格式
	const char *pFormat = NULL;

	switch(pPushMsgRec->MsgSubType)
	{
		case PriceGreater:
			pFormat = GetEarlyWarningFormat(0, Precision, pReceive, ReceivSize, type);
			break;

		case PriceSmaller:
			pFormat = GetEarlyWarningFormat(1, Precision, pReceive, ReceivSize, type);
			break;

		case IncreaseGreater:
			pFormat = GetEarlyWarningFormat(2, Precision, pReceive, ReceivSize, type);
			break;

		case DecreaseGreater:
			pFormat = GetEarlyWarningFormat(3, Precision, pReceive, ReceivSize, type);
			break;

		case ExchangeGreater:
			pFormat = GetEarlyWarningFormat(4, Precision, pReceive, ReceivSize, type);
			break;

		default:
			break;
	}
	return pFormat;
}

// 写入历史消息
const int
CPushUser::WriteHisRecord(const unsigned int MsgType, const char *stkCode,
						const char *stkName, const char *MsgContent, CBuffWriter *writer)
{
	writer->Push_back((void*)(&MsgType), sizeof(int));

	short iItemLen = (short)strlen(stkCode);
	writer->Push_back(&iItemLen, sizeof(short));
	if (iItemLen > 0)
	{
		writer->Push_back((void*)stkCode, iItemLen);
	}

	iItemLen = (short)strlen(stkName);
	writer->Push_back(&iItemLen, sizeof(short));
	if (iItemLen > 0)
	{
		writer->Push_back((void*)stkName, iItemLen);
	}

	iItemLen = (short)strlen(MsgContent);
	writer->Push_back(&iItemLen, sizeof(short));
	if (iItemLen > 0)
	{
		writer->Push_back((void*)MsgContent, iItemLen);
	}

	return 0;
}

// 获取区间预警及信息地雷历史推送消息
int
CPushUser::GetEWarningAndInfoMineHisMsg(const unsigned int UserMapID, const unsigned int start,
	const unsigned int num, CBuffWriter *writer)
{
	unsigned short iTotalCount = 0;
	// 消息预警记录
	unsigned short iMaxCount = CPushMsgHistoryManage::GetMaxMsgCacheCount(m_cPlatformCode, PTE_EWARNING);
	unsigned int uGotNum = m_iPushWarningTotal;
	unsigned short eEarnCount = 0;
	eEarnCount = uGotNum >= iMaxCount ? iMaxCount : uGotNum;

	// 信息地雷记录
	unsigned short InfoMineCount = 0;
	iMaxCount = CPushMsgHistoryManage::GetMaxMsgCacheCount(m_cPlatformCode, PTE_INFOMINE);
	uGotNum = m_iInfoMineTotal;
	InfoMineCount = uGotNum >= iMaxCount ? iMaxCount : uGotNum;

	iTotalCount = eEarnCount + InfoMineCount;
	if (0 == iTotalCount)
	{
		// 历史记录为空
		return -24;
	}

	char cReceiveHisMsgBuf[1024 * 5] = {0};
	CBuffWriter HisWriter(cReceiveHisMsgBuf, 1024 * 5, 0);
	unsigned short iValidCount = 0;

	// 首先获取预警记录历史
	iValidCount = GetEWarningRangeHisMsg(UserMapID, start, num, eEarnCount, &HisWriter);
	if (iValidCount < num && InfoMineCount > 0)
	{
		// 用信息地雷记录补足
		iValidCount += GetInfoMineRangeHisMsg(UserMapID, 0, num - iValidCount, InfoMineCount, &HisWriter);
	}

	// sub Head
	sub_head SubHead;
	SubHead.sub_type = (unsigned short)ST_HIS;
	SubHead.sub_attrs = 0;
	SubHead.sub_extend = UserMapID;
	SubHead.sub_length = sizeof(short) + sizeof(short) + HisWriter.GetNewdataLen();

	writer->Push_back(&SubHead, sizeof(sub_head));
	writer->Push_back(&iTotalCount, sizeof(short));
	writer->Push_back((void*)&iValidCount, sizeof(short));
	if (iValidCount > 0)
	{
		writer->Push_back(cReceiveHisMsgBuf, HisWriter.GetNewdataLen());
	}
	return 0;
}

// 获取区间预警历史推送消息
int
CPushUser::GetEWarningRangeHisMsg(const unsigned int UserMapID, const unsigned int start,
	const unsigned int num, const unsigned int TotalCount, CBuffWriter *writer)
{
	pPushMsg pHis = NULL;
	char cReceiveBuf[256] = {0};
	const char *pMsg = NULL;
	char cFormat[256] = {0};

	// 记录条件
	char strCode[MAX_STKCODE_LEN ] = {0};
	char strName[MAX_STKNAME] = {0};

	unsigned short iCount = 0;
	PriceAlarm *pCurrentAlarm = NULL;
	unsigned short nStkIndex = 0xFFFF;
	for (unsigned int i = 0; iCount < TotalCount && iCount < (short)num && i < TotalCount; i++)
	{
		if (i < start)
		{
			continue;
		}

		size_t uPushIndex = m_iPushWarningTotal - 1 - i;
		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_EWARNING,
			uPushIndex, pHis) < 0)
		{
			break;
		}

		nStkIndex = CQuotesSnapshot::findSecurityIndex(pHis->strStkCode);
		if (0xFFFF == nStkIndex)
		{
			continue;
		}
		pCurrentAlarm = CQuotesSnapshot::GetIndexSecurity(nStkIndex, pHis->strStkCode);
		if (NULL == pCurrentAlarm)
		{
			continue;
		}

		StrCopy(strCode, pHis->strStkCode, MAX_STKCODE_LEN);
		StrCopy(strName, pCurrentAlarm->m_strName, MAX_STKNAME);

		const char *pFormat = GetEalyMsgFormat(pHis, pCurrentAlarm->m_cPrecision, cFormat, 256);
		if (NULL == pFormat)
		{
			DEBUG("NULL == pFormat");
			continue;
		}
		pMsg = ChanageToPartEarlyMsg(pFormat, pHis, cReceiveBuf, 256, pCurrentAlarm->m_cPrecision);

		if (NULL != pMsg && (strlen(pMsg)) > 0)
		{
			WriteHisRecord(PTE_EWARNING, strCode, strName, pMsg, writer);
			iCount++;
		}
	}

	return iCount;
}

// 获取预警记录格式
const char*
CPushUser::GetEarlyWarningFormat(const int serial, const unsigned char pricision,
	char *pReceive, const unsigned int ReceivSize, const int type)
{
	if (NULL == pReceive)
	{
		return NULL;
	}
	bzero(pReceive, ReceivSize);

	char *pStrFormat = NULL;
	if (1 == type)
	{
		if (serial >= 5)
		{
			pStrFormat = sTotalEarlyWarningFormat[0];
		}

		pStrFormat = sTotalEarlyWarningFormat[serial];
	}
	else if (0 == type)
	{
		if (serial >= 5)
		{
			pStrFormat = sEarlyWarningFormat[0];
		}
		pStrFormat = sEarlyWarningFormat[serial];
	}

	if (NULL == pStrFormat)
	{
		return NULL;
	}

	snprintf(pReceive, ReceivSize, "%s%%.%uf", pStrFormat, pricision);

	return pReceive;
}

// 清空预警记录设置
const int
CPushUser::ClearWarnRecord()
{
	CPlatformEWarnManage *pCurrentEWarn = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentEWarn)
	{
		return -1;
	}
	if (0 != GetHaveSetPrivateWarningProperty()
		&& (unsigned int)-1 != m_uRecordSetMarkID)
	{
		pCurrentEWarn->ClearGroupRecord(m_uRecordSetMarkID);
		m_uRecordSetMarkID = (unsigned int)-1;
		return 0;
	}

	return -2;
}

// 生成信息地雷记录的序列号
int
CPushUser::GetInfoMineMsgID()
{
	return m_iInfoMineTotal++;
}

// 设置信息地雷记录ID
void
CPushUser::SetInfoMineMsgID(const unsigned int NewID)
{
	if (NewID >= m_iInfoMineTotal)
	{
		m_iInfoMineTotal = NewID;
	}
}

// 形成信息地雷记录
bool
CPushUser::GenerateInfoMineMsg(const char *StkCode,
		IMmessage *InfoMineMsg, PushMsg *msg)
{
	if (NULL == msg || NULL == StkCode || NULL == InfoMineMsg)
	{
		return false;
	}

	GetInfoMineMsgID();
	msg->MsgType = PTE_INFOMINE;
	msg->MsgSubType = INFOMINE_NORMAL;
	msg->DateTime = InfoMineMsg->ttNewsTime;
	msg->LatestValue = 0;
	msg->SettingRecordID = InfoMineMsg->MsgID;
	memcpy(msg->strStkCode, StkCode, MAX_STKCODE_LEN);

	return true;
}

// 形成统计信息地雷记录
bool
CPushUser::GenerateInfoMineStatisMsg(const unsigned int KeyValue, PushMsg *msg)
{
	if (NULL == msg)
	{
		return false;
	}
	
	GetInfoMineMsgID();
	msg->MsgType = PTE_INFOMINE;
	msg->MsgSubType = INFOMINE_STATISTIC;
	msg->DateTime = GetNowTime();
	msg->LatestValue = KeyValue;
	msg->SettingRecordID = (unsigned int)-1;
	memset(msg->strStkCode, 0, MAX_STKCODE_LEN);
	return true;
}

// 获取区间信息地雷推送历史消息
int
CPushUser::GetInfoMineRangeHisMsg(const unsigned int UserMapID, const unsigned int start,
	const unsigned int num, const unsigned int TotalCount, CBuffWriter *writer)
{
	pPushMsg pHis = NULL;
	char cReceiveBuf[256] = {0};
	const char *pMsg = NULL;

	// 记录条件
	char strCode[MAX_STKCODE_LEN ] = {0};
	char strName[MAX_STKNAME] = {0};

	unsigned short iCount = 0;
	unsigned short nStkIndex = 0xFFFF;
	for (unsigned int i = 0; iCount < TotalCount && iCount < (short)num && i < TotalCount; i++)
	{
		if (i < start)
		{
			continue;
		}

		size_t uPushIndex = m_iInfoMineTotal - 1 - i;
		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_INFOMINE,
			uPushIndex, pHis) < 0)
		{
			break;
		}

		nStkIndex = CQuotesSnapshot::findSecurityIndex(pHis->strStkCode);
		if (0xFFFF == nStkIndex)
		{
			continue;
		}
		ACC_STK_STATIC *pStaticStk = CQuotesSnapshot::GetIndexStatic(nStkIndex, pHis->strStkCode);

		StrCopy(strCode, pHis->strStkCode, MAX_STKCODE_LEN);
		StrCopy(strName, pStaticStk->m_szName, MAX_STKNAME);

		IMmessage InfoMineMsg;
		if (!CInfoMine::getInfoMinePointer()->GetIndexIMmessage(strCode, pHis->SettingRecordID, InfoMineMsg))
		{
			DEBUG("No record or removed");
			continue;
		}

		pMsg = ChangeToInfoMineMsg(&InfoMineMsg, cReceiveBuf, 256);
		if (NULL != pMsg && (strlen(pMsg)) > 0)
		{
			WriteHisRecord(PTE_INFOMINE, strCode, strName, pMsg, writer);
			iCount++;
		}
	}

	return iCount;
}

// 转化为信息地雷的字符串形式
const char*
CPushUser::ChangeToInfoMineMsg(const IMmessage *msg,
						char *ReceiveCache, const unsigned int BufSize)
{
	if (NULL == msg || NULL == ReceiveCache)
	{
		DEBUG("1");
		return NULL;
	}

	char timer[35] = {0};
	snprintf(ReceiveCache, BufSize, "%s %s",
			GetGivenTimeStamp(msg->ttNewsTime, TIME_FORMAT_FORMATTED_MD, timer, 35),
			msg->strTitle);

	return ReceiveCache;
}

// 生成公告记录的序列号
int
CPushUser::GetNoticeMsgID()
{
	return m_iPublicNoticeTotal++;
}

// 设置公告记录ID
void
CPushUser::SetNoticeMsgID(const unsigned int NewID)
{
	if (NewID >= m_iPublicNoticeTotal)
	{
		m_iPublicNoticeTotal = NewID;
	}
}


// 形成公告信息记录
bool
CPushUser::GenerateNoticeMsg(PNmessage *NoticMsg, PushMsg *msg)
{
	if (NULL == msg || NULL == NoticMsg)
	{
		return false;
	}

	GetNoticeMsgID();
	msg->MsgType = PTE_NOTICE;
	msg->MsgSubType = NoticMsg->PublicNoticeType;
	msg->DateTime = NoticMsg->ttNewsTime;
	msg->LatestValue = 0;
	msg->SettingRecordID = NoticMsg->MsgID;
	bzero(msg->strStkCode, MAX_STKCODE_LEN);

	return true;
}

// 获取区间公告历史推送消息
int
CPushUser::GetPublicNoticeRangeHisMsg(const unsigned int UserMapID, const unsigned int start,
						const unsigned int num, CBuffWriter *writer)
{
	pPushMsg pHis = NULL;
	unsigned short iItemLen = 0;
	unsigned short iTotalCount = 0;

	unsigned short iMaxCount = CPushMsgHistoryManage::GetMaxMsgCacheCount(m_cPlatformCode, PTE_NOTICE);
	unsigned int uGotNum = m_iPublicNoticeTotal;
	iTotalCount = uGotNum >= iMaxCount ? iMaxCount : uGotNum;

	if (0 == iTotalCount)
	{
		// 历史记录为空
		return -24;
	}

	// 记录缓存
	char cReceiveHisMsgBuf[1024 * 2] = {0};
	CBuffWriter HisWriter(cReceiveHisMsgBuf, 1024 * 2, 0);

	unsigned short iCount = 0;
	for (unsigned int i = 0; i < iTotalCount && iCount < iTotalCount && iCount < (short)num; i++)
	{
		if (i < start)
		{
			continue;
		}

		size_t uPushIndex = m_iPublicNoticeTotal -1 - i;
		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_NOTICE,
			uPushIndex, pHis) < 0)
		{
			break;
		}

		// 查找记录设置
		PNmessage msg;
		if (!CPublicNews::getPublicNewsPointer()->GetIndexPNmessage(pHis->SettingRecordID,
				msg))
		{
			DEBUG("getPNmsgByID failed");
			continue;
		}

		// 记录历史
		HisWriter.Push_back(&uPushIndex, sizeof(int));
		HisWriter.Push_back(&msg.PublicNoticeType, sizeof(char));
		if (NOTICE_NORMAL == msg.PublicNoticeType)
		{
			iItemLen = 0;
			HisWriter.Push_back(&iItemLen, sizeof(short));
		}
		else if (NOTICE_EXTENSION == msg.PublicNoticeType)
		{
			iItemLen = (short)strlen(msg.strURL);
			HisWriter.Push_back(&iItemLen, sizeof(short));
			HisWriter.Push_back((void*)msg.strURL, iItemLen);
		}
		iItemLen = (short)strlen(msg.strContent);
		HisWriter.Push_back(&iItemLen, sizeof(short));
		HisWriter.Push_back((void*)msg.strContent, iItemLen);
		iCount++;
	}

	// sub Head
	sub_head SubHead;
	SubHead.sub_type = (unsigned short)ST_PUBLIC_HIS;
	SubHead.sub_attrs = 0;
	SubHead.sub_extend = UserMapID;
	SubHead.sub_length = sizeof(short) + sizeof(short) + HisWriter.GetNewdataLen();

	writer->Push_back(&SubHead, sizeof(sub_head));
	writer->Push_back(&iTotalCount, sizeof(short));
	writer->Push_back((void*)&iCount, sizeof(short));
	if (iCount > 0)
	{
		writer->Push_back(cReceiveHisMsgBuf, HisWriter.GetNewdataLen());
	}

	return 0;
}

// 转化为公共推送过的字符串形式
const char*
CPushUser::ChangeToPublicMsg(const PNmessage *msg,
						char *ReceiveCache, const unsigned int BufSize)
{
	if (NULL == msg || NULL == ReceiveCache)
	{
		DEBUG("1");
		return NULL;
	}

	snprintf(ReceiveCache, BufSize, "%s",
			msg->strContent);

	return ReceiveCache;
}

// 返回指定时间戳
char*
CPushUser::GetGivenTimeStamp(const time_t timer, const char *FormatDef, 
	char *buf, const unsigned int BufSize)
{
	time_t CurrentTime = timer;
	struct tm *pTm = ExchangeTime(&CurrentTime);
	if (NULL == pTm || BufSize < 32)
	{
		return NULL;
	}
	return FormatTime(buf, pTm, FormatDef);
}

// 记录消息
bool
CPushUser::PushBackMsg(const unsigned int UserMapID, PushMsg &msg)
{
	size_t MsgIndex = 0;
	size_t HaveSendMark = 0;
	switch(msg.MsgType)
	{
		case PTE_EWARNING:
			MsgIndex = m_iPushWarningTotal - 1;
			HaveSendMark = m_iPushWarningNum;
			break;

		case PTE_INFOMINE:
			MsgIndex = m_iInfoMineTotal - 1;
			HaveSendMark = m_iInfoMineNum;
			break;

		case PTE_NOTICE:
			MsgIndex = m_iPublicNoticeTotal - 1;
			HaveSendMark = m_iPublicNoticeNum;
			break;

		default:
			return false;
	}
	int iRes = CPushMsgHistoryManage::PushBackMsgHistory(m_cPlatformCode,
				UserMapID, MsgIndex, HaveSendMark, msg);

	return 0 == iRes;
}

// 检查用户的登录信息合法性
int
CPushUser::CheckUserLogValid(const UserLogInfor *pUlf)
{
	int iRes = 0;
	/* 应需求暂时忽略验证
	if (NULL == pUlf)
	{
		iRes = -6;
		return iRes;
	}

	// 验证用户名及密码
	if (NULL == pUlf->strUserName || NULL == pUlf->strUserPwd
		|| 0 != strncmp(pUlf->strUserName, m_strUserName, MAX_STKNAME)
		|| 0 != strncmp(pUlf->strUserPwd, m_strUserPwd, MAX_USER_PWD_LEN))
	{
		iRes = -7;
		return iRes;
	}

	if (0 != StrNoCaseCmp(pUlf->strUserID, m_strUserID, MAX_USER_ID_LEN))
	{
		iRes = -8;
		return iRes;
	}
*/
	return iRes;
}

// 获取全部预警记录
int
CPushUser::GetTotalEWarnRecords(CBuffWriter *writer)
{
	CPlatformEWarnManage *pCurrentEWarn = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);

	if (NULL == pCurrentEWarn)
	{
		return 0;
	}
	
	// 补充读取记录
	if (PU_VALID_EARLY_WARNING_SET == GetHaveSetPrivateWarningProperty() 
		&& (unsigned int)-1 == m_uRecordSetMarkID)
	{
		ReloadEWarningFromDb();
	}

	short iTotalCount = ((unsigned int)-1 == m_uRecordSetMarkID ? 0 : pCurrentEWarn->GetEarlyWarningNum(m_uRecordSetMarkID));
	if (iTotalCount <= 0)
	{
		return 0;
	}

	CEarlyWarningConditionRecord *pFind = NULL;
	PriceAlarm *pCurrentAlarm = NULL;
	unsigned short nStkIndex = 0xFFFF;
	int iIntVal;
	short iStrLength;
	unsigned char cVal;
	float fVal = 0.0f;
	unsigned int uTriggerValue = 0;
	short iExecuteCount = 0;
	short iCurrentUserMaxPer = GetCurrentUserPermitWarningNum(pCurrentEWarn);
	for (short i = 0; iExecuteCount < iTotalCount && i < iCurrentUserMaxPer; i++)
	{
		pFind = pCurrentEWarn->GetConditionRecord(m_uRecordSetMarkID, i);
		if (NULL == pFind || WARNINGSET_DELDB == pFind->GetOperToDb())
		{
			continue;
		}

		nStkIndex = CQuotesSnapshot::findSecurityIndex(pFind->GetStkCode());
		if (0xFFFF == nStkIndex)
		{
			continue;
		}
		pCurrentAlarm = CQuotesSnapshot::GetIndexSecurity(nStkIndex, pFind->GetStkCode());
		if (NULL == pCurrentAlarm)
		{
			continue;
		}

		float fPric = powf(10, pCurrentAlarm->m_cPrecision);

		// 预警ID
		iIntVal = i;
		writer->Push_back(&iIntVal, sizeof(int));

		// 代码
		iStrLength = (short)strlen(pFind->GetStkCode());
		writer->Push_back(&iStrLength, sizeof(short));
		writer->Push_back((void*)pFind->GetStkCode(), iStrLength);

		// 名称
		iStrLength = (short)strlen(pCurrentAlarm->m_strName);
		writer->Push_back(&iStrLength, sizeof(short));
		writer->Push_back((void*)pCurrentAlarm->m_strName, iStrLength);

		// 价格高于
		uTriggerValue = pFind->GetCondition(PriceGreater)->GetConditionValue();
		fVal = (unsigned int)-1 == uTriggerValue ? -1 : uTriggerValue / fPric;
		writer->Push_back(&fVal, sizeof(float));

		// 价格低于
		uTriggerValue = pFind->GetCondition(PriceSmaller)->GetConditionValue();
		fVal = (unsigned int)-1 == uTriggerValue ? -1 : uTriggerValue / fPric;
		writer->Push_back(&fVal, sizeof(float));

		// 日涨幅
		uTriggerValue = pFind->GetCondition(IncreaseGreater)->GetConditionValue();
		fVal = (unsigned int)-1 == uTriggerValue ? -1 : uTriggerValue / fPric;
		writer->Push_back(&fVal, sizeof(float));
		cVal = pFind->GetCondition(IncreaseGreater)->GetRawGetPeriodProperty();
		writer->Push_back(&cVal, sizeof(char));

		// 日跌幅
		uTriggerValue = pFind->GetCondition(DecreaseGreater)->GetConditionValue();
		fVal = (unsigned int)-1 == uTriggerValue ? -1 : uTriggerValue / fPric;
		writer->Push_back(&fVal, sizeof(float));
		cVal = pFind->GetCondition(DecreaseGreater)->GetRawGetPeriodProperty();
		writer->Push_back(&cVal, sizeof(char));

		// 日换手
		uTriggerValue = pFind->GetCondition(ExchangeGreater)->GetConditionValue();
		fVal = (unsigned int)-1 == uTriggerValue ? -1 : uTriggerValue / fPric;
		writer->Push_back(&fVal, sizeof(float));
		cVal = pFind->GetCondition(ExchangeGreater)->GetRawGetPeriodProperty();
		writer->Push_back(&cVal, sizeof(char));

		// 信息地雷
		cVal = pFind->GetInforMineRawProper();
		writer->Push_back(&cVal, sizeof(char));

		// 小数精度
		cVal = (char)pCurrentAlarm->m_cPrecision;
		writer->Push_back(&cVal, sizeof(char));

		iExecuteCount++;
	}

	return iExecuteCount;
}

// 获取全部预警记录的状态
int
CPushUser::GetEWarnRecordsStatus(const int iMemCount, CBuffWriter *writer)
{
	CPlatformEWarnManage *pCurrentEWarn = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);

	if (NULL == pCurrentEWarn)
	{
		return 0;
	}

	short iTotalCount = ((unsigned int)-1 == m_uRecordSetMarkID ? 0 : pCurrentEWarn->GetEarlyWarningNum(m_uRecordSetMarkID));
	if (iTotalCount <= 0)
	{
		return 0;
	}

	CEarlyWarningConditionRecord *pFind = NULL;
	int iIntVal;
	unsigned char cVal;
	short iExecuteCount = 0;
	short iCurrentUserMaxPer = GetCurrentUserPermitWarningNum(pCurrentEWarn);
	for (short i = 0; iExecuteCount < iTotalCount && i < iCurrentUserMaxPer; i++)
	{
		pFind = pCurrentEWarn->GetConditionRecord(m_uRecordSetMarkID, i);
		if (NULL == pFind || WARNINGSET_DELDB == pFind->GetOperToDb())
		{
			continue;
		}

		// 预警ID
		iIntVal = i;
		writer->Push_back(&iIntVal, sizeof(int));

		for (int i = 0; i < iMemCount; i++)
		{
			cVal = pFind->GetWarningValidationByBit(i);
			writer->Push_back(&cVal, sizeof(char));
		}
		cVal = (unsigned char)pFind->GetInforMineRawProper();
		writer->Push_back(&cVal, sizeof(char));

		iExecuteCount++;
	}

	return iExecuteCount;
}

// 处理300请求
int
CPushUser::ParseReq300(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	short iStrLength = 0;
	unsigned char cMark = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;

	// 数据定义
	char strStkCode[MAX_STKCODE_LEN] = {0};
	float fPriceGreater = 0.0f;
	float fPriceSmaller = 0.0f;
	float fIncrease = 0.0f;
	unsigned char cIncreasePeriod = 0;
	float fDown = 0.0f;
	unsigned char cDownPeriod = 0;
	float fExchange = 0.0f;
	unsigned char cExchangePeriod = 0;
	unsigned char cInfoMsgPeriod = 0;
	unsigned int uID = (unsigned int) -1;
	unsigned char Precision = 2;

	// 条件
	CEarlyWarningCondition conditionItem;
	unsigned short sProperty = 0;
	CEarlyWarningConditionRecord record;

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto RESULT;
	}

	// 取股票代码
	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	iStrLength = reader.ReadShort();
	if (iStrLength < 0 || iStrLength > (short)reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	reader.ReadString(iStrLength, strStkCode, MAX_STKCODE_LEN);
	if (strlen(strStkCode) < 1)
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}

	// 价格高于
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -2;
		goto CONDITION_ERROR;
	}
	fPriceGreater = reader.ReadFloat();

	// 价格低于
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -3;
		goto CONDITION_ERROR;
	}
	fPriceSmaller = reader.ReadFloat();

	// 日涨幅
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -4;
		goto CONDITION_ERROR;
	}
	fIncrease = reader.ReadFloat();

	// 日涨幅有效期
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -5;
		goto CONDITION_ERROR;
	}
	cIncreasePeriod = reader.ReadByte();

	// 日跌幅
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -6;
		goto CONDITION_ERROR;
	}
	fDown = reader.ReadFloat();

	// 日跌幅有效期
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -7;
		goto CONDITION_ERROR;
	}
	cDownPeriod = reader.ReadByte();

	// 换手率
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -8;
		goto CONDITION_ERROR;
	}
	fExchange = reader.ReadFloat();

	// 日换手率有效期
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -9;
		goto CONDITION_ERROR;
	}
	cExchangePeriod = reader.ReadByte();

	// 信息地雷有效期
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -10;
		goto CONDITION_ERROR;
	}
	cInfoMsgPeriod = reader.ReadByte();

	// 构造
	iRes = record.Construct(strStkCode, cInfoMsgPeriod);
	if (iRes < 0)
	{
		goto RESULT;
	}
	Precision = iRes;

	// 价格高于
	sProperty = 0;
	if (fabs(fPriceGreater - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	conditionItem.Construct(ChangeToUINT(fPriceGreater, Precision), sProperty | EWCP_PERIOD_VALIDITY_ONEDAY | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						GREATER_AND_EQUAL);
	record.SetCondition(conditionItem, PriceGreater);

	// 价格低于
	sProperty = 0;
	if (fabs(fPriceSmaller - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	conditionItem.Construct(ChangeToUINT(fPriceSmaller, Precision), sProperty | EWCP_PERIOD_VALIDITY_ONEDAY | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						SMALLER_AND_EQUAL);
	record.SetCondition(conditionItem, PriceSmaller);

	// 涨幅大于
	sProperty = 0;
	if (fabs(fIncrease - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	sProperty |= (0 == cIncreasePeriod ? EWCP_PERIOD_VALIDITY_ONCE : EWCP_PERIOD_VALIDITY_ALWAYS);
	conditionItem.Construct(ChangeToUINT(fIncrease, Precision), sProperty | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						GREATER_AND_EQUAL);
	record.SetCondition(conditionItem, IncreaseGreater);

	// 跌幅大于
	sProperty = 0;
	if (fabs(fDown - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	sProperty |= (0 == cDownPeriod ? EWCP_PERIOD_VALIDITY_ONCE : EWCP_PERIOD_VALIDITY_ALWAYS);
	conditionItem.Construct(ChangeToUINT(fDown, Precision), sProperty | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						GREATER_AND_EQUAL);
	record.SetCondition(conditionItem, DecreaseGreater);

	// 换手率大于
	sProperty = 0;
	if (fabs(fExchange - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	sProperty |= (0 == cExchangePeriod ? EWCP_PERIOD_VALIDITY_ONCE : EWCP_PERIOD_VALIDITY_ALWAYS);
	conditionItem.Construct(ChangeToUINT(fExchange, Precision), sProperty | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						GREATER_AND_EQUAL);
	record.SetCondition(conditionItem, ExchangeGreater);

	// 补充读取记录
	if (PU_VALID_EARLY_WARNING_SET == GetHaveSetPrivateWarningProperty() 
		&& (unsigned int)-1 == m_uRecordSetMarkID)
	{
		ReloadEWarningFromDb();
	}
	
	iRes = AddEarlyWarningRecord(record);
	goto RESULT;

RESULT:
	if (iRes > -1)
	{
		// 添加成功
		uID = (unsigned int)iRes;
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(int);
		cMark = 0;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		writer->Push_back(&uID, sizeof(int));

		// 预警属性设置变更
		SetHaveSetPrivateWarningProperty(PU_VALID_EARLY_WARNING_SET);
		SetUserActiveProperty(PU_ACTIVE);
		SetAllowPushMsgProperty(PU_RECEIVE_PUSH_MSG);
		SetOperDbProperty(PU_DB_UPDATE);
	}
	else
	{
		// 添加失败
		pErrorMsg = GetPushErrorMsg(iRes * -1);
		cMark = 1;
		iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(short) + iStrLength;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
	}

	UpdateBelongCalcThreadProperty(pCmdHead->sub_extend);

	return 0;

CONDITION_ERROR:
	// 解析错误
	pErrorMsg = GetPushErrorMsg(1);
	TRACE("ERR = %s", pErrorMsg);
	cMark = 1;
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(char) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cMark, sizeof(char));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return iRes;
}

// 处理301请求
int
CPushUser::ParseReq301(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	short iStrLength = 0;
	unsigned char cMark = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;

	int iEarlyID = -1;

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto RESULT;
	}

	// 取预警ID数组数目
	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	iStrLength = reader.ReadShort();

	if (iStrLength < 0 || (short)(iStrLength * sizeof(int)) > (short)reader.GetUnReadLen())
	{
		iRes = -2;
		goto CONDITION_ERROR;
	}
	// 取预警ID数组
	for (int i = 0; i < iStrLength; i++)
	{
		iEarlyID = reader.ReadInt();
		iRes = RemoveEarlyWarningRecord((unsigned int)iEarlyID);
		if (0 != iRes)
		{
			break;
		}
	}
	goto RESULT;

RESULT:
	if (iRes > -1)
	{
		// 操作成功
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char);
		cMark = 0;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
	}
	else
	{
		// 操作失败
		pErrorMsg = GetPushErrorMsg(iRes * -1);
		cMark = 1;
		iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(short) + iStrLength;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
	}

	UpdateBelongCalcThreadProperty(pCmdHead->sub_extend);
	return 0;

CONDITION_ERROR:
	// 解析错误
	pErrorMsg = GetPushErrorMsg(1);
	cMark = 1;
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(char) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cMark, sizeof(char));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return iRes;
}

// 处理302请求
int
CPushUser::ParseReq302(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	unsigned short iStrLength = 0;
	unsigned short uErrorRet = 0;
	unsigned int uErrorInt = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	short iStartPos = 0;
	short iNum = 0;

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto VERIRY_ERROR;
	}

	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	iStartPos = reader.ReadShort();

	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -2;
		goto CONDITION_ERROR;
	}
	iNum = reader.ReadShort();
	// 单次最多请求20
	if (iNum > 20)
	{
		iNum = 20;
	}
	iRes = GetEWarningAndInfoMineHisMsg(pCmdHead->sub_extend, iStartPos, iNum, writer);
	if (iRes < 0)
	{
		goto VERIRY_ERROR;
	}
	return 0;

VERIRY_ERROR:
	pErrorMsg = GetPushErrorMsg(iRes * -1);
	iNum = 0;
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(short) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&iNum, sizeof(short));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return 0;

CONDITION_ERROR:
	// 解析错误
	pErrorMsg = GetPushErrorMsg(1);
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(short) + sizeof(short) + sizeof(int) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));

	writer->Push_back(&uErrorRet, sizeof(short));
	writer->Push_back(&uErrorRet, sizeof(short));
	writer->Push_back(&uErrorInt, sizeof(int));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return iRes;
}

// 处理303请求
int
CPushUser::ParseReq303(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	CBuffReader reader(data, length, 0);
	int iRes = 0;

	const int iMemCount = CEarlyWarningConditionRecord::GetRecordMemCount();
	unsigned short iItemLen = sizeof(int) + sizeof(char) * 6;
	sub_head SubHead = {0};
	short uReturn = 0;

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		memcpy(&SubHead, pCmdHead, sizeof(sub_head));
		SubHead.sub_length = sizeof(short);
		writer->Push_back(&SubHead, sizeof(sub_head));
		uReturn = 0;
		writer->Push_back(&uReturn, sizeof(short));
		return iRes;
	}

	char cReceBuf[1024] = {0};
	CBuffWriter LocalWriter(cReceBuf, 1024, 0);
	uReturn = GetEWarnRecordsStatus(iMemCount, &LocalWriter);

	// sub Head
	SubHead.sub_type = (unsigned short)pCmdHead->sub_type;
	SubHead.sub_attrs = 0;
	SubHead.sub_extend = pCmdHead->sub_extend;
	SubHead.sub_length = sizeof(short) + uReturn * iItemLen;

	writer->Push_back(&SubHead, sizeof(sub_head));
	writer->Push_back(&uReturn, sizeof(short));
	if (LocalWriter.GetNewdataLen() > 0)
	{
		writer->Push_back(cReceBuf, LocalWriter.GetNewdataLen());
	}

	return 0;
}

// 处理305请求
int
CPushUser::ParseReq305(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		return iRes;
	}

	if (sizeof(int) > reader.GetUnReadLen())
	{
		iRes = -1;
		return iRes;
	}
	int iMsgID = reader.ReadInt();

	// 接收回应的crc
	m_iRebackWarningNum = iMsgID;

	return iRes;
}

// 处理307请求
int
CPushUser::ParseReq307(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	short iStrLength;
	unsigned char cMark = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;

	// 校验
	iRes = CheckUserLogValid(pUlf);

	if (iRes > -1)
	{
		char cReceBuf[1024] = {0};
		CBuffWriter LocalWriter(cReceBuf, 1024, 0);
		iStrLength = GetTotalEWarnRecords(&LocalWriter);

		// 操作成功
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(short) + LocalWriter.GetNewdataLen();
		cMark = 0;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));

		// 添加记录
		writer->Push_back(&iStrLength, sizeof(short));
		writer->Push_back(cReceBuf, LocalWriter.GetNewdataLen());
	}
	else
	{
		// 操作失败
		pErrorMsg = GetPushErrorMsg(iRes * -1);
		cMark = 1;
		iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(short) + iStrLength;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
	}

	return 0;
}

// 处理308请求
int
CPushUser::ParseReq308(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	short iStrLength = 0;
	unsigned char cMark = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;

	// 数据定义
	char strStkCode[MAX_STKCODE_LEN] = {0};
	float fPriceGreater = 0.0f;
	float fPriceSmaller = 0.0f;
	float fIncrease = 0.0f;
	unsigned char cIncreasePeriod = 0;
	float fDown = 0.0f;
	unsigned char cDownPeriod = 0;
	float fExchange = 0.0f;
	unsigned char cExchangePeriod = 0;
	unsigned char cInfoMsgPeriod = 0;
	int iEWarningID = 0;
	unsigned char Precision = 2;

	// 条件
	CEarlyWarningCondition conditionItem;
	unsigned short sProperty = 0;
	CEarlyWarningConditionRecord record;

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto RESULT;
	}

	// 取预警ID
	if (sizeof(int) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	iEWarningID = reader.ReadInt();

	// 取股票代码
	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	iStrLength = reader.ReadShort();
	if (iStrLength < 0 || iStrLength > (short)reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	reader.ReadString(iStrLength, strStkCode, MAX_STKCODE_LEN);
	if (strlen(strStkCode) < 1)
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}

	// 价格高于
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -2;
		goto CONDITION_ERROR;
	}
	fPriceGreater = reader.ReadFloat();

	// 价格低于
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -3;
		goto CONDITION_ERROR;
	}
	fPriceSmaller = reader.ReadFloat();

	// 日涨幅
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -4;
		goto CONDITION_ERROR;
	}
	fIncrease = reader.ReadFloat();

	// 日涨幅有效期
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -5;
		goto CONDITION_ERROR;
	}
	cIncreasePeriod = reader.ReadByte();

	// 日跌幅
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -6;
		goto CONDITION_ERROR;
	}
	fDown = reader.ReadFloat();

	// 日跌幅有效期
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -7;
		goto CONDITION_ERROR;
	}
	cDownPeriod = reader.ReadByte();

	// 换手率
	if (sizeof(float) > reader.GetUnReadLen())
	{
		iRes = -8;
		goto CONDITION_ERROR;
	}
	fExchange = reader.ReadFloat();

	// 日换手率有效期
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -9;
		goto CONDITION_ERROR;
	}
	cExchangePeriod = reader.ReadByte();

	// 信息地雷有效期
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -10;
		goto CONDITION_ERROR;
	}
	cInfoMsgPeriod = reader.ReadByte();

	// 构造
	iRes = record.Construct(strStkCode, cInfoMsgPeriod);
	if (iRes < 0)
	{
		goto RESULT;
	}
	Precision = iRes;

	// 价格高于
	sProperty = 0;
	if (fabs(fPriceGreater - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	conditionItem.Construct(ChangeToUINT(fPriceGreater, Precision), sProperty | EWCP_PERIOD_VALIDITY_ONEDAY | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						GREATER_AND_EQUAL);
	record.SetCondition(conditionItem, PriceGreater);

	// 价格低于
	sProperty = 0;
	if (fabs(fPriceSmaller - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	conditionItem.Construct(ChangeToUINT(fPriceSmaller, Precision), sProperty | EWCP_PERIOD_VALIDITY_ONEDAY | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						SMALLER_AND_EQUAL);
	record.SetCondition(conditionItem, PriceSmaller);

	// 涨幅大于
	sProperty = 0;
	if (fabs(fIncrease - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	sProperty |= (0 == cIncreasePeriod ? EWCP_PERIOD_VALIDITY_ONCE : EWCP_PERIOD_VALIDITY_ALWAYS);
	conditionItem.Construct(ChangeToUINT(fIncrease, Precision), sProperty | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						GREATER_AND_EQUAL);
	record.SetCondition(conditionItem, IncreaseGreater);

	// 跌幅大于
	sProperty = 0;
	if (fabs(fDown - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	sProperty |= (0 == cDownPeriod ? EWCP_PERIOD_VALIDITY_ONCE : EWCP_PERIOD_VALIDITY_ALWAYS);
	conditionItem.Construct(ChangeToUINT(fDown ,Precision), sProperty | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						GREATER_AND_EQUAL);
	record.SetCondition(conditionItem, DecreaseGreater);

	// 换手率大于
	sProperty = 0;
	if (fabs(fExchange - FLOATINVALID) <= FLOATZERO)
	{
		sProperty |= EWCP_INVALID;
	}
	else
	{
		sProperty |= EWCP_UNDER_MONITORING;
	}
	sProperty |= (0 == cExchangePeriod ? EWCP_PERIOD_VALIDITY_ONCE : EWCP_PERIOD_VALIDITY_ALWAYS);
	conditionItem.Construct(ChangeToUINT(fExchange, Precision), sProperty | EWCP_NO_MATCH_CONDITION | EWCP_KEEP_STATE,
						GREATER_AND_EQUAL);
	record.SetCondition(conditionItem, ExchangeGreater);

	// 编辑记录
	iRes = EditEarlyWarningRecord(iEWarningID, &record);
	goto RESULT;

RESULT:
	if (iRes > -1)
	{
		// 成功
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char);
		cMark = 0;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		
		// 更新属性信息
		SetUserActiveProperty(PU_ACTIVE);
		SetAllowPushMsgProperty(PU_RECEIVE_PUSH_MSG);
		SetOperDbProperty(PU_DB_UPDATE);
	}
	else
	{
		// 失败
		pErrorMsg = GetPushErrorMsg(iRes * -1);
		cMark = 1;
		iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(short) + iStrLength;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
	}

	return 0;

CONDITION_ERROR:
	// 解析错误
	pErrorMsg = GetPushErrorMsg(1);
	cMark = 1;
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(char) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cMark, sizeof(char));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return iRes;
}

// 处理未实现的请求
int
CPushUser::ParseReqUndefined(const sub_head *pCmdHead, void *data, const unsigned int length,
		CBuffWriter *writer)
{
	sub_head subHead = {0};
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_type = ST_ERROR;

	int iErroCode = 31;
	const char *pErrorMsg = GetPushErrorMsg(iErroCode);
	short iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	subHead.sub_length = sizeof(int) + sizeof(short) + iStrLength + length;

	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&iErroCode, sizeof(int));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	writer->Push_back((void*)data, length);
	return 0;
}

// 处理312请求
int
CPushUser::ParseReq312(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	unsigned short iStrLength = 0;
	unsigned short uErrorRet = 0;
	unsigned int uErrorInt = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	short iStartPos = 0;
	short iNum = 0;

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto VERIRY_ERROR;
	}

	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	iStartPos = reader.ReadShort();

	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -2;
		goto CONDITION_ERROR;
	}
	iNum = reader.ReadShort();
	// 单次最多请求10
	if (iNum > 10)
	{
		iNum = 10;
	}
	iRes = GetPublicNoticeRangeHisMsg(pCmdHead->sub_extend, iStartPos, iNum, writer);
	if (iRes < 0)
	{
		goto VERIRY_ERROR;
	}
	return 0;

VERIRY_ERROR:
	pErrorMsg = GetPushErrorMsg(iRes * -1);
	iNum = 0;
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(short) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&iNum, sizeof(short));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return 0;

CONDITION_ERROR:
	// 解析错误
	pErrorMsg = GetPushErrorMsg(1);
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(short) + sizeof(short) + sizeof(int) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));

	writer->Push_back(&uErrorRet, sizeof(short));
	writer->Push_back(&uErrorRet, sizeof(short));
	writer->Push_back(&uErrorInt, sizeof(int));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return iRes;
}

// 处理315请求
int
CPushUser::ParseReq315(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	short iStrLength = 0;
	unsigned char cAction = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	unsigned char cRecvMark = 0;
	short iCodeCount = 0;
	const int iMaxSingleUpload = 50;
	char strStkCodeList[iMaxSingleUpload][MAX_STKCODE_LEN + 1];
	unsigned char cMark = 0;
	int iErroCode;
	
	// 取动作
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	cAction = reader.ReadByte();
	
	// 取标识字段
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -2;
		goto CONDITION_ERROR;
	}
	cRecvMark = reader.ReadByte();
	
	// 取代码列表
	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -3;
		goto CONDITION_ERROR;
	}
	iCodeCount = reader.ReadShort();
	memset(strStkCodeList, 0, iMaxSingleUpload * (MAX_STKCODE_LEN + 1));
	for (int i = 0; i < iCodeCount; i++)
	{
		if (sizeof(short) > reader.GetUnReadLen())
		{
			iRes = -4;
			goto CONDITION_ERROR;
		}
		iStrLength = reader.ReadShort();
		if (iStrLength > 0)
		{
			if (i < iMaxSingleUpload)
			{
				reader.ReadString(iStrLength, strStkCodeList[i], MAX_STKCODE_LEN);
			}
			else
			{
				reader.ReadString(iStrLength, strStkCodeList[iMaxSingleUpload - 1], MAX_STKCODE_LEN);
			}
		}
	}
	
	iRes = UploadSelfSelectStk(pCmdHead->sub_extend, cAction, cRecvMark, (const char**)strStkCodeList, iCodeCount, MAX_STKCODE_LEN + 1);
	goto RESULT;

RESULT:
	if (iRes > -1)
	{
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char);
		cMark = 0;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
	}
	else
	{
		// 添加失败
		iErroCode = iRes * -1;
		pErrorMsg = GetPushErrorMsg(iErroCode);
		cMark = 1;
		iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short) + iStrLength;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		writer->Push_back(&iErroCode, sizeof(int));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
	}

	return 0;
	
	
CONDITION_ERROR:
	// 解析错误
	pErrorMsg = GetPushErrorMsg(1);
	cMark = 1;
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cMark, sizeof(char));
	iErroCode = 1;
	writer->Push_back(&iErroCode, sizeof(int));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return iRes;
}

// 处理316请求
int
CPushUser::ParseReq316(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	short iStrLength = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	unsigned char cRecvMark = 0;
	unsigned char cMark = 0;
	CSelfSelectStock* pSelfSelectStock = NULL;
	int iErroCode;
	
	// 取标识
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	cRecvMark = reader.ReadByte();
	
	pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);
	if (NULL == pSelfSelectStock)
	{
		iRes = -32;
		goto RESULT;
	}
	
	iRes = 0;
	pSelfSelectStock->SetHaveReadMark(pCmdHead->sub_extend, true);
	
	goto RESULT;

RESULT:
	if (iRes > -1)
	{
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(char);
		cMark = 0;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		cMark = (unsigned char)pSelfSelectStock->GetStaticsCount(pCmdHead->sub_extend);
		writer->Push_back(&cMark, sizeof(char));
	}
	else
	{
		// 添加失败
		iErroCode = iRes * -1;
		pErrorMsg = GetPushErrorMsg(iErroCode);
		cMark = 1;
		iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short) + iStrLength;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		writer->Push_back(&iErroCode, sizeof(int));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
	}

	return 0;
	
	
CONDITION_ERROR:
	// 解析错误
	pErrorMsg = GetPushErrorMsg(1);
	cMark = 1;
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cMark, sizeof(char));
	iErroCode = 1;
	writer->Push_back(&iErroCode, sizeof(int));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return iRes;
}

// 处理318请求
int
CPushUser::ParseReq318(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	short iStrLength = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	unsigned char cRecvMark = 0;
	unsigned char cMark = 0;
	CSelfSelectStock* pSelfSelectStock = NULL;
	int iErroCode;
	
	// 取标识
	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	cRecvMark = reader.ReadByte();
	
	pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);
	if (NULL == pSelfSelectStock)
	{
		iRes = -32;
		goto RESULT;
	}
	
	iRes = 0;
	
	goto RESULT;

RESULT:
	if (iRes > -1)
	{
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(char) + sizeof(char);
		cMark = 0;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		cMark = (unsigned char)pSelfSelectStock->GetStaticsCount(pCmdHead->sub_extend);
		writer->Push_back(&cMark, sizeof(char));
		cMark = (unsigned char)pSelfSelectStock->GetInfomineTotalCount(pCmdHead->sub_extend);
		writer->Push_back(&cMark, sizeof(char));
	}
	else
	{
		// 添加失败
		iErroCode = iRes * -1;
		pErrorMsg = GetPushErrorMsg(iErroCode);
		cMark = 1;
		iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short) + iStrLength;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
		writer->Push_back(&iErroCode, sizeof(int));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
	}

	return 0;
	
	
CONDITION_ERROR:
	// 解析错误
	pErrorMsg = GetPushErrorMsg(1);
	cMark = 1;
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cMark, sizeof(char));
	iErroCode = 1;
	writer->Push_back(&iErroCode, sizeof(int));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return iRes;
}

// 设置用户存活状态属性
void
CPushUser::SetUserActiveProperty(enum PushUserPropertyFlag property)
{
	unsigned short uProper = m_iUserProperty & 0xFFF8;
	m_iUserProperty = uProper | property;
}

// 设置否设置自选股预警信息
void
CPushUser::SetHaveSetPrivateWarningProperty(enum PushUserPropertyFlag property)
{
	unsigned short uProper = m_iUserProperty & 0xFFEF;
	m_iUserProperty = uProper | property;
}

// 设置否允许接收推送消息
void
CPushUser::SetAllowPushMsgProperty(enum PushUserPropertyFlag property)
{
	unsigned short uProper = m_iUserProperty & 0xFFF7;
	m_iUserProperty = uProper | property;
}

// 设置新的操作数据库的方式
void
CPushUser::SetOperDbProperty(enum PushUserPropertyFlag property)
{
	unsigned short uProper = m_iUserProperty & 0xFF9F;
	unsigned short uCurrentProper = GetOperDbProperty();
	if (PU_DB_ADD == uCurrentProper)
	{
		if (PU_DB_DEL == property || PU_DB_NULL == property)
		{
			m_iUserProperty = uProper | property;
		}
		else
		{
			m_iUserProperty = uCurrentProper | uProper;
		}
	}
	else
	{
		m_iUserProperty = uProper | property;
	}
}

// 获取当前用户平台定义
const unsigned char
CPushUser::GetUserPlatform()const
{
	return m_cPlatformCode;
}

// 更新节点最新活动时间
void
CPushUser::UpdateLastestAliveTime(const sub_head *pCmdHead)
{
	if (NULL == pCmdHead || ST_GET_LASTEST_USER_DATA <= pCmdHead->sub_type)
	{
		return;
	}
	
	m_tLastestAliveTime = GetNowTime();
}

// 更新节点最新活动时间
void
CPushUser::UpdateLastestAliveTime(const time_t NowTime)
{
	m_tLastestAliveTime = NowTime;
}

// 检查短用户节点的存活状态
const int
CPushUser::ShortItvlCheckUserAlive(const time_t NowTime)
{
	const UserNodeServerT* pSetting = GetUserNodeServer();
	float fConsumeTime = NowTime - m_tLastestAliveTime;
	float fConsumeTimeMinutes = fConsumeTime / 60.0;
	const int iCurrentAlive = GetActiveProperty();
	if (PU_ACTIVE == iCurrentAlive)
	{
		if (fConsumeTime >= S_USER_ALIVE_TIME && fConsumeTimeMinutes < pSetting->SUserDeadTime)
		{
			// 设置状态为非活跃
			SetUserActiveProperty(PU_INACTIVE);
		}
		else if (fConsumeTimeMinutes >= pSetting->SUserDeadTime)
		{
			// 设置状态为死亡用户
			SetUserActiveProperty(PU_DEAD);
		}
	}
	else if (PU_INACTIVE == iCurrentAlive)
	{
		if (fConsumeTimeMinutes >= pSetting->SUserDeadTime)
		{
			// 设置状态为死亡用户
			SetUserActiveProperty(PU_DEAD);
		}
		else if (fConsumeTime < S_USER_ALIVE_TIME)
		{
			// 设置为重新活跃用户
			SetUserActiveProperty(PU_RELIVE);
		}
	}
	else if (PU_RELIVE == iCurrentAlive)
	{
		if (fConsumeTime < S_USER_ALIVE_TIME)
		{
			// 设置为活跃用户
			SetUserActiveProperty(PU_ACTIVE);
		}
		else if (fConsumeTime >= S_USER_ALIVE_TIME && fConsumeTimeMinutes < pSetting->SUserDeadTime)
		{
			// 设置状态为非活跃
			SetUserActiveProperty(PU_INACTIVE);
		}
		else if (fConsumeTimeMinutes >= pSetting->SUserDeadTime)
		{
			// 设置状态为死亡用户
			SetUserActiveProperty(PU_DEAD);
		}
	}
	else if (PU_DEAD == iCurrentAlive)
	{
		if (fConsumeTime < S_USER_ALIVE_TIME)
		{
			// 设置为重新活跃用户
			SetUserActiveProperty(PU_RELIVE);
		}
	}
	else
	{
		return -2;
	}
	return 0;
}

// 检查长用户节点的存活状态
const int
CPushUser::LongItvlCheckUserAlive(const time_t NowTime)
{
	const UserNodeServerT* pSetting = GetUserNodeServer();
	float fConsumeTime = NowTime - m_tLastestAliveTime;
	float fConsumeTimeMinutes = fConsumeTime / 60.0;
	const int iCurrentAlive = GetActiveProperty();
	if (PU_ACTIVE == iCurrentAlive)
	{
		if (fConsumeTimeMinutes >= pSetting->LUserDeadTime)
		{
			// 设置状态为死亡用户
			SetUserActiveProperty(PU_DEAD);
		}
	}
	else if (PU_INACTIVE == iCurrentAlive || PU_DEAD == iCurrentAlive)
	{
		if (fConsumeTimeMinutes < pSetting->LUserDeadTime)
		{
			// 设置为活跃用户
			SetUserActiveProperty(PU_ACTIVE);
		}
	}
	else
	{
		return -2;
	}

	return 0;
}

// 判断用户是否为本服务器本地的用户
bool
CPushUser::IsLocalUser()
{
	return GetServerCodeNum() == GetUserServerCodeNum();
}

// 是否还可以继续执行扫描
bool
CPushUser::CanContinueExeScan(const unsigned char PushType)
{
	unsigned int uTotal = 0;
	unsigned int uSend = 0;
	unsigned int uMaxOffset = CPushMsgHistoryManage::GetMaxMsgCacheCount(m_cPlatformCode, PushType);
	
	switch(PushType)
	{
		case PTE_EWARNING:
			uTotal = m_iPushWarningTotal;
			uSend = m_iPushWarningNum;
			break;
			
		case PTE_INFOMINE:
			uTotal = m_iInfoMineTotal;
			uSend = m_iInfoMineNum;
			break;
			
		case PTE_NOTICE:
			uTotal = m_iPublicNoticeTotal;
			uSend = m_iPublicNoticeNum;
			break;
			
		default:
			break;
	}
	
	if (uTotal > uSend && uMaxOffset > 0 && (uTotal - uSend) >= uMaxOffset)
	{
		return false;
	}
	
	return true;
}

// 是否有数据需要发送
bool
CPushUser::HaveDataSend(const unsigned char PushType)
{
	unsigned int uTotal = 0;
	unsigned int uSend = 0;
	
	switch(PushType)
	{
		case PTE_EWARNING:
			uTotal = m_iPushWarningTotal;
			uSend = m_iPushWarningNum;
			break;
			
		case PTE_INFOMINE:
			uTotal = m_iInfoMineTotal;
			uSend = m_iInfoMineNum;
			break;
			
		case PTE_NOTICE:
			uTotal = m_iPublicNoticeTotal;
			uSend = m_iPublicNoticeNum;
			break;
			
		default:
			break;
	}
	
	if (uTotal > 0 && uTotal > uSend)
	{
		return true;
	}
	
	return false;
}

// 更新用户版本
void
CPushUser::SetUserLocalVersion(unsigned short VersionValue)
{
	m_uLocalVersion = VersionValue;
}

// 更新用户的注册时间
void
CPushUser::UpdateUserRegTime(const time_t tv)
{
	m_uRegTime = tv;
}

// 获取用户存活状态
const unsigned short
CPushUser::GetActiveProperty()const
{
	return (m_iUserProperty & 0x0007);
}

// 获取是否设置了自选股预警信息
const unsigned short
CPushUser::GetHaveSetPrivateWarningProperty()const
{
	return (m_iUserProperty & 0x00010);
}

// 获取是否允许接收推送消息
const unsigned short
CPushUser::GetAllowPushMsgProperty()const
{
	return (m_iUserProperty & 0x00008);
}

// 获取最新的操作数据库的方式
const unsigned short
CPushUser::GetOperDbProperty()const
{
	return (m_iUserProperty & 0x0060);
}

// 获取是否已经无效
bool
CPushUser::IsDead()const
{
	return PU_DEAD == GetActiveProperty();
}

// 获取用户标识
const char*
CPushUser::GetUserID()const
{
	return m_strUserID;
}

// 获取用户本地软件版本
const unsigned short
CPushUser::GetUserLocalVersion()const
{
	return m_uLocalVersion;
}

// 获取用户的注册时间
const TIME_T32
CPushUser::GetUserRegTime()const
{
	return m_uRegTime;
}

// 处理用户请求
int
CPushUser::ProcessUserRequest(const sub_head *pCmdHead,  const UserLogInfor *pUlf, void *data,
						const unsigned int length, CBuffWriter *writer)
{
	if (NULL == pCmdHead || NULL == data)
	{
		DEBUG("Error cmd or null data error.");
		return -1;
	}

	switch(pCmdHead->sub_type)
	{
		// 添加股票预警条件
		case ST_ADD:
			return ParseReq300(pCmdHead, pUlf, data, length, writer);

		// 删除股票预警条件
		case ST_DEL:
			return ParseReq301(pCmdHead, pUlf, data, length, writer);

		// 历史预警信息
		case ST_HIS:
			return ParseReq302(pCmdHead, pUlf, data, length, writer);

		// 当前预警条件
		case ST_LATEST:
			return ParseReq303(pCmdHead, pUlf, data, length, writer);

		// 客户端心跳
		case ST_KEEP:
			return ParseReq305(pCmdHead, pUlf, data, length, writer);

		// 取当前设置的记录
		case ST_RECGOT:
			return ParseReq307(pCmdHead, pUlf, data, length, writer);

		// 编辑预警记录
		case ST_EDIT:
			return ParseReq308(pCmdHead, pUlf, data, length, writer);

		// 公共消息历史信息
		case ST_PUBLIC_HIS:
			return ParseReq312(pCmdHead, pUlf, data, length, writer);
			
		// 上传自选股
		case ST_UPD_SELF_STOCK:
			return ParseReq315(pCmdHead, pUlf, data, length, writer);
		
		// 自选股统计信息回应
		case ST_SELF_STOCK_RECVMARK:
			return ParseReq316(pCmdHead, pUlf, data, length, writer);
		
		case ST_GET_INFOMINE_COUNT:
			return ParseReq318(pCmdHead, pUlf, data, length, writer);

		default:
			return ParseReqUndefined(pCmdHead, data, length, writer);
	}

	return -1;
}

// 获取用户的线程编号
const unsigned short
CPushUser::GetUserThreadNum()
{
	unsigned short uProp = m_uUserBasicCalc & 0xF000;
	return (uProp >>= 12) & 0x0F;
}

// 设置用户线程编号
void
CPushUser::SetUserThreadNum(const unsigned short property)
{
	unsigned short uProp = m_uUserBasicCalc & 0x0FFF;
	unsigned short value = property;
	m_uUserBasicCalc = (value <<= 12) | uProp;
}

// 获取用户所属行情编号
const unsigned short
CPushUser::GetUserHqLinkIndex()
{
	unsigned short uProp = m_uUserBasicCalc & 0x0FFF;
	return uProp;
}

// 设置用户所属行情编码
void
CPushUser::SetUserHqLinkIndex(const unsigned short property)
{
	unsigned short uProp = m_uUserBasicCalc & 0xF000;
	m_uUserBasicCalc = uProp | property;
}

// 设置用户预警服务器编码
void
CPushUser::SetUserServerCodeNum(const unsigned short code)
{
	m_nPushServId = code;
}

// 获取用户预警服务器编码
const unsigned short
CPushUser::GetUserServerCodeNum()const
{
	return m_nPushServId;
}

// 更新用户所属的计算线程属性
void
CPushUser::UpdateBelongCalcThreadProperty(const unsigned int UserMapID)
{
	CalOperatingThread  *pCurrentCalcThread = ThreadsManage::GetIndexCalcThread(GetUserThreadNum());

	unsigned short uActive = GetActiveProperty();
	unsigned short uSetEWarn = GetHaveSetPrivateWarningProperty();

	if (PU_NO_VALID_EARLY_WARNING_SET == uSetEWarn)
	{
		pCurrentCalcThread->RemoveEWarningUser(UserMapID, GetUserPlatform());
	}
	else
	{
		pCurrentCalcThread->AddEWarningUser(UserMapID, GetUserPlatform());
	}

	if (PU_DEAD == uActive)
	{
		pCurrentCalcThread->RemoveActiveUser(UserMapID, GetUserPlatform());
	}
	else
	{
		pCurrentCalcThread->AddActiveUser(UserMapID, GetUserPlatform());
	}
}

// 更新推送用户扩展信息
void
CPushUser::UpdatePushUserExpandInfo(const unsigned int ExpandInfoValue)
{
	m_uExpandInfoValue = ExpandInfoValue;
}

// 重置所有预警节点记录的状态
int
CPushUser::ResetAllEarlyWarningRecordState()
{
	CPlatformEWarnManage *pCurrentEWarn = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentEWarn)
	{
		return -1;
	}

	short iTotalCount = ((unsigned int)-1 == m_uRecordSetMarkID ? 0 : pCurrentEWarn->GetEarlyWarningNum(m_uRecordSetMarkID));
	if (iTotalCount <= 0)
	{
		return 0;
	}

	CEarlyWarningConditionRecord *pFind = NULL;
	short iExecuteCount = 0;
	short iCurrentUserMaxPer = GetCurrentUserPermitWarningNum(pCurrentEWarn);
	for (short i = 0; iExecuteCount < iTotalCount && i < iCurrentUserMaxPer; i++)
	{
		pFind = pCurrentEWarn->GetConditionRecord(m_uRecordSetMarkID, i);
		if (NULL == pFind || WARNINGSET_DELDB == pFind->GetOperToDb())
		{
			continue;
		}

		pFind->ResetRecordState();
		pFind->UpdateSetOperToDb(WARNINGSET_UPDATEDB);
		iExecuteCount++;
	}

	return 0;
}

// 重置预警节点最新状态
int
CPushUser::ResetEarlyWarningRecordLastestStkInfo()
{
	CPlatformEWarnManage *pCurrentEWarn = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentEWarn)
	{
		return -1;
	}

	short iTotalCount = ((unsigned int)-1 == m_uRecordSetMarkID ? 0 : pCurrentEWarn->GetEarlyWarningNum(m_uRecordSetMarkID));
	if (iTotalCount <= 0)
	{
		return 0;
	}

	CEarlyWarningConditionRecord *pFind = NULL;
	short iExecuteCount = 0;
	short iCurrentUserMaxPer = GetCurrentUserPermitWarningNum(pCurrentEWarn);
	for (short i = 0; iExecuteCount < iTotalCount && i < iCurrentUserMaxPer; i++)
	{
		pFind = pCurrentEWarn->GetConditionRecord(m_uRecordSetMarkID, i);
		if (NULL == pFind || WARNINGSET_DELDB == pFind->GetOperToDb())
		{
			continue;
		}

		pFind->ResetStkLatestInfo();
		iExecuteCount++;
	}

	return 0;
}

// 从数据库中重新加载预警记录设置
const int
CPushUser::ReloadEWarningFromDb()
{
	if (PU_NO_VALID_EARLY_WARNING_SET == GetHaveSetPrivateWarningProperty())
	{
		return 0;
	}

	CPlatformEWarnManage *pCurrentManage = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentManage)
	{
		DEBUG("m_uRecordSetMarkID=%u 传输的平台类型尚未实现，请等待升级！", m_uRecordSetMarkID);
		return -32;
	}

	unsigned char iCount = GetCurrentUserPermitWarningNum(pCurrentManage);
	CEarlyWarningConditionRecord eWarnRecord;
	int iAddCount = 0;
	int iRecordID = -1;
	for (unsigned char i = 0; i < iCount; i++)
	{
		if (0 < DatabaseSnapshot::GetMain()->ReadUserEWarning(GetUserID(),
				GetUserPlatform(), i, eWarnRecord))
		{
			iRecordID = AddEarlyWarningRecord(eWarnRecord);
			if (0 <= iRecordID)
			{	
				// 从库中加载的记录不需要更新
				CEarlyWarningConditionRecord *pRecord = pCurrentManage->GetConditionRecord(m_uRecordSetMarkID, iRecordID);
				if (NULL != pRecord)
				{
					pRecord->UpdateSetOperToDb(WARNINGSET_NULL);
				}
				iAddCount++;
			}
		}
	}
	return iAddCount;
}

// 从数据库中重新加载已经发生的历史
const int
CPushUser::ReloadTypePushHistoryFromDb(const unsigned int UserMapID,
	const unsigned char type, const unsigned int MarkCount, const unsigned int HaveSendMark)
{
	PushMsg tPushMsg;
	unsigned short iMaxCount = CPushMsgHistoryManage::GetMaxMsgCacheCount(m_cPlatformCode, type);
	unsigned int uAddCount = 0;
	unsigned int uExeCount = 0;
	if (MarkCount > 0)
	{
		uAddCount = MarkCount > iMaxCount ? iMaxCount : MarkCount;
		for (unsigned int i = MarkCount - 1; uExeCount < uAddCount && i >= 0; i--)
		{
			if (0 < DatabaseSnapshot::GetMain()->ReadHistory(GetUserID(),
				GetUserPlatform(), i, type, tPushMsg))
			{
				if (0 != CPushMsgHistoryManage::PushBackMsgHistory(m_cPlatformCode,
					UserMapID, i, HaveSendMark, tPushMsg))
				{
					break;
				}
				
			}
			uExeCount++;
		}
	}

	return uExeCount;
}

// 从数据库中重新加载已经发生的历史
const int
CPushUser::ReloadPushHistoryFromDb(const unsigned int UserMapID)
{
	int iCount = ReloadTypePushHistoryFromDb(UserMapID, PTE_EWARNING, m_iPushWarningDBNum, m_iPushWarningNum);
	iCount += ReloadTypePushHistoryFromDb(UserMapID, PTE_INFOMINE, m_iInfoMineDBNum, m_iInfoMineNum);
	iCount += ReloadTypePushHistoryFromDb(UserMapID, PTE_NOTICE, m_iPublicNoticeDBNum, m_iPublicNoticeNum);

	return iCount;
}

// 从数据库中移除无效的历史记录信息
void
CPushUser::DeleteTypeOutofDateHisMsg(const unsigned char type, const unsigned int MarkCount)
{
	unsigned short iMaxCount = CPushMsgHistoryManage::GetMaxMsgCacheCount(m_cPlatformCode, type);
	
	if (MarkCount > iMaxCount)
	{
		DatabaseSnapshot::GetSub()->DeleteUserHistroySmallerThanID(GetUserID(),
			GetUserPlatform(), MarkCount - iMaxCount, type);
	}
}

// 从计算队列当中移除
void
CPushUser::RemoveFromCalcQueue(const unsigned int UserMapID)
{
	CalOperatingThread  *pCurrentCalcThread = ThreadsManage::GetIndexCalcThread(GetUserThreadNum());
	if (NULL == pCurrentCalcThread)
	{
		ERROR("NULL pCurrentCalcThread[%s:%u]", GetUserID(), GetUserThreadNum());
		return;
	}
	pCurrentCalcThread->RemoveEWarningUser(UserMapID, GetUserPlatform());
	pCurrentCalcThread->RemoveActiveUser(UserMapID, GetUserPlatform());
}

// 获取特定缩影的title
const char*
CPushUser::GetIndexUserTiltleSeting(const int Index)
{
	return sTitle[Index];
}				

// 用户上传自选股
int
CPushUser::UploadSelfSelectStk(const unsigned int UserMapID, const unsigned char OperType,					
		const unsigned char RecvMark, const char **CodeList, const int CodeCount, const int ItemLen)
{
	int iRes = 0;
	CSelfSelectStock* pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);
	if (NULL == pSelfSelectStock)
	{
		iRes = -32;
		return iRes;
	}
	
	pSelfSelectStock->SetRecvMark(UserMapID, RecvMark != 0);
	pSelfSelectStock->SetModifiedMark(UserMapID, true);
	
	switch(OperType)
	{
		case USSOT_ADD:
			{
				for (int i = CodeCount - 1; i >= 0; i--)
				{
					iRes = pSelfSelectStock->AddSelfCode(UserMapID, (char*)CodeList + i * ItemLen);
					if (0 != iRes)
					{
						TRACE("AddSelfCode Result =%d Code=%s", iRes, (char*)CodeList + i * ItemLen);
					}
				}
			}
			break;
		
		case USSOT_REMOVE:
			{
				for (int i = CodeCount - 1; i >= 0; i--)
				{
					iRes = pSelfSelectStock->RemoveSelfCode(UserMapID, (char*)CodeList + i * ItemLen);
					if (0 != iRes)
					{
						TRACE("RemoveSelfCode Result =%d Code=%s", iRes, (char*)CodeList + i * ItemLen);
					}
				}
			}
			break;
			
		case USSOT_OVERLAP:
			{
				pSelfSelectStock->ClearAllSelfCode(UserMapID);
				for (int i = CodeCount - 1; i >= 0; i--)
				{
					iRes = pSelfSelectStock->AddSelfCode(UserMapID, (char*)CodeList + i * ItemLen);
					if (0 != iRes)
					{
						TRACE("AddSelfCode Result =%d Code=%s", iRes, (char*)CodeList + i * ItemLen);
					}
				}
			}
			break;
			
		default:
			break;
	}
	
	return iRes;
}

// 读取用户的自选股
int
CPushUser::TryLoadUserSelfStkList(const unsigned int UserMapID)
{
	int iRet = -1;
	
	// FIELD_MAX_LEN 512
	const int iMaxBufSize = 512;
	char cRecvBuf[iMaxBufSize + 1] = {0};
	iRet = DatabaseSnapshot::GetMain()->ReadUserSelfStock(GetUserID(),
				GetUserPlatform(), cRecvBuf, iMaxBufSize);
				
	if (iRet > 0)
	{
		CSelfSelectStock* pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);
		if (NULL == pSelfSelectStock)
		{
			iRet = -32;
			return iRet;
		}
		pSelfSelectStock->SetTotalSelfStkCode(UserMapID, cRecvBuf);
	}
	return iRet;
}					

// 最新同步到数据库
const int
CPushUser::SyncLatestToDb(const unsigned int UserMapID, const char *token)
{
	int iRet = -1;
	int iMatchRec = -1;

	// 同步预警记录
	if (PU_NO_VALID_EARLY_WARNING_SET == GetHaveSetPrivateWarningProperty()
		|| (unsigned int)-1 == m_uRecordSetMarkID)
	{
		DatabaseSnapshot::GetSub()->DeleteUserAllEWarning(GetUserID(), GetUserPlatform());
	}
	else
	{
		CPlatformEWarnManage *pCurrentManage = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
		if (NULL != pCurrentManage)
		{
			unsigned char iCount = GetCurrentUserPermitWarningNum(pCurrentManage);
			for (unsigned char i = 0; i < iCount; i++)
			{
				CEarlyWarningConditionRecord *pRecord = pCurrentManage->GetConditionRecord(m_uRecordSetMarkID, i);
				if (NULL == pRecord)
				{
					TRACE("pRecord=NULL[%u]", i);
					continue;
				}
				
				if (WARNINGSET_NULL == pRecord->GetOperToDb()
					|| IsEmptyString(pRecord->GetStkCode()))
				{
					continue;
				}
				
				iMatchRec = DatabaseSnapshot::GetSub()->FindUserEWarning(GetUserID(), GetUserPlatform(), i);
				if (0 == iMatchRec)
				{
					iRet = DatabaseSnapshot::GetSub()->InsertUserEWarning(GetUserID(), GetUserPlatform(), i, pRecord);
				}
				else if (0 < iMatchRec)
				{
					iRet = DatabaseSnapshot::GetSub()->UpdateUserEWarning(GetUserID(), GetUserPlatform(), i, pRecord);
				}
				else
				{
					DEBUG("ERROR in data state[%s %u %u]", GetUserID(), GetUserPlatform(), i);
					continue;
				}
				
				switch(pRecord->GetOperToDb())
				{
					case WARNINGSET_INSERTDB:
					case WARNINGSET_UPDATEDB:
						if (iRet > 0)
						{
							pRecord->UpdateSetOperToDb(WARNINGSET_NULL);
						}
						break;
					case WARNINGSET_DELDB:// 需要保持DELDB状态，并且不删除记录，只更新
						break;
						
					default:
						break;
				}
			}
		}
	}

	iRet = -1;
	// 同步历史记录
	unsigned int uStartSync = 0;
	if (m_iPushWarningTotal > m_iPushWarningDBNum)
	{
		for (uStartSync = m_iPushWarningDBNum; uStartSync < m_iPushWarningTotal; uStartSync++)
		{
			pPushMsg pHis = NULL;
			if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_EWARNING,
					uStartSync, pHis) < 0)
			{
				DEBUG("GetPushMsg PTE_EWARNING failed.[%u]", uStartSync);
				continue;
			}

			DatabaseSnapshot::GetSub()->InsertHistory(GetUserID(),
				GetUserPlatform(), uStartSync, pHis);
		}
		m_iPushWarningDBNum = uStartSync;
	}

	if (m_iInfoMineTotal > m_iInfoMineDBNum)
	{
		for (uStartSync = m_iInfoMineDBNum; uStartSync < m_iInfoMineTotal; uStartSync++)
		{
			pPushMsg pHis = NULL;
			if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_INFOMINE,
					uStartSync, pHis) < 0)
			{
				DEBUG("GetPushMsg PTE_INFOMINE failed.[%u]", uStartSync);
				continue;
			}

			DatabaseSnapshot::GetSub()->InsertHistory(GetUserID(),
				GetUserPlatform(), uStartSync, pHis);
		}
		m_iInfoMineDBNum = uStartSync;
	}

	if (m_iPublicNoticeTotal > m_iPublicNoticeDBNum)
	{
		for (uStartSync = m_iPublicNoticeDBNum; uStartSync < m_iPublicNoticeTotal; uStartSync++)
		{
			pPushMsg pHis = NULL;
			if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_NOTICE,
					uStartSync, pHis) < 0)
			{
				DEBUG("GetPushMsg PTE_NOTICE failed.[%u]", uStartSync);
				continue;
			}

			DatabaseSnapshot::GetSub()->InsertHistory(GetUserID(),
				GetUserPlatform(), uStartSync, pHis);
		}
		m_iPublicNoticeDBNum = uStartSync;
	}

	iRet = -1;
	// 同步用户信息
	unsigned short uCurrentProper = GetOperDbProperty();
	switch (uCurrentProper)
	{
		case PU_DB_ADD:
			iRet = DatabaseSnapshot::GetSub()->InsertNewUser(this, token);
			break;

		case PU_DB_UPDATE:
			iRet = DatabaseSnapshot::GetSub()->UpdateUser(this, token);
			break;

		case PU_DB_DEL:
			iRet = DatabaseSnapshot::GetSub()->DeleteUser(this);
			break;

		default:
			break;
	}

	if (iRet > 0)
	{
		SetOperDbProperty(PU_DB_NULL);
	}
	
	//  更新自选记录
	CSelfSelectStock* pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);
	if (NULL != pSelfSelectStock)
	{
		if (pSelfSelectStock->IsModified(UserMapID))
		{
			DatabaseSnapshot::GetSub()->UpdateUserSelfStock(GetUserID(),
				GetUserPlatform(), pSelfSelectStock->GetTotalSelfStkCode(UserMapID));
		}
	}

	return -1;
}

// 清除失效的历史记录信息
void 
CPushUser::DeleteOutofDateHisMsg()
{
	DeleteTypeOutofDateHisMsg(PTE_EWARNING, m_iPushWarningDBNum);
	DeleteTypeOutofDateHisMsg(PTE_INFOMINE, m_iInfoMineDBNum);
	DeleteTypeOutofDateHisMsg(PTE_NOTICE, m_iPublicNoticeDBNum);
}

// 重置自选股记录的状态
int
CPushUser::ResetSelfSelectStkState(const unsigned int UserMapID)
{
	CSelfSelectStock* pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);
	if (NULL != pSelfSelectStock)
	{
		pSelfSelectStock->Reset(UserMapID);
	}
	
	return 0;
}

// 执行自选股信息地雷的实时推送
const int
CPushUser::ProcessSelfStkInfoMineRT(const unsigned int UserMapID)
{
	CSelfSelectStock *pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);	
	PushMsg tPushMsg;
	unsigned long uCurrentCrc = pSelfSelectStock->GetLatestInfoMineCrc(UserMapID);
	
	IMmessage msg;
	while(0 <= CInfoMine::getInfoMinePointer()->GetLatestMsg(uCurrentCrc, msg))
	{
		uCurrentCrc = msg.MsgID;
		pSelfSelectStock->SetLatestInfoMineCrc(UserMapID, uCurrentCrc);
		
		if (0 > pSelfSelectStock->GetStkCodeIndex(UserMapID, msg.strStockCode))
		{
			continue;
		}
		
		// 形成记录
		if (!GenerateInfoMineMsg(msg.strStockCode, &msg, &tPushMsg))
		{
			DEBUG("GenerateInfoMineMsg error");
			continue;
		}

		// 记录历史
		if (!PushBackMsg(UserMapID, tPushMsg))
		{
			DEBUG("PushBackMsg error.");
			continue;
		}
	}
	
	return 0;
}

// 获取自选股统计格式字符控制
const char*
CPushUser::GetSeflSkkStatisFormat()
{
	return sSelfStkInfoMineStatis;
}

