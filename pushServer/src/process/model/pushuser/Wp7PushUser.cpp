#include "Wp7PushUser.h"
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
#include "../../quota/QuotesSnapshot.h"
#include "../ErrorMsg/ErrorMsg.h"
#include "../history/RespMsg.h"
#include "../history/PushMsgHistory.h"
#include "../history/PushMsgHistoryManage.h"
#include "../db/DatabaseSnapshot.h"
#include "../outerservpush/Push2WP.h"
#include "../../thread/SendOuterDataThread.h"
#include "../../thread/ThreadsManage.h"
#include "../db/PublicNews.h"
#include "../db/InfoMine.h"
#include "../selfstock/SelfSelectStockManage.h"
#include "../selfstock/SelfSelectStock.h"

using namespace std;

CWp7PushUser::CWp7PushUser(void)
	: CPushUser()
{
	bzero(m_strPushToken, MAX_WP_PUSH_TOKEN_LEN + 1);
}

CWp7PushUser::CWp7PushUser(const UserLogInfor *LogInfo, const unsigned int ExpandInfo)
	: CPushUser(LogInfo)
{
	m_uExpandInfoValue = ExpandInfo;
	bzero(m_strPushToken, MAX_WP_PUSH_TOKEN_LEN + 1);
}

CWp7PushUser::~CWp7PushUser(void)
{
}

// 设置推送令牌
int
CWp7PushUser::SetUserPushToken(const char *token)
{
	StrCopy(m_strPushToken, token, MAX_WP_PUSH_TOKEN_LEN);
	return 0;
}

// 获取推送令牌
void
CWp7PushUser::GetUserPushToken(char *token, const int MaxLen)
{
	StrCopy(token, m_strPushToken, MaxLen);
}

// 执行节点股价预警计算
int
CWp7PushUser::ExecuteStkPriceWarningCal(const unsigned int UserMapID)
{
	// 死亡用户无需再扫描
	const int iCurrentActiveP = GetActiveProperty();
	if (PU_DEAD == iCurrentActiveP)
	{
		return -1;
	}
	
	const int iMemCount = CEarlyWarningConditionRecord::GetRecordMemCount();
	int iMatchFlag = 0;
	struct PriceAlarm CurrentItem;
	PushMsg tPushMsg;
	
	if (PU_NO_VALID_EARLY_WARNING_SET == GetHaveSetPrivateWarningProperty())
	{
		return -2;
	}
	
	CPlatformEWarnManage *pCurrentEWarn = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentEWarn)
	{
		return -3;
	}
	
	short iTotalCount = pCurrentEWarn->GetEarlyWarningNum(m_uRecordSetMarkID);
	if (iTotalCount <= 0)
	{
		return 0;
	}
	
	if (!CanContinueExeScan(PTE_EWARNING))
	{
		return -4;
	}
	
	CEarlyWarningConditionRecord *pFind = NULL;
	short iExecuteCount = 0;
	short iChangeCount = 0;
	short iCurrentUserMaxPer = GetCurrentUserPermitWarningNum(pCurrentEWarn);
	for (short i = 0; iExecuteCount < iTotalCount && i < iCurrentUserMaxPer; i++)
	{
		pFind = pCurrentEWarn->GetConditionRecord(m_uRecordSetMarkID, i);
		if (NULL == pFind || WARNINGSET_DELDB == pFind->GetOperToDb())
		{
			continue;
		}
		
		iExecuteCount++;
		iMatchFlag = pFind->ExcuteCmp(&CurrentItem);
		if (0 == iMatchFlag)
		{
			continue;
		}
		
		for (int bit = 0; bit < iMemCount; bit++)
		{
			if (!IsBitSet(iMatchFlag, bit))
			{
				continue;
			}

			// 形成记录
			if (!GenerateEarlyWarningMsg(bit, &CurrentItem, i, pFind, &tPushMsg))
			{
				DEBUG("GenerateEarlyWarningMsg error");
				continue;
			}

			// 记录历史
			if (!PushBackMsg(UserMapID, tPushMsg))
			{
				DEBUG("PushBackMsg error.");
				continue;
			}
			iChangeCount++;
		}

		// 更新用户记录
		pFind->UpdateSetOperToDb(WARNINGSET_UPDATEDB);
	}
	
	return 0;
}

