#include "AndroidPushUser.h"
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
#include "../db/DatabaseSnapshot.h"
#include "../outerservpush/AppPushNotice.h"
#include "../outerservpush/Push2WP.h"
#include "../../thread/CalOperatingThread.h"
#include "../../thread/ThreadsManage.h"
#include "../../thread/NetThread.h"
#include "../db/PublicNews.h"
#include "../db/InfoMine.h"
#include "../selfstock/SelfSelectStockManage.h"
#include "../selfstock/SelfSelectStock.h"
#include "../loguser/Loguser.h"

using namespace std;

#pragma pack(1)
typedef struct ReqLatestMsgTag
{
	unsigned char MsgType;
	unsigned int ReqID;
	unsigned char ReqExt;
};

typedef struct ResLatestMsgTag
{
	// 消息类型
	unsigned char MsgType;
	// 消息子类型
	unsigned char MsgSubType;
	// 消息ID
	unsigned int ResID;
	// 代码
    char StkCode[MAX_STKCODE_LEN + 1];
    // 股票名称
    char StkName[MAX_STKNAME + 1];
    // 附加参数
    char ExtParam[MAXURLLEN + 1];
    // 消息内容
    char MsgContents[MAX_SINGLE_MSG + 1];
};
#pragma pack()

CAndroidPushUser::CAndroidPushUser(void)
	: CPushUser()
{
}

CAndroidPushUser::CAndroidPushUser(const UserLogInfor *LogInfo, const unsigned int ExpandInfo)
	: CPushUser(LogInfo)
{
	m_uExpandInfoValue = ExpandInfo;
}

CAndroidPushUser::~CAndroidPushUser(void)
{

}

// 执行节点股价预警计算
int
CAndroidPushUser::ExecuteStkPriceWarningCal(const unsigned int UserMapID)
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
	CEarlyWarningConditionRecord *pFind = NULL;

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