// 执行自选股新闻相关扫描
int
CWp7PushUser::ExecutePrivateNoticeScan(const unsigned int UserMapID)
{
	// 死亡用户无需再扫描
	const int iCurrentActiveP = GetActiveProperty();
	if (PU_DEAD == iCurrentActiveP)
	{
		return -1;
	}

	PushMsg tPushMsg;
	unsigned int uCurrentCrc = 0;
	
	if (PU_NO_VALID_EARLY_WARNING_SET == GetHaveSetPrivateWarningProperty())
	{
		return -2;
	}
	
	CPlatformEWarnManage *pCurrentEWarn = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentEWarn)
	{
		return -3;
	}
	
	short iTotalCount = pCurrentEWarn->GetEarlyWarningNum(m_uRecordSetMarkID);
	if (iTotalCount <= 0)
	{
		return 0;
	}

	if (!CanContinueExeScan(PTE_INFOMINE))
	{
		return -4;
	}
	
	CEarlyWarningConditionRecord *pFind = NULL;
	short iExecuteCount = 0;
	short iCurrentUserMaxPer = GetCurrentUserPermitWarningNum(pCurrentEWarn);
	short iChangeCount = 0;
	for (short i = 0; iExecuteCount < iTotalCount && i < iCurrentUserMaxPer; i++)
	{
		pFind = pCurrentEWarn->GetConditionRecord(m_uRecordSetMarkID, i);
		if (NULL == pFind || WARNINGSET_DELDB == pFind->GetOperToDb())
		{
			continue;
		}
		
		iExecuteCount++;
		
		if (0 == pFind->GetInforMineProper())
		{
			continue;
		}

		IMmessage msg;		
		uCurrentCrc = pFind->GetLatestInfoMineCrc();
		while ((unsigned int)-1 != 
			(uCurrentCrc = CInfoMine::getInfoMinePointer()->GetLatestMsg(pFind->GetStkCode(),
							uCurrentCrc, msg)))
		{
			pFind->UpdateSetLatestInfoMineCrc(uCurrentCrc);
			
			// 形成记录
			if (!GenerateInfoMineMsg(pFind->GetStkCode(), &msg, &tPushMsg))
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
			
			iChangeCount++;
		}
	}
	
	return 0;
}

// 执行公共新闻公告相关扫描
int
CWp7PushUser::ExecutePublicNoticeScan(const unsigned int UserMapID)
{
	// 死亡用户无需再扫描
	const int iCurrentActiveP = GetActiveProperty();
	if (PU_DEAD == iCurrentActiveP)
	{
		return -1;
	}
	
	unsigned int uCurrentCrc = 0;
	if (m_iPublicNoticeTotal > 0)
	{
		uCurrentCrc = CPushMsgHistoryManage::GetUserHisMsgCrc(m_cPlatformCode, UserMapID,
						PTE_NOTICE, m_iPublicNoticeTotal - 1);
	}
	else
	{
		uCurrentCrc = 0;
	}
	
	if ((unsigned int)-1 == uCurrentCrc)
	{
		DEBUG("GetUserHisMsgCrc error");
		return -2;
	}
	
	if (!CanContinueExeScan(PTE_NOTICE))
	{
		return -3;
	}
	
	if (0 != CPublicNews::getPublicNewsPointer()->GetCurrentStatus())
	{
		return -4;
	}
	
	short iChangeCount = 0;
	PNmessage msg;
	while ((unsigned int)-1 != 
		(uCurrentCrc = CPublicNews::getPublicNewsPointer()->GetLatestMsg(uCurrentCrc, this, msg)))
	{
		// 形成记录
		PushMsg tPushMsg;
		if (!GenerateNoticeMsg(&msg, &tPushMsg))
		{
			DEBUG("GenerateNoticeMsg error");
			continue;
		}
	
		// 记录历史
		if (!PushBackMsg(UserMapID, tPushMsg))
		{
			DEBUG("PushBackMsg error.");
			continue;
		}
		iChangeCount++;
	}

	return iChangeCount;
}

// 以Wp7的记录格式添加预警记录
bool
CWp7PushUser::AddWp7EWarningMsg(const PushMsg *pPushMsgRec, WPmessage *writer)
{
	// 生成消息
	char cReceiveBuf[256] = {0};
	const char *pMsg = NULL;
	CEarlyWarningConditionRecord*  pCurrentSet = NULL;
	string strCode("");
	string strName("");
	char cFormat[256] = {0};
	unsigned char Precision = 2;
	
	CPlatformEWarnManage *pCurrentEWarn = CEwarningManage::GetPlatFormEWarn(m_cPlatformCode);
	if (NULL == pCurrentEWarn)
	{
		return false;
	}

	// 记录条件
	pCurrentSet = (CEarlyWarningConditionRecord*)pCurrentEWarn->GetConditionRecord(m_uRecordSetMarkID, pPushMsgRec->SettingRecordID);
	if (NULL == pCurrentSet || WARNINGSET_DELDB == pCurrentSet->GetOperToDb())
	{
		// 尝试从最新动态行情数据中查找
		unsigned short uFindIndex = 0xFFFF;
		uFindIndex = CQuotesSnapshot::findSecurityIndex(pPushMsgRec->strStkCode);
		if (0xFFFF == uFindIndex)
		{
			DEBUG("scan stock code faild[%s]", pPushMsgRec->strStkCode);
			return false;
		}
		PriceAlarm *pCurrentAlarm = CQuotesSnapshot::GetIndexSecurity(uFindIndex, pPushMsgRec->strStkCode);
		if (NULL == pCurrentAlarm)
		{
			return false;
		}
		
		strCode = pPushMsgRec->strStkCode;
		strName = pCurrentAlarm->m_strName;
		Precision = pCurrentAlarm->m_cPrecision;
	}
	else 
	{
		strCode = pCurrentSet->GetStkCode();
		if (NULL == pCurrentSet->GetStkName())
		{
			DEBUG("NULL stk name");
			return false;
		}
		strName = pCurrentSet->GetStkName();
		Precision = pCurrentSet->GetFloatPrecision();
	}
	
	// 记录格式
	const char *pFormat = GetEalyMsgFormat(pPushMsgRec, Precision, cFormat, 256, 1);
	if (NULL == pFormat)
	{
		DEBUG("NULL == pFormat");
		return false;
	}
	
	pMsg = ChangeToTotalEarlyMsg(pFormat, pPushMsgRec, cReceiveBuf, 256, strCode, strName,Precision);

	if (NULL == pMsg)
	{
		DEBUG("NULL == pMsg");
		return false;
	}

	strncpy(writer->text1, strName.c_str(), WPL_PARAM1_LEN);
	strncpy(writer->text2, pMsg, WPL_PARAM2_LEN);
	strncpy(writer->stockcode, strCode.c_str(), MAX_STKCODE_LEN);
	writer->displaytype = 1;
	writer->notifytype = PTE_EWARNING;

    return true;
}

// 以Wp7的记录格式生成公共消息记录
bool
CWp7PushUser::AddWp7PublicNoticeMsg(const PushMsg *pPushMsgRec, WPmessage *writer)
{
	if (NULL == pPushMsgRec)
	{
		DEBUG("MsgRecord = NULL");
		return false;
	}
	
	//  查找消息
	PNmessage MsgRecord;
	if (!CPublicNews::getPublicNewsPointer()->GetIndexPNmessage(pPushMsgRec->SettingRecordID,
			MsgRecord))
	{
		DEBUG("getPNmsgByID failed[%u]", pPushMsgRec->SettingRecordID);
		return false;
	}
	
	// 生成消息
	const char *pMsg = NULL;
	string strParam("");
	char cReceiveBuf[256] = {0};
	
	pMsg = ChangeToPublicMsg(&MsgRecord, cReceiveBuf, 256);
	unsigned char cType = MsgRecord.PublicNoticeType;
	if (NOTICE_NORMAL == cType)
	{
		bzero(writer->url, WPL_URL_LEN);
	}
	else if (NOTICE_EXTENSION == cType)
	{
		strncpy(writer->url, MsgRecord.strURL, WPL_URL_LEN);
	}
	else
	{
		ERROR("nodefined type");
		return false;
	}
	
	strncpy(writer->text1, "公告", WPL_PARAM1_LEN);
	strncpy(writer->text2, pMsg, WPL_PARAM2_LEN);
	writer->displaytype = 1;
	writer->notifytype = PTE_NOTICE;
	return true;
}