// 执行预警记录信息地雷扫描
int
CAndroidPushUser::ExecuteStkInfoMineScan(const unsigned int UserMapID)
{
	// 死亡用户无需再扫描
	const int iCurrentActiveP = GetActiveProperty();
	if (PU_DEAD == iCurrentActiveP)
	{
		return -1;
	}

	PushMsg tPushMsg;
	CEarlyWarningConditionRecord *pFind = NULL;
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
CAndroidPushUser::ExecutePublicNoticeScan(const unsigned int UserMapID)
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

// 执行发送
int
CAndroidPushUser::ExecuteSend(const unsigned int UserMapID, const unsigned char PushType)
{
	const int iCurrentActiveP = GetActiveProperty();
	if (!HaveDataSend(PushType))
	{
		return -1;
	}

	HqLinkNode *pBelongHqLink = ThreadsManage::GetNetThread()->GetHqLinkNode(GetUserHqLinkIndex());
	if (NULL == pBelongHqLink)
	{
		return -2;
	}

	int iRes = -1;
	switch(PushType)
	{
		case PTE_EWARNING:
			{
				if (PU_ACTIVE == iCurrentActiveP || PU_RELIVE == iCurrentActiveP)
				{
					iRes = pBelongHqLink->PushToSendQueue(m_cPlatformCode, UserMapID, PTE_EWARNING);
				}
			}
			break;

		case PTE_INFOMINE:
			{
				if (PU_VALID_EARLY_WARNING_SET == GetHaveSetPrivateWarningProperty()
					&& (PU_ACTIVE == iCurrentActiveP || PU_RELIVE == iCurrentActiveP))
				{
					iRes = pBelongHqLink->PushToSendQueue(m_cPlatformCode, UserMapID, PTE_INFOMINE);
				}
			}
			break;

		case PTE_NOTICE:
			{
				if (PU_VALID_EARLY_WARNING_SET == GetHaveSetPrivateWarningProperty()
					&& (PU_ACTIVE == iCurrentActiveP || PU_RELIVE == iCurrentActiveP))
				{
					iRes = pBelongHqLink->PushToSendQueue(m_cPlatformCode, UserMapID, PTE_NOTICE);
				}
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

// 发送消息
int
CAndroidPushUser::SendOuterMsg(const unsigned int UserMapID, const unsigned char MsgType)
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

// 获取线程回复资源
LargeMemoryCache*
CAndroidPushUser::GetThreadResponeRes()
{
	CalOperatingThread *pCurrentThread = ThreadsManage::GetIndexCalcThread(GetUserThreadNum());
	if (NULL == pCurrentThread)
	{
		return NULL;
	}

	return pCurrentThread->GetResponseCache();
}

// 发送预警消息
const int
CAndroidPushUser::SendEWarningMsg(const unsigned int UserMapID)
{
	if (m_iPushWarningNum >= m_iPushWarningTotal)
	{
		return -1;
	}

	// 申请内存
	LargeMemoryCache *pSendBuf = GetThreadResponeRes();
	if (NULL == pSendBuf)
	{
		DEBUG("GetThreadResponeRes failed.");
		return -2;
	}
	int iBufLen = 0;
	char *pBuf = pSendBuf->GetRemainMem(iBufLen);
	if (0 != pSendBuf->GetLatestWarning())
	{
		WARN("User Send buffer maybe not enough.");
	}

	if (NULL == pBuf)
	{
		TRACE("no memory to receive msg");
		pSendBuf->ClearAll();
		return -3;
	}
	CBuffWriter writer(pBuf, iBufLen, 0);
	// 预留出头的空间
	writer.SeekPos(sizeof(ACC_CMDHEAD));

	pPushMsg pHis = NULL;
	unsigned int uCurrentPushCrc = 0;
	for (unsigned int i = m_iPushWarningNum; i < m_iPushWarningTotal; i++)
	{
		if (!writer.CanHold(256))
		{
			break;
		}

		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_EWARNING,
			i, pHis) < 0)
		{
			WARN("User[%u] ewarning[id=%u] jumped.", UserMapID, i);
			uCurrentPushCrc = i;
			continue;
		}

		if (!AddAndroidEWarningMsg(UserMapID, uCurrentPushCrc, pHis, &writer))
		{
			DEBUG("AddAndroidEWarningMsg failed.");
			break;
		}
		uCurrentPushCrc = i;
	}

	HqLinkNode *pBelongHqLink = ThreadsManage::GetNetThread()->GetHqLinkNode(GetUserHqLinkIndex());
	if (writer.GetNewdataLen() <= 0 || NULL == pBelongHqLink)
	{
		pSendBuf->ClearUsedMem();
		return -4;
	}

	// 发送消息
	GenerateSendRecord(pSendBuf, &writer);
	if (pBelongHqLink->SendPushData(pSendBuf->GetRawMemPointer(0),
		pSendBuf->GetUsedMemoryLen()))
	{
		DEBUG("[user=%s]Send push msg, len=%u", GetUserID(), writer.GetNewdataLen());
		// 更新预警发送标识
		m_iPushWarningNum = uCurrentPushCrc + 1;

		pSendBuf->ClearUsedMem();
		return 0;
	}

	pSendBuf->ClearUsedMem();
	return -5;
}

// 发送信息地雷
const int
CAndroidPushUser::SendInfoMineMsg(const unsigned int UserMapID)
{
	if (m_iInfoMineNum >= m_iInfoMineTotal)
	{
		return -1;
	}

	// 申请内存
	LargeMemoryCache *pSendBuf = GetThreadResponeRes();
	if (NULL == pSendBuf)
	{
		DEBUG("GetThreadResponeRes failed.");
		return -2;
	}
	int iBufLen = 0;
	char *pBuf = pSendBuf->GetRemainMem(iBufLen);
	if (0 != pSendBuf->GetLatestWarning())
	{
		WARN("User Send buffer maybe not enough.");
	}

	if (NULL == pBuf)
	{
		TRACE("no memory to receive msg");
		pSendBuf->ClearAll();
		return -3;
	}
	CBuffWriter writer(pBuf, iBufLen, 0);
	// 预留出头的空间
	writer.SeekPos(sizeof(ACC_CMDHEAD));

	pPushMsg pHis = NULL;
	unsigned int uCurrentPushCrc = 0;
	for (unsigned int i = m_iInfoMineNum; i < m_iInfoMineTotal; i++)
	{
		if (!writer.CanHold(256))
		{
			break;
		}

		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_INFOMINE,
			i, pHis) < 0)
		{
			WARN("User[%u] InfoMine[id=%u] jumped.", UserMapID, i);
			uCurrentPushCrc = i;
			continue;
		}

		if (!AddAndroidMineMsg(UserMapID, i, pHis, &writer))
		{
			DEBUG("AddAndroidMineMsg failed.");
			break;
		}
		uCurrentPushCrc = i;
	}

	HqLinkNode *pBelongHqLink = ThreadsManage::GetNetThread()->GetHqLinkNode(GetUserHqLinkIndex());
	if (writer.GetNewdataLen() <= 0 || NULL == pBelongHqLink)
	{
		pSendBuf->ClearUsedMem();
		return -4;
	}

	// 发送消息
	GenerateSendRecord(pSendBuf, &writer);
	if (pBelongHqLink->SendPushData(pSendBuf->GetRawMemPointer(0),
		pSendBuf->GetUsedMemoryLen()))
	{
		DEBUG("[user=%s]Send InfoMine msg, len=%u", GetUserID(), writer.GetNewdataLen());
		// 更新发送标识
		m_iInfoMineNum = uCurrentPushCrc + 1;

		pSendBuf->ClearUsedMem();
		return 0;
	}

	pSendBuf->ClearUsedMem();
	return -5;
}