// 以Wp7的记录格式生成信息地雷记录
bool
CWp7PushUser::AddWp7MineMsg(const PushMsg *pPushMsgRec, WPmessage *writer)
{
	// 生成消息
	char cReceiveBuf[256] = {0};
	const char *pMsg = NULL;
	string strCode("");
	string strName("");
	char cCountCache[8] = {0};

	// 地雷
	if (PTE_INFOMINE == pPushMsgRec->MsgType)
	{
		IMmessage InfoMineMsg;
		if (INFOMINE_NORMAL == pPushMsgRec->MsgSubType)
		{
			if (!CInfoMine::getInfoMinePointer()->GetIndexIMmessage(pPushMsgRec->strStkCode, pPushMsgRec->SettingRecordID, InfoMineMsg))
			{
				DEBUG("No record or removed");
				return false;
			}
					
			// 尝试从最新动态行情数据中查找
			unsigned short uFindIndex = 0xFFFF;
			uFindIndex = CQuotesSnapshot::findSecurityIndex(pPushMsgRec->strStkCode);
			if (0xFFFF == uFindIndex)
			{
				DEBUG("scan stock code faild[%s]", pPushMsgRec->strStkCode);
				return false;
			}
			
			ACC_STK_STATIC *pStaticStk = CQuotesSnapshot::GetIndexStatic(uFindIndex, pPushMsgRec->strStkCode);
			strCode = pPushMsgRec->strStkCode;
			strName = pStaticStk->m_szName;
			
			pMsg = ChangeToInfoMineMsg(&InfoMineMsg, cReceiveBuf, 256);
		}
		else if (INFOMINE_STATISTIC == pPushMsgRec->MsgSubType)
		{
			InfoMineMsg.ttNewsTime = pPushMsgRec->DateTime;
			snprintf(InfoMineMsg.strTitle, INFOMINE_MAXTITLELEN, GetSeflSkkStatisFormat(), pPushMsgRec->LatestValue);
			pMsg = ChangeToInfoMineMsg(&InfoMineMsg, cReceiveBuf, 256);
			snprintf(cCountCache, 8, "%d", pPushMsgRec->LatestValue);
			strName = cCountCache;
		}
	}

	if (NULL == pMsg)
	{
		DEBUG("NULL == pMsg");
		return false;
	}

	strncpy(writer->text1, strName.c_str(), WPL_PARAM1_LEN);
	strncpy(writer->text2, pMsg, WPL_PARAM2_LEN);
	strncpy(writer->stockcode, strCode.c_str(), MAX_STKCODE_LEN);
	writer->displaytype = 1;
	writer->notifytype = PTE_INFOMINE;
	
    return true;
}

// 处理306请求
int
CWp7PushUser::ParseReq306(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	short iStrLength = 0;
	unsigned char cMark = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	char strToken[MAX_WP_PUSH_TOKEN_LEN] = {0};
	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto RESULT;
	}
	
	// 取推送令牌
	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	iStrLength = reader.ReadShort();
	if (iStrLength < 0 || (short)iStrLength > (short)reader.GetUnReadLen())
	{
		iRes = -2;
		goto CONDITION_ERROR;
	}
	reader.ReadString(iStrLength, strToken, MAX_WP_PUSH_TOKEN_LEN);
	iRes = SetUserPushToken(strToken);
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

// 处理1000请求
int
CWp7PushUser::ParseReq1000(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	unsigned short iStrLength = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	unsigned char cReqMark = 0;
	char cResMark = 0;

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto VERIRY_ERROR;
	}

	if (sizeof(char) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	cReqMark = reader.ReadByte();

	cResMark = 0;
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(char) + sizeof(char) + sizeof(CWp7PushUser);
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cResMark, sizeof(char));
	writer->Push_back(&pUlf->m_cPlatformCode, sizeof(char));
	writer->Push_back(this, sizeof(CWp7PushUser));
	
	return 0;