// 发送公告
const int
CAndroidPushUser::SendNoticeMsg(const unsigned int UserMapID)
{
	if (m_iPublicNoticeNum >= m_iPublicNoticeTotal)
	{
		return -1;
	}

	// 申请内存
	LargeMemoryCache *pSendBuf = GetThreadResponeRes();
	if (NULL == pSendBuf)
	{
		DEBUG("GetThreadResponeRes failed.");
		return -2;
	}
	int iBufLen = 0;
	char *pBuf = pSendBuf->GetRemainMem(iBufLen);
	if (0 != pSendBuf->GetLatestWarning())
	{
		WARN("User Send buffer maybe not enough.");
	}

	if (NULL == pBuf)
	{
		TRACE("no memory to receive msg");
		pSendBuf->ClearAll();
		return -3;
	}
	CBuffWriter writer(pBuf, iBufLen, 0);
	// 预留出头的空间
	writer.SeekPos(sizeof(ACC_CMDHEAD));

	pPushMsg pHis = NULL;
	unsigned int uCurrentPushCrc = 0;
	for (unsigned int i = m_iPublicNoticeNum; i < m_iPublicNoticeTotal; i++)
	{
		if (!writer.CanHold(256))
		{
			break;
		}

		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_NOTICE,
			i, pHis) < 0)
		{
			WARN("User[%u] Notice [id=%u] jumped.", UserMapID, i);
			uCurrentPushCrc = i;
			continue;
		}

		if (!AddAndroidPublicNoticeMsg(UserMapID, uCurrentPushCrc, pHis, &writer))
		{
			DEBUG("AddAndroidPublicNoticeMsg failed.");
			break;
		}
		uCurrentPushCrc = i;
	}

	HqLinkNode *pBelongHqLink = ThreadsManage::GetNetThread()->GetHqLinkNode(GetUserHqLinkIndex());
	if (writer.GetNewdataLen() <= 0 || NULL == pBelongHqLink)
	{
		pSendBuf->ClearUsedMem();
		return -4;
	}

	// 发送消息
	GenerateSendRecord(pSendBuf, &writer);
	if (pBelongHqLink->SendPushData(pSendBuf->GetRawMemPointer(0),
		pSendBuf->GetUsedMemoryLen()))
	{
		DEBUG("[user=%s]Send Notice msg, len=%u", GetUserID(), writer.GetNewdataLen());
		// 更新发送标识
		m_iPublicNoticeNum = uCurrentPushCrc + 1;

		pSendBuf->ClearUsedMem();
		return 0;
	}

	pSendBuf->ClearUsedMem();
	return -5;
}

// 以android的记录格式生成预警记录
bool
CAndroidPushUser::AddAndroidEWarningMsg(const unsigned int UserMapID,
	const unsigned int MsgID, const PushMsg *pPushMsgRec, CBuffWriter *writer)
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
	const char *pFormat = GetEalyMsgFormat(pPushMsgRec, Precision, cFormat, 256);
	if (NULL == pFormat)
	{
		DEBUG("NULL == pFormat");
		return false;
	}

	pMsg = ChanageToPartEarlyMsg(pFormat, pPushMsgRec, cReceiveBuf, 256, Precision);

	if (NULL == pMsg)
	{
		DEBUG("NULL == pMsg");
		return false;
	}
	short sLen = 0;
	// sub Head
	sub_head SubHead;
	SubHead.sub_type = (unsigned short)ST_PUSH;
	SubHead.sub_attrs = 0;
	SubHead.sub_extend = UserMapID;
	SubHead.sub_length = sizeof(int)
		+ sizeof(short) + strCode.size()
		+ sizeof(short) + strName.size()
		+ sizeof(short) + sLen;

	writer->Push_back(&SubHead, sizeof(sub_head));

	int iMsgID = MsgID;
	writer->Push_back(&iMsgID, sizeof(int));

	sLen = strCode.size();
	writer->Push_back(&sLen, sizeof(short));
	writer->Push_back((void*)strCode.c_str(), sLen);

	sLen = strName.size();
	writer->Push_back(&sLen, sizeof(short));
	writer->Push_back((void*)strName.c_str(), sLen);

	sLen = strlen(pMsg);
	writer->Push_back(&sLen, sizeof(short));
	writer->Push_back((void*)pMsg, sLen);
	return true;
}