VERIRY_ERROR:
	iRes = iRes * -1;
	pErrorMsg = GetPushErrorMsg(iRes);
	cResMark = 1;
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short) + iStrLength;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cResMark, sizeof(char));
	writer->Push_back(&iRes, sizeof(int));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return 0;

CONDITION_ERROR:
	pErrorMsg = GetPushErrorMsg(1);
	iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
	memcpy(&subHead, pCmdHead, sizeof(sub_head));
	subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short) + iStrLength;

	cResMark = 1;
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cResMark, sizeof(char));
	iRes = 1;
	writer->Push_back(&iRes, sizeof(int));
	writer->Push_back(&iStrLength, sizeof(short));
	if (iStrLength > 0)
	{
		writer->Push_back((char*)pErrorMsg, iStrLength);
	}
	return iRes;
}

// 检查用户节点的运行状态
const int
CWp7PushUser::CheckUserAliveStatus(const time_t NowTime)
{
	int iRes = LongItvlCheckUserAlive(NowTime);
	return iRes;
}

// 处理用户请求
int
CWp7PushUser::ProcessUserRequest(const sub_head *pCmdHead,  const UserLogInfor *pUlf, void *data,
						const unsigned int length, CBuffWriter *writer)
{
	if (NULL == pCmdHead || NULL == data)
	{
		DEBUG("Error cmd or null data error.");
		return -1;
	}
	
	// 更新活动时间
	UpdateLastestAliveTime(pCmdHead);
	if (305 != pCmdHead->sub_type)
	{
		TRACE("Request = %d platform = %d UserID = %s",
				pCmdHead->sub_type, pUlf->m_cPlatformCode, m_strUserID);
	}
	
	switch(pCmdHead->sub_type)
	{
		// 推送令牌
		case ST_TOKEN:
			return ParseReq306(pCmdHead, pUlf, data, length, writer);
			
		case ST_GET_LASTEST_USER_DATA:
			return ParseReq1000(pCmdHead, pUlf, data, length, writer);

		default:
			return CPushUser::ProcessUserRequest(pCmdHead, pUlf, data, length, writer);
	}

	return -1;
}

// 发送预警消息
const int 
CWp7PushUser::SendEWarningMsg(const unsigned int UserMapID)
{
	if (m_iPushWarningNum >= m_iPushWarningTotal)
	{
		return -1;
	}
	
	WPmessage wp7Msg;
	CSendOuterDataThread *pCurrentThread = ThreadsManage::GetOutSendThread();
	if (IsEmptyString(m_strPushToken) || NULL == pCurrentThread)
	{
		return -2;
	}
	
	int iSendCount = 0;
	pPushMsg pHis = NULL;
	unsigned int uCurrentPushCrc = 0;
	for (unsigned int i = m_iPushWarningNum; i < m_iPushWarningTotal; i++)
	{
		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_EWARNING,
			i, pHis) < 0)
		{
			WARN("User[%u] ewarning[id=%u] jumped.", UserMapID, i);
			uCurrentPushCrc = i;
			continue;
		}
		
		bzero(&wp7Msg, sizeof(WPmessage));
		if (!AddWp7EWarningMsg(pHis, &wp7Msg))
		{
			DEBUG("AddWp7EWarningMsg failed.");
			break;
		}
		uCurrentPushCrc = i;
		
		if (0 == pCurrentThread->UserExecuteSend(m_cPlatformCode, m_strPushToken, &wp7Msg))
		{
			iSendCount++;
			DEBUG("User[%s] send wp7 push msg", GetUserID());
			
			// 更新预警发送标识
			m_iPushWarningNum = uCurrentPushCrc + 1;
		}
		else 
		{
			TRACE("Send wp7 Push error");
		}
	}
	
	return 0;
}

// 发送信息地雷
const int
CWp7PushUser::SendInfoMineMsg(const unsigned int UserMapID)
{
	if (m_iInfoMineNum >= m_iInfoMineTotal)
	{
		return -1;
	}
	
	WPmessage wp7Msg;
	CSendOuterDataThread *pCurrentThread = ThreadsManage::GetOutSendThread();
	if (IsEmptyString(m_strPushToken) || NULL == pCurrentThread)
	{
		return -2;
	}
	
	int iSendCount = 0;
	pPushMsg pHis = NULL;
	unsigned int uCurrentPushCrc = 0;
	for (unsigned int i = m_iInfoMineNum; i < m_iInfoMineTotal; i++)
	{
		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_INFOMINE,
			i, pHis) < 0)
		{
			WARN("User[%u] InfoMine[id=%u] jumped.", UserMapID, i);
			uCurrentPushCrc = i;
			continue;
		}
		
		bzero(&wp7Msg, sizeof(WPmessage));
		if (!AddWp7MineMsg(pHis, &wp7Msg))
		{
			DEBUG("AddWp7MineMsg failed.");
			break;
		}
		uCurrentPushCrc = i;
		
		if (0 == pCurrentThread->UserExecuteSend(m_cPlatformCode, m_strPushToken, &wp7Msg))
		{
			iSendCount++;
			DEBUG("User[%s] send wp7 InfoMine msg", GetUserID());
			
			// 更新发送标识
			m_iInfoMineNum = uCurrentPushCrc + 1;
		}
		else 
		{
			TRACE("Send wp7 InfoMine error");
		}
	}
	
	return 0;
}

// 发送公告
const int 
CWp7PushUser::SendNoticeMsg(const unsigned int UserMapID)
{
	if (m_iPublicNoticeNum >= m_iPublicNoticeTotal)
	{
		return -1;
	}
	
	WPmessage wp7Msg;
	CSendOuterDataThread *pCurrentThread = ThreadsManage::GetOutSendThread();
	if (IsEmptyString(m_strPushToken) || NULL == pCurrentThread)
	{
		return -2;
	}
	
	int iSendCount = 0;
	pPushMsg pHis = NULL;
	unsigned int uCurrentPushCrc = 0;
	for (unsigned int i = m_iPublicNoticeNum; i < m_iPublicNoticeTotal; i++)
	{		
		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_NOTICE,
			i, pHis) < 0)
		{
			WARN("User[%u] Notice [id=%u] jumped.", UserMapID, i);
			uCurrentPushCrc = i;
			continue;
		}
		
		bzero(&wp7Msg, sizeof(WPmessage));
		if (!AddWp7PublicNoticeMsg(pHis, &wp7Msg))
		{
			DEBUG("AddWp7PublicNoticeMsg failed.");
			break;
		}
		uCurrentPushCrc = i;
		
		if (0 == pCurrentThread->UserExecuteSend(m_cPlatformCode, m_strPushToken, &wp7Msg))
		{
			iSendCount++;
			DEBUG("User[%s] send wp7 notice msg", GetUserID());
			
			// 更新发送标识
			m_iPublicNoticeNum = uCurrentPushCrc + 1;
		}
		else 
		{
			TRACE("Send wp7 notice error");
		}
	}
	
	return 0;
}

// 发送消息
int
CWp7PushUser::SendOuterMsg(const unsigned int UserMapID, const unsigned char MsgType)
{
	switch(MsgType)
	{
		case PTE_INFOMINE:
			return SendInfoMineMsg(UserMapID);
			
		case PTE_EWARNING:
			return SendEWarningMsg(UserMapID);
			
		case PTE_NOTICE:
			return SendNoticeMsg(UserMapID);
			
		default:
			break;
	}
	
	return -6;
}

// 执行发送
int
CWp7PushUser::ExecuteSend(const unsigned int UserMapID, const unsigned char PushType)
{	
	if (!HaveDataSend(PushType))
	{
		return -1;
	}
	
	CSendOuterDataThread *pCurrentThread = ThreadsManage::GetOutSendThread();
	if (NULL == pCurrentThread)
	{
		return -2;
	}
	
	const int iCurrentActiveP = GetActiveProperty();
	if (PU_DEAD == iCurrentActiveP)
	{
		return -3;
	}
	
	int iRes = -1;
	switch(PushType)
	{
		case PTE_EWARNING:
			{
				iRes = pCurrentThread->PushToSendQueue(m_cPlatformCode, UserMapID, PTE_EWARNING);
			}
			break;
			
		case PTE_INFOMINE:
			{
				iRes = pCurrentThread->PushToSendQueue(m_cPlatformCode, UserMapID, PTE_INFOMINE);
			}
			break;
			
		case PTE_NOTICE:
			{
				iRes = pCurrentThread->PushToSendQueue(m_cPlatformCode, UserMapID, PTE_NOTICE);
			}
			break;
			
		default:
			break;
	}
	
	if (0 == iRes)
	{
		// 更新用户信息
		SetOperDbProperty(PU_DB_UPDATE);
	}
	
	return iRes;
}