// 以android的记录格式生成信息地雷记录
bool
CAndroidPushUser::AddAndroidMineMsg(const unsigned int UserMapID,
	const unsigned int MsgID, const PushMsg *pPushMsgRec, CBuffWriter *writer)
{
	// 生成消息
	char cReceiveBuf[256] = {0};
	const char *pMsg = NULL;
	string strCode("");
	string strName("");
	string strParam("");
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
			snprintf(cCountCache, 8, "%d", pPushMsgRec->LatestValue);
			strParam = cCountCache;
			pMsg = ChangeToInfoMineMsg(&InfoMineMsg, cReceiveBuf, 256);
		}
	}

	if (NULL == pMsg)
	{
		DEBUG("NULL == pMsg");
		return false;
	}
	short sLen = 0;

	TRACE("pMsg=%s", pMsg);
	// sub Head
	sub_head SubHead;
	SubHead.sub_type = (unsigned short)ST_PUSH_MINE;
	SubHead.sub_attrs = 0;
	SubHead.sub_extend = UserMapID;
	SubHead.sub_length = sizeof(int)
		+ sizeof(short) + strCode.size()
		+ sizeof(short) + strName.size()
		+ sizeof(short) + strParam.size()
		+ sizeof(short) + sLen;

	writer->Push_back(&SubHead, sizeof(sub_head));

	unsigned int iMsgID = MsgID;
	writer->Push_back(&iMsgID, sizeof(int));

	sLen = strCode.size();
	writer->Push_back(&sLen, sizeof(short));
	if (sLen > 0)
	{
		writer->Push_back((void*)strCode.c_str(), sLen);
	}

	sLen = strName.size();
	writer->Push_back(&sLen, sizeof(short));
	if (sLen > 0)
	{
		writer->Push_back((void*)strName.c_str(), sLen);
	}

	sLen = strParam.size();
	writer->Push_back(&sLen, sizeof(short));
	if (sLen > 0)
	{
		writer->Push_back((void*)strParam.c_str(), sLen);
	}

	sLen = strlen(pMsg);
	writer->Push_back(&sLen, sizeof(short));
	if (sLen > 0)
	{
		writer->Push_back((void*)pMsg, sLen);
	}
	return true;
}


// 以android的记录格式生成公共消息记录
bool
CAndroidPushUser::AddAndroidPublicNoticeMsg(const unsigned int UserMapID,
	const unsigned int MsgID, const PushMsg *pPushMsgRec, CBuffWriter *writer)
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
	short sLen = 0;
	string strParam("");
	char cReceiveBuf[256] = {0};
	unsigned char cType = MsgRecord.PublicNoticeType;
	if (NOTICE_NORMAL == cType)
	{
		pMsg = ChangeToPublicMsg(&MsgRecord, cReceiveBuf, 256);
	}
	else if (NOTICE_EXTENSION == cType)
	{
		pMsg = ChangeToPublicMsg(&MsgRecord, cReceiveBuf, 256);
		strParam = MsgRecord.strURL;
	}

	if (NULL == pMsg)
	{
		DEBUG("NULL == pMsg");
		return false;
	}

	// sub Head
	sub_head SubHead;
	SubHead.sub_type = (unsigned short)ST_PUSH_PUBLIC;
	SubHead.sub_attrs = 0;
	SubHead.sub_extend = UserMapID;
	SubHead.sub_length = sizeof(int)
			+ sizeof(char)
			+ sizeof(short) + strParam.size()
			+ sizeof(short) + strlen(pMsg);

	writer->Push_back(&SubHead, sizeof(sub_head));

	// 消息ID
	unsigned int uMsgID = MsgID;
	writer->Push_back(&uMsgID, sizeof(int));
	// 公告类型
	writer->Push_back(&cType, sizeof(char));

	// 附加参数
	sLen = strParam.size();
	writer->Push_back(&sLen, sizeof(short));
	if (sLen > 0)
	{
		writer->Push_back((void*)strParam.c_str(), sLen);
	}

	// 消息
	sLen = strlen(pMsg);
	writer->Push_back(&sLen, sizeof(short));
	if (sLen > 0)
	{
		writer->Push_back((void*)pMsg, sLen);
	}
	return true;
}

// 检查用户节点的运行状态
const int
CAndroidPushUser::CheckUserAliveStatus(const time_t NowTime)
{
	int iRes = ShortItvlCheckUserAlive(NowTime);
	return iRes;
}

// 形成发送数据
void
CAndroidPushUser::GenerateSendRecord(LargeMemoryCache *SendBuf, CBuffWriter *writer)
{
	writer->SeekToBegin();

	// common head
	ACC_CMDHEAD CommonHead;
	CommonHead.m_wCmdType = ACCCMD_REQ_GUID;
	CommonHead.m_wAttr = 0;
	CommonHead.m_wAttr |= ACCATTR_PUSHINFO;
	CommonHead.m_nLen = writer->GetNewdataLen();
	CommonHead.m_nExpandInfo = m_uExpandInfoValue;

	writer->Push_set(&CommonHead, sizeof(ACC_CMDHEAD));
	writer->SeekToEnd();
	SendBuf->SetRemainMemStart(writer->GetNewdataLen());
}

// 处理314请求
int
CAndroidPushUser::ParseReq314(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	unsigned short iStrLength = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	unsigned int uGotID = 0;
	char cResMark = 0;

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto VERIRY_ERROR;
	}

	if (sizeof(int) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	uGotID = reader.ReadUInt();

	iRes = GetLastestPublicNotice(pCmdHead, uGotID, writer);
	if (-1 == iRes || -2 == iRes)
	{
		goto EMPTY;
	}
	else if (0 == iRes)
	{
		return 0;
	}
	else
	{
		goto VERIRY_ERROR;
	}

EMPTY:
	{
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short);
		writer->Push_back(&subHead, sizeof(sub_head));

		cResMark = 0;
		writer->Push_back(&cResMark, sizeof(char));
		writer->Push_back(&uGotID, sizeof(int));
		iRes = 0;
		writer->Push_back(&iRes, sizeof(short));

		return 0;
	}

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

// 处理317请求
int
CAndroidPushUser::ParseReq317(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	unsigned short iStrLength = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	char cResMark = 0;
	short iReqShort = 0;
	short iIndex = 0;
	ReqLatestMsgTag ReqData;
	const int iMaxContentsBuf = 1536;
	char cContentsBuf[iMaxContentsBuf + 1] = {0};
	char cMergeBuf[iMaxContentsBuf * 2 + 1] = {0};
	CBuffWriter MergeWriter(cMergeBuf, iMaxContentsBuf * 2, 0);
	unsigned int uCurrentPushCrc = 0;
	short iResShort = 0;
	int ReadCount = 0;
	const short iExtLen = sizeof(sub_head) + sizeof(char) + sizeof(short);

	// 校验
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto VERIRY_ERROR;
	}

	// 请求个数
	if (sizeof(short) > reader.GetUnReadLen())
	{
		iRes = -1;
		goto CONDITION_ERROR;
	}
	iReqShort = reader.ReadShort();
	if (iReqShort <= 0)
	{
		goto EMPTY;
	}

	// 请求数据
	for (iIndex = 0; iIndex < iReqShort; iIndex++)
	{
		bzero(&ReqData, sizeof(ReqLatestMsgTag));
		if (sizeof(char) > reader.GetUnReadLen())
		{
			iRes = -2;
			break;
		}
		ReqData.MsgType = reader.ReadByte();

		if (sizeof(int) > reader.GetUnReadLen())
		{
			iRes = -3;
			break;
		}
		ReqData.ReqID = reader.ReadUInt();

		if (sizeof(char) > reader.GetUnReadLen())
		{
			iRes = -4;
			break;
		}
		ReqData.ReqExt = reader.ReadByte();
		
		// 请求数据处理
		bzero(cContentsBuf, iMaxContentsBuf + 1);
		CBuffWriter MsgContentWriter(cContentsBuf, iMaxContentsBuf, 0);
		if (PTE_INFOMINE == ReqData.MsgType)
		{
			uCurrentPushCrc = m_iInfoMineNum;
			ReadCount = RequestLatestInfoMineForUnWarningUser(ReqData, pCmdHead, &MsgContentWriter, uCurrentPushCrc);
			if (ReadCount > 0)
			{
				if (MergeWriter.CanHold(MsgContentWriter.GetNewdataLen())
					&& writer->CanHold(iExtLen + MergeWriter.GetNewdataLen() + MsgContentWriter.GetNewdataLen()))
				{
					MergeWriter.Push_back(cContentsBuf, MsgContentWriter.GetNewdataLen());
					iResShort += ReadCount;
					
					// 置位取数据
					m_iInfoMineNum = uCurrentPushCrc + 1;
					// 更新用户信息
					SetOperDbProperty(PU_DB_UPDATE);
				}
				else
				{
					break;
				}
			}
		}
		else if (PTE_NOTICE == ReqData.MsgType)
		{
			uCurrentPushCrc = m_iPublicNoticeNum;
			ReadCount = RequestLatestNoticeForUnWarningUser(ReqData, pCmdHead, &MsgContentWriter, uCurrentPushCrc);
			if (ReadCount > 0)
			{
				if (MergeWriter.CanHold(MsgContentWriter.GetNewdataLen())
					&& writer->CanHold(iExtLen + MergeWriter.GetNewdataLen() + MsgContentWriter.GetNewdataLen()))
				{
					MergeWriter.Push_back(cContentsBuf, MsgContentWriter.GetNewdataLen());
					iResShort += ReadCount;
					
					// 置位取数据
					m_iPublicNoticeNum = uCurrentPushCrc + 1;
					// 更新用户信息
					SetOperDbProperty(PU_DB_UPDATE);
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			DEBUG("Type not defined.[%u]", ReqData.MsgType);
		}
	}

	if (iRes < 0)
	{
		DEBUG("Read ReqLatestMsgTag failed[index=%d, Res=%d]", iIndex, iRes);
		goto CONDITION_ERROR;
	}
	
	if (iResShort > 0 && MergeWriter.GetNewdataLen() > 0)
	{
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(short) + MergeWriter.GetNewdataLen();
		writer->Push_back(&subHead, sizeof(sub_head));
		
		cResMark = 0;
		writer->Push_back(&cResMark, sizeof(char));
		
		writer->Push_back(&iResShort, sizeof(short));
		writer->Push_back(cMergeBuf, MergeWriter.GetNewdataLen());
		return 0;
	}
	
	goto EMPTY;


EMPTY:
	{
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(short);
		writer->Push_back(&subHead, sizeof(sub_head));

		cResMark = 0;
		writer->Push_back(&cResMark, sizeof(char));
		iResShort = 0;
		writer->Push_back(&iResShort, sizeof(short));

		return 0;
	}

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

// 处理1000请求
int
CAndroidPushUser::ParseReq1000(const sub_head *pCmdHead, const UserLogInfor *pUlf,
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
	subHead.sub_length = sizeof(char) + sizeof(char) + sizeof(CAndroidPushUser);
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cResMark, sizeof(char));
	writer->Push_back(&pUlf->m_cPlatformCode, sizeof(char));
	writer->Push_back(this, sizeof(CAndroidPushUser));
	
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

// 请求最新的公告(非预警的android用户)
int
CAndroidPushUser::GetLastestPublicNotice(const sub_head *pCmdHead, const unsigned int HaveGotCrc, CBuffWriter *writer)
{
	pPushMsg pHis = NULL;
	sub_head subHead = {0};
	unsigned int uCurrentPushCrc = HaveGotCrc;
	unsigned int UserMapID = pCmdHead->sub_extend;

	if (m_iPublicNoticeNum >= m_iPublicNoticeTotal || uCurrentPushCrc >= m_iPublicNoticeTotal)
	{
		return -1;
	}

	short iAddCount = 0;
	char cContentsBuf[1536] = {0};
	CBuffWriter MsgContentWriter(cContentsBuf, 1535, 0);
	// 传零值的限定
	unsigned int i = (0 == HaveGotCrc ? m_iPublicNoticeTotal - 1 : m_iPublicNoticeNum);

	for (; i < m_iPublicNoticeTotal; i++)
	{
		if (!MsgContentWriter.CanHold(256))
		{
			break;
		}

		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_NOTICE,
			i, pHis) < 0)
		{
			WARN("User[%u] Notice [id=%u] jumped.", UserMapID, i);
			uCurrentPushCrc = i;
			continue;
		}

		if (0 == PushbackNoticeMsg(pHis, i, &MsgContentWriter))
		{
			iAddCount++;
			uCurrentPushCrc = i;
		}
	}

	// 结果
	char cRes = 0;
	if (iAddCount > 0 && MsgContentWriter.GetNewdataLen() > 0
		&& writer->CanHold(MsgContentWriter.GetNewdataLen()))
	{
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char) + sizeof(int) + sizeof(short) + MsgContentWriter.GetNewdataLen();
		writer->Push_back(&subHead, sizeof(sub_head));

		cRes = 0;
		writer->Push_back(&cRes, sizeof(char));
		writer->Push_back(&uCurrentPushCrc, sizeof(int));
		writer->Push_back(&iAddCount, sizeof(short));
		writer->Push_back(cContentsBuf, MsgContentWriter.GetNewdataLen());

		// 置位取标识
		m_iPublicNoticeNum = uCurrentPushCrc + 1;
		// 更新用户信息
		SetOperDbProperty(PU_DB_UPDATE);
		return 0;
	}

	return -35;
}