// 执行自选股信息地雷扫描
const int
CWp7PushUser::ExecuteSelfStkInfoMine(const unsigned int UserMapID)
{
	// 死亡用户无需再扫描
	const int iCurrentActiveP = GetActiveProperty();
	if (PU_DEAD == iCurrentActiveP)
	{
		return -1;
	}

	CSelfSelectStock *pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);
	if (NULL == pSelfSelectStock)
	{
		return -2;
	}

	const int iRecCount = pSelfSelectStock->GetSelfCodeCount(UserMapID);
	if (iRecCount < 0)
	{
		return -3;
	}
	else if (0 == iRecCount)
	{
		return 0;
	}

	if (!pSelfSelectStock->IsRecvMarkSet(UserMapID))
	{
		return 0;
	}

	if (!CanContinueExeScan(PTE_INFOMINE))
	{
		return -4;
	}
	
	if (0 != CInfoMine::getInfoMinePointer()->GetCurrentStatus())
	{
		return -5;
	}
	
	int iRes = ProcessSelfStkInfoMineStatis(UserMapID);
	if (0 == iRes)
	{
		return ProcessSelfStkInfoMineRT(UserMapID);
	}
	
	return iRes;
}

// 执行自选股信息地雷统计
const int
CWp7PushUser::ProcessSelfStkInfoMineStatis(const unsigned int UserMapID)
{
	CSelfSelectStock *pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);
	const int iRecCount = pSelfSelectStock->GetSelfCodeCount(UserMapID);
	char cStkCode[MAX_STKCODE_LEN + 1] = {0};
	unsigned int iTotalInfoMineCount = 0;
	unsigned int uStatisCount = 0;
	
	// 已经发送过不必扫描
	if (pSelfSelectStock->GetHaveSendTimes(UserMapID)
		 >= (int)GetSelfSelectStockInfomineConfig()->InfoMineStatisTimes)
	{
		return 0;
	}
	
	for (int i = 0; i < iRecCount; i++)
	{
		if (NULL == pSelfSelectStock->ReadSelfIndexCode(UserMapID, i, cStkCode, MAX_STKCODE_LEN))
		{
			DEBUG("ReadSelfIndexCode failed [UserMapID=%u index=%d]", UserMapID, i);
			return -1;
		}

		iTotalInfoMineCount += CInfoMine::getInfoMinePointer()->GetStockInfoMineCount(TrimRude(cStkCode));
	}

	pSelfSelectStock->SetInfomineTotalCount(UserMapID, iTotalInfoMineCount);
	if (0 == iTotalInfoMineCount)
	{
		goto END;
	}

	uStatisCount = iTotalInfoMineCount;
	if (pSelfSelectStock->HaveReadMark(UserMapID))
	{
		if (iTotalInfoMineCount <= (unsigned int)pSelfSelectStock->GetStaticsCount(UserMapID))
		{
			uStatisCount = 0;
		}
		else
		{
			uStatisCount = iTotalInfoMineCount - pSelfSelectStock->GetStaticsCount(UserMapID);
		}
	}

	if (uStatisCount > 0)
	{		
		// 形成记录
		PushMsg tPushMsg;
		if (!GenerateInfoMineStatisMsg(uStatisCount, &tPushMsg))
		{
			DEBUG("GenerateInfoMineMsg error");
			return -3;
		}

		// 记录历史
		if (!PushBackMsg(UserMapID, tPushMsg))
		{
			DEBUG("PushBackMsg error.");
			return -4;
		}
		
		pSelfSelectStock->SetStaticsCount(UserMapID, uStatisCount);
		goto END;
	}
	
	goto END;
	
END:
	pSelfSelectStock->SetHaveReadMark(UserMapID, false);
	pSelfSelectStock->SetHaveSendTimes(UserMapID, GetSelfSelectStockInfomineConfig()->InfoMineStatisTimes);
	return 0;
}