// 存储公告消息
int
CAndroidPushUser::PushbackNoticeMsg(const PushMsg *pPushMsgRec, const unsigned int MsgID,
	CBuffWriter *writer)
{
	if (NULL == pPushMsgRec)
	{
		DEBUG("MsgRecord = NULL");
		return -1;
	}

	//  查找消息
	PNmessage MsgRecord;
	if (!CPublicNews::getPublicNewsPointer()->GetIndexPNmessage(pPushMsgRec->SettingRecordID,
			MsgRecord))
	{
		DEBUG("getPNmsgByID failed[%u]", pPushMsgRec->SettingRecordID);
		return -2;
	}

	// 生成消息
	const char *pMsg = NULL;
	short sLen = 0;
	string strParam("");
	char cReceiveBuf[256] = {0};
	unsigned char cType = MsgRecord.PublicNoticeType;
	if (NOTICE_NORMAL == cType)
	{
		pMsg = ChangeToPublicMsg(&MsgRecord, cReceiveBuf, 256);
	}
	else if (NOTICE_EXTENSION == cType)
	{
		pMsg = ChangeToPublicMsg(&MsgRecord, cReceiveBuf, 256);
		strParam = MsgRecord.strURL;
	}

	if (NULL == pMsg)
	{
		DEBUG("NULL == pMsg");
		return false;
	}

	// 消息ID
	unsigned int uMsgID = MsgID;
	writer->Push_back(&uMsgID, sizeof(int));
	// 公告类型
	writer->Push_back(&cType, sizeof(char));

	// 附加参数
	sLen = strParam.size();
	writer->Push_back(&sLen, sizeof(short));
	if (sLen > 0)
	{
		writer->Push_back((void*)strParam.c_str(), sLen);
	}

	// 消息
	sLen = strlen(pMsg);
	writer->Push_back(&sLen, sizeof(short));
	writer->Push_back((void*)pMsg, sLen);
	return 0;
}

// 处理用户请求
int
CAndroidPushUser::ProcessUserRequest(const sub_head *pCmdHead,  const UserLogInfor *pUlf, void *data,
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
	LogUserPrint(__FILE__, __LINE__, __FUNCTION__, m_strUserID, m_cPlatformCode, 1,
				"Request = %d platform = %d UserID = %s\nm_iInfoMineTotal=%u m_iInfoMineNum=%u m_iPublicNoticeTotal =%u m_iPublicNoticeNum=%u",
				pCmdHead->sub_type, pUlf->m_cPlatformCode, m_strUserID,
				m_iInfoMineTotal, m_iInfoMineNum, m_iPublicNoticeTotal, m_iPublicNoticeNum);

	switch(pCmdHead->sub_type)
	{
		case ST_REQ_NOTICE:
			return ParseReq314(pCmdHead, pUlf, data, length, writer);
		
		case ST_GET_NOWARNING_MG:
			return ParseReq317(pCmdHead, pUlf, data, length, writer);
			
		case ST_GET_LASTEST_USER_DATA:
			return ParseReq1000(pCmdHead, pUlf, data, length, writer);

		default:
			return CPushUser::ProcessUserRequest(pCmdHead, pUlf, data, length, writer);
	}

	return -1;
}

// 执行自选股信息地雷扫描
const int
CAndroidPushUser::ExecuteSelfStkInfoMine(const unsigned int UserMapID)
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
CAndroidPushUser::ProcessSelfStkInfoMineStatis(const unsigned int UserMapID)
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

// 非预警用户请求最新的公告消息
int
CAndroidPushUser::RequestLatestNoticeForUnWarningUser(const ReqLatestMsgTag &Req,	
	const sub_head *pCmdHead, CBuffWriter *writer, unsigned int &GotCrc)
{
	pPushMsg pHis = NULL;
	unsigned int UserMapID = pCmdHead->sub_extend;

	if (m_iPublicNoticeNum >= m_iPublicNoticeTotal || Req.ReqID >= m_iPublicNoticeTotal)
	{
		return -1;
	}

	short iAddCount = 0;
	// 传零值的限定
	unsigned int i = (0 == Req.ReqID ? m_iPublicNoticeTotal - 1 : m_iPublicNoticeNum);

	for (; i < m_iPublicNoticeTotal; i++)
	{
		if (!writer->CanHold(256))
		{
			break;
		}

		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_NOTICE,
			i, pHis) < 0)
		{
			WARN("User[%u] Notice [id=%u] jumped.", UserMapID, i);
			GotCrc = i;
			continue;
		}
		
		//  查找消息
		PNmessage MsgRecord;
		if (!CPublicNews::getPublicNewsPointer()->GetIndexPNmessage(pHis->SettingRecordID,
				MsgRecord))
		{
			DEBUG("getPNmsgByID failed[%u]", pHis->SettingRecordID);
			continue;
		}
	
		// 生成消息
		const char *pMsg = NULL;
		short sLen = 0;
		char cReceiveBuf[256] = {0};
		pMsg = ChangeToPublicMsg(&MsgRecord, cReceiveBuf, 256);
		if (NULL == pMsg)
		{
			DEBUG("NULL == pMsg");
			GotCrc = i;
			break;
		}
		
		// 添加消息类型
		unsigned char cType = PTE_NOTICE;
		writer->Push_back(&cType, sizeof(char));
		
		// 添加消息子类型
		cType = MsgRecord.PublicNoticeType;
		writer->Push_back(&cType, sizeof(char));
		
		// 添加消息ID
		unsigned int uMsgID = i;
		writer->Push_back(&uMsgID, sizeof(int));
		
		// 添加股票代码
		sLen = 0;
		writer->Push_back(&sLen, sizeof(short));
		
		// 添加股票名称
		sLen = 0;
		writer->Push_back(&sLen, sizeof(short));
		
		// 添加附加参数
		if (NOTICE_NORMAL == MsgRecord.PublicNoticeType)
		{
			sLen = 0;
		}
		else if (NOTICE_EXTENSION == MsgRecord.PublicNoticeType)
		{
			sLen = strlen(MsgRecord.strURL);
		}
		writer->Push_back(&sLen, sizeof(short));
		if (sLen > 0)
		{
			writer->Push_back((void*)MsgRecord.strURL, sLen);
		}
		
		// 添加消息内容
		sLen = strlen(pMsg);
		writer->Push_back(&sLen, sizeof(short));
		if (sLen > 0)
		{
			writer->Push_back((void*)pMsg, sLen);
		}
		
		iAddCount++;
		GotCrc = i;
	}

	return iAddCount;
}

// 非预警用户请求最新的信息地雷
int
CAndroidPushUser::RequestLatestInfoMineForUnWarningUser(const ReqLatestMsgTag &Req,	
	const sub_head *pCmdHead, CBuffWriter *writer, unsigned int &GotCrc)
{
	pPushMsg pHis = NULL;
	unsigned int UserMapID = pCmdHead->sub_extend;
	char StkCode[MAX_STKCODE_LEN + 1] = {0};
    char StkName[MAX_STKNAME + 1] = {0};
    char ExtParam[MAXURLLEN + 1] = {0};

	if (m_iInfoMineNum >= m_iInfoMineTotal || Req.ReqID >= m_iInfoMineTotal)
	{
		return -1;
	}

	short iAddCount = 0;
	// 传零值的限定
	unsigned int i = (0 == Req.ReqID ? m_iInfoMineTotal - 1 : m_iInfoMineNum);

	for (; i < m_iInfoMineTotal; i++)
	{
		if (!writer->CanHold(256))
		{
			break;
		}

		if (CPushMsgHistoryManage::GetPushMsg(m_cPlatformCode, UserMapID, PTE_INFOMINE,
			i, pHis) < 0)
		{
			WARN("User[%u] Notice [id=%u] jumped.", UserMapID, i);
			GotCrc = i;
			continue;
		}
		
		bzero(StkCode, MAX_STKCODE_LEN + 1);
		bzero(StkName, MAX_STKNAME + 1);
		bzero(ExtParam, MAXURLLEN + 1);
		
		//  查找消息
		IMmessage InfoMineMsg;
		if (INFOMINE_NORMAL == pHis->MsgSubType)
		{
			if (!CInfoMine::getInfoMinePointer()->GetIndexIMmessage(pHis->strStkCode, pHis->SettingRecordID, InfoMineMsg))
			{
				DEBUG("No record or removed[%s %u]", pHis->strStkCode, pHis->SettingRecordID);
				GotCrc = i;
				continue;
			}

			// 尝试从最新动态行情数据中查找
			unsigned short uFindIndex = 0xFFFF;
			uFindIndex = CQuotesSnapshot::findSecurityIndex(pHis->strStkCode);
			if (0xFFFF == uFindIndex)
			{
				DEBUG("scan stock code faild[%s]", pHis->strStkCode);
				GotCrc = i;
				continue;
			}

			ACC_STK_STATIC *pStaticStk = CQuotesSnapshot::GetIndexStatic(uFindIndex, pHis->strStkCode);
			StrCopy(StkCode, pHis->strStkCode, MAX_STKCODE_LEN);
			StrCopy(StkName, pStaticStk->m_szName, MAX_STKNAME);
		}
		else if (INFOMINE_STATISTIC == pHis->MsgSubType)
		{
			InfoMineMsg.ttNewsTime = pHis->DateTime;
			snprintf(InfoMineMsg.strTitle, INFOMINE_MAXTITLELEN, GetSeflSkkStatisFormat(), pHis->LatestValue);
			snprintf(ExtParam, MAXURLLEN, "%d", pHis->LatestValue);
		}
	
		// 生成消息
		const char *pMsg = NULL;
		short sLen = 0;
		char cReceiveBuf[256] = {0};
		pMsg = ChangeToInfoMineMsg(&InfoMineMsg, cReceiveBuf, 256);
		if (NULL == pMsg)
		{
			DEBUG("NULL == pMsg");
			break;
		}
		
		// 添加消息类型
		unsigned char cType = PTE_INFOMINE;
		writer->Push_back(&cType, sizeof(char));
		
		// 添加消息子类型
		cType = pHis->MsgSubType;
		writer->Push_back(&cType, sizeof(char));
		
		// 添加消息ID
		unsigned int uMsgID = i;
		writer->Push_back(&uMsgID, sizeof(int));
		
		// 添加股票代码
		sLen = strlen(StkCode);
		writer->Push_back(&sLen, sizeof(short));
		if (sLen > 0)
		{
			writer->Push_back(StkCode, sLen);
		}
		
		// 添加股票名称
		sLen = strlen(StkName);
		writer->Push_back(&sLen, sizeof(short));
		if (sLen > 0)
		{
			writer->Push_back(StkName, sLen);
		}
		
		// 添加附加参数
		sLen = strlen(ExtParam);
		writer->Push_back(&sLen, sizeof(short));
		if (sLen > 0)
		{
			writer->Push_back(ExtParam, sLen);
		}
		
		// 添加消息内容
		sLen = strlen(pMsg);
		writer->Push_back(&sLen, sizeof(short));
		if (sLen > 0)
		{
			writer->Push_back((void*)pMsg, sLen);
		}
		
		iAddCount++;
		GotCrc = i;
	}

	return iAddCount;
}

