#include "Win8PushUser.h"
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
#include "../outerservpush/Push2Win8.h"
#include "../../thread/CalOperatingThread.h"
#include "../../thread/ThreadsManage.h"
#include "../../thread/NetThread.h"
#include "../../thread/SendOuterDataThread.h"
#include "../db/PublicNews.h"
#include "../db/InfoMine.h"
#include "../selfstock/SelfSelectStockManage.h"
#include "../selfstock/SelfSelectStock.h"

using namespace std;

CWin8PushUser::CWin8PushUser()
	: CPushUser()
{
	bzero(m_strPushToken, MAX_WIN8_PUSH_TOKEN_LEN + 1);
}

CWin8PushUser::CWin8PushUser(const UserLogInfor *LogInfo, const unsigned int ExpandInfo)
	: CPushUser(LogInfo)
{
	m_uExpandInfoValue = ExpandInfo;
	bzero(m_strPushToken, MAX_WIN8_PUSH_TOKEN_LEN + 1);
}

CWin8PushUser::~CWin8PushUser()
{
}

// ����306����
int
CWin8PushUser::ParseReq306(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	short iStrLength = 0;
	unsigned char cMark = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	char strToken[MAX_WIN8_PUSH_TOKEN_LEN] = {0};
	// У��
	iRes = CheckUserLogValid(pUlf);
	if (iRes < 0)
	{
		goto RESULT;
	}
	
	// ȡ��������
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
	reader.ReadString(iStrLength, strToken, MAX_WIN8_PUSH_TOKEN_LEN);
	iRes = SetUserPushToken(strToken);
	goto RESULT;

RESULT:
	if (iRes > -1)
	{
		// �����ɹ�
		memcpy(&subHead, pCmdHead, sizeof(sub_head));
		subHead.sub_length = sizeof(char);
		cMark = 0;
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));
	}
	else
	{
		// ����ʧ��
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
	// ��������
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

// ����1000����
int
CWin8PushUser::ParseReq1000(const sub_head *pCmdHead, const UserLogInfor *pUlf,
		void *data, const unsigned int length, CBuffWriter *writer)
{
	int iRes = 0;
	CBuffReader reader(data, length, 0);
	unsigned short iStrLength = 0;
	sub_head subHead = {0};
	const char *pErrorMsg = NULL;
	unsigned char cReqMark = 0;
	char cResMark = 0;

	// У��
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
	subHead.sub_length = sizeof(char) + sizeof(char) + sizeof(CWin8PushUser);
	writer->Push_back(&subHead, sizeof(sub_head));
	writer->Push_back(&cResMark, sizeof(char));
	writer->Push_back(&pUlf->m_cPlatformCode, sizeof(char));
	writer->Push_back(this, sizeof(CWin8PushUser));
	
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

// ��Win8�ļ�¼��ʽ���Ԥ����¼
bool
CWin8PushUser::AddWin8EWarningMsg(const PushMsg *pPushMsgRec, WpReqTag *writer)
{
	// ������Ϣ
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

	// ��¼����
	pCurrentSet = (CEarlyWarningConditionRecord*)pCurrentEWarn->GetConditionRecord(m_uRecordSetMarkID, pPushMsgRec->SettingRecordID);
	if (NULL == pCurrentSet || WARNINGSET_DELDB == pCurrentSet->GetOperToDb())
	{
		// ���Դ����¶�̬���������в���
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
	
	// ��¼��ʽ
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
	
	short iCount = 0;
	// �γ�toast
	strncpy(writer->ReqContent[iCount].cType, "wns/toast", MAX_REQ_XML_TYPE_LEN);
	int iRes = snprintf(writer->ReqContent[iCount].cXml, MAX_REQ_XML_LEN, 
		"<toast><visual><binding template=\"ToastText01\"><text id=\"1\">%s</text></binding></visual></toast>", 
						pMsg);
	if (iRes > MAX_REQ_XML_LEN)
	{
		return false;
	}
	iCount++;
	
	// �γ�tile
	strncpy(writer->ReqContent[iCount].cType, "wns/tile", MAX_REQ_XML_TYPE_LEN);
	iRes = snprintf(writer->ReqContent[iCount].cXml, MAX_REQ_XML_LEN, 
		"<tile><visual><binding template=\"TileWideText09\"><text id=\"1\">%s</text><text id=\"2\">%s</text></binding></visual></tile>", 
						GetIndexUserTiltleSeting(0), pMsg);
	if (iRes > MAX_REQ_XML_LEN)
	{
		return false;
	}
	iCount++;
	
	writer->ValidNum = iCount;
	
    return true;
}

// ��Win8�ļ�¼��ʽ���ɹ�����Ϣ��¼
bool
CWin8PushUser::AddWin8PublicNoticeMsg(const PushMsg *pPushMsgRec, WpReqTag *writer)
{
	if (NULL == pPushMsgRec)
	{
		DEBUG("MsgRecord = NULL");
		return false;
	}
	
	//  ������Ϣ
	PNmessage MsgRecord;
	if (!CPublicNews::getPublicNewsPointer()->GetIndexPNmessage(pPushMsgRec->SettingRecordID,
			MsgRecord))
	{
		DEBUG("getPNmsgByID failed[%u]", pPushMsgRec->SettingRecordID);
		return false;
	}
	
	// ������Ϣ
	const char *pMsg = NULL;
	string strParam("");
	char cReceiveBuf[256] = {0};
	
	pMsg = ChangeToPublicMsg(&MsgRecord, cReceiveBuf, 256);
	unsigned char cType = MsgRecord.PublicNoticeType;
	
	int iRes = 0;
	short iCount = 0;
	if (NOTICE_NORMAL == cType)
	{
		// �γ�toast
		strncpy(writer->ReqContent[iCount].cType, "wns/toast", MAX_REQ_XML_TYPE_LEN);
		iRes = snprintf(writer->ReqContent[iCount].cXml, MAX_REQ_XML_LEN, 
			"<toast><visual><binding template=\"ToastText01\"><text id=\"1\">%s</text></binding></visual></toast>", 
							pMsg);
		if (iRes > MAX_REQ_XML_LEN)
		{
			return false;
		}
		iCount++;
		
		writer->ValidNum = iCount;
	}
	else if (NOTICE_EXTENSION == cType)
	{
		strncpy(writer->ReqContent[iCount].cType, "wns/toast", MAX_REQ_XML_TYPE_LEN);
		iRes = snprintf(writer->ReqContent[iCount].cXml, MAX_REQ_XML_LEN, 
				"<toast><visual><binding template=\"ToastText02\"><text id=\"1\">%s</text><text id=\"2\">%s</text></binding></visual></toast>", 
						pMsg, MsgRecord.strURL);
		if (iRes > MAX_REQ_XML_LEN)
		{
			return false;
		}
		iCount++;
		
		writer->ValidNum = iCount;
	}
	else
	{
		ERROR("nodefined type");
		return false;
	}
	
	return true;
}

// ��Win8�ļ�¼��ʽ������Ϣ���׼�¼
bool
CWin8PushUser::AddWin8MineMsg(const PushMsg *pPushMsgRec, WpReqTag *writer)
{
	// ������Ϣ
	char cReceiveBuf[256] = {0};
	const char *pMsg = NULL;
	string strCode("");
	string strName("");
	int iRes = 0;
	short iCount = 0;

	// ����
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
					
			// ���Դ����¶�̬���������в���
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
			
			// �γ�toast
			strncpy(writer->ReqContent[iCount].cType, "wns/toast", MAX_REQ_XML_TYPE_LEN);
			iRes = snprintf(writer->ReqContent[iCount].cXml, MAX_REQ_XML_LEN, 
				"<toast><visual><binding template=\"ToastText01\"><text id=\"1\">%s</text></binding></visual></toast>", 
								pMsg);
			if (iRes > MAX_REQ_XML_LEN)
			{
				return false;
			}
			iCount++;
			
			// �γ�tile
			strncpy(writer->ReqContent[iCount].cType, "wns/tile", MAX_REQ_XML_TYPE_LEN);
			iRes = snprintf(writer->ReqContent[iCount].cXml, MAX_REQ_XML_LEN, 
				"<tile><visual><binding template=\"TileWideText09\"><text id=\"1\">%s</text><text id=\"2\">%s</text></binding></visual></tile>", 
								strName.c_str(), pMsg);
			if (iRes > MAX_REQ_XML_LEN)
			{
				return false;
			}
			iCount++;
			
			writer->ValidNum = iCount;
		}
		else if (INFOMINE_STATISTIC == pPushMsgRec->MsgSubType)
		{
			InfoMineMsg.ttNewsTime = pPushMsgRec->DateTime;
			snprintf(InfoMineMsg.strTitle, INFOMINE_MAXTITLELEN, GetSeflSkkStatisFormat(), pPushMsgRec->LatestValue);
			pMsg = ChangeToInfoMineMsg(&InfoMineMsg, cReceiveBuf, 256);
			
			// �γ�toast
			strncpy(writer->ReqContent[iCount].cType, "wns/toast", MAX_REQ_XML_TYPE_LEN);
			iRes = snprintf(writer->ReqContent[iCount].cXml, MAX_REQ_XML_LEN, 
				"<toast><visual><binding template=\"ToastText01\"><text id=\"1\">%s</text></binding></visual></toast>", 
								pMsg);
			if (iRes > MAX_REQ_XML_LEN)
			{
				return false;
			}
			iCount++;
			
			// �γ�tile
			strncpy(writer->ReqContent[iCount].cType, "wns/tile", MAX_REQ_XML_TYPE_LEN);
			iRes = snprintf(writer->ReqContent[iCount].cXml, MAX_REQ_XML_LEN, 
				"<tile><visual><binding template=\"TileWideText09\"><text id=\"1\">%s</text><text id=\"2\">%s</text></binding></visual></tile>", 
								GetIndexUserTiltleSeting(1), pMsg);
			if (iRes > MAX_REQ_XML_LEN)
			{
				return false;
			}
			iCount++;
			
			// �γ�badge
			strncpy(writer->ReqContent[iCount].cType, "wns/badge", MAX_REQ_XML_TYPE_LEN);
			iRes = snprintf(writer->ReqContent[iCount].cXml, MAX_REQ_XML_LEN, 
				"<badge value=\"%u\"/>", pPushMsgRec->LatestValue);
			if (iRes > MAX_REQ_XML_LEN)
			{
				return false;
			}
			iCount++;
			
			writer->ValidNum = iCount;
		}
	}
	
    return true;
}

// ����Ԥ����Ϣ
const int 
CWin8PushUser::SendEWarningMsg(const unsigned int UserMapID)
{
	if (m_iPushWarningNum >= m_iPushWarningTotal)
	{
		return -1;
	}
	
	WpReqTag ReqData;
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
		
		bzero(&ReqData, sizeof(WpReqTag));
		if (!AddWin8EWarningMsg(pHis, &ReqData))
		{
			DEBUG("AddWin8EWarningMsg failed.");
			break;
		}
		uCurrentPushCrc = i;
		
		if (0 == pCurrentThread->UserExecuteSend(m_cPlatformCode, m_strPushToken, &ReqData))
		{
			iSendCount++;
			DEBUG("User[%s] send win8 push msg", GetUserID());
			
			// ����Ԥ�����ͱ�ʶ
			m_iPushWarningNum = uCurrentPushCrc + 1;
		}
		else 
		{
			TRACE("Send win8 Push error");
		}
	}
	
	return 0;
}

// ������Ϣ����
const int
CWin8PushUser::SendInfoMineMsg(const unsigned int UserMapID)
{
	if (m_iInfoMineNum >= m_iInfoMineTotal)
	{
		return -1;
	}
	
	WpReqTag ReqData;
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
		
		bzero(&ReqData, sizeof(WpReqTag));
		if (!AddWin8MineMsg(pHis, &ReqData))
		{
			DEBUG("AddWin8MineMsg failed.");
			break;
		}
		uCurrentPushCrc = i;
		
		if (0 == pCurrentThread->UserExecuteSend(m_cPlatformCode, m_strPushToken, &ReqData))
		{
			iSendCount++;
			DEBUG("User[%s] send win8 InfoMine msg", GetUserID());
			
			// ���·��ͱ�ʶ
			m_iInfoMineNum = uCurrentPushCrc + 1;
		}
		else 
		{
			TRACE("Send win8 InfoMine error");
		}
	}
	
	return 0;
}

// ���͹���
const int 
CWin8PushUser::SendNoticeMsg(const unsigned int UserMapID)
{
	if (m_iPublicNoticeNum >= m_iPublicNoticeTotal)
	{
		return -1;
	}
	
	WpReqTag ReqData;
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
		
		bzero(&ReqData, sizeof(WpReqTag));
		if (!AddWin8PublicNoticeMsg(pHis, &ReqData))
		{
			DEBUG("AddWin8PublicNoticeMsg failed.");
			break;
		}
		uCurrentPushCrc = i;
		
		if (0 == pCurrentThread->UserExecuteSend(m_cPlatformCode, m_strPushToken, &ReqData))
		{
			iSendCount++;
			DEBUG("User[%s] send win8 notice msg", GetUserID());
			
			// ���·��ͱ�ʶ
			m_iPublicNoticeNum = uCurrentPushCrc + 1;
		}
		else 
		{
			TRACE("Send win8 notice error");
		}
	}
	
	return 0;
}

// ������������
int
CWin8PushUser::SetUserPushToken(const char *token)
{
	StrCopy(m_strPushToken, token, MAX_WIN8_PUSH_TOKEN_LEN);
	return 0;
}

// ��ȡ��������
void
CWin8PushUser::GetUserPushToken(char *token, const int MaxLen)
{
	StrCopy(token, m_strPushToken, MaxLen);
}

// ����û��ڵ������״̬
const int
CWin8PushUser::CheckUserAliveStatus(const time_t NowTime)
{
	int iRes = LongItvlCheckUserAlive(NowTime);
	return iRes;
}

// �����û�����
int
CWin8PushUser::ProcessUserRequest(const sub_head *pCmdHead,  const UserLogInfor *pUlf, void *data,
						const unsigned int length, CBuffWriter *writer)
{
	if (NULL == pCmdHead || NULL == data)
	{
		DEBUG("Error cmd or null data error.");
		return -1;
	}
	
	// ���»ʱ��
	UpdateLastestAliveTime(pCmdHead);
	if (305 != pCmdHead->sub_type)
	{
		TRACE("Request = %d platform = %d UserID = %s",
				pCmdHead->sub_type, pUlf->m_cPlatformCode, m_strUserID);
	}
	
	switch(pCmdHead->sub_type)
	{
		// ��������
		case ST_TOKEN:
			return ParseReq306(pCmdHead, pUlf, data, length, writer);
			
		case ST_GET_LASTEST_USER_DATA:
			return ParseReq1000(pCmdHead, pUlf, data, length, writer);

		default:
			return CPushUser::ProcessUserRequest(pCmdHead, pUlf, data, length, writer);
	}

	return -1;
}

// ִ�нڵ�ɼ�Ԥ������
int
CWin8PushUser::ExecuteStkPriceWarningCal(const unsigned int UserMapID)
{
	// �����û�������ɨ��
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

			// �γɼ�¼
			if (!GenerateEarlyWarningMsg(bit, &CurrentItem, i, pFind, &tPushMsg))
			{
				DEBUG("GenerateEarlyWarningMsg error");
				continue;
			}

			// ��¼��ʷ
			if (!PushBackMsg(UserMapID, tPushMsg))
			{
				DEBUG("PushBackMsg error.");
				continue;
			}
			iChangeCount++;
		}

		// �����û���¼
		pFind->UpdateSetOperToDb(WARNINGSET_UPDATEDB);
	}
	
	return 0;
}

// ִ�й������Ź������ɨ��
int
CWin8PushUser::ExecutePublicNoticeScan(const unsigned int UserMapID)
{
	// �����û�������ɨ��
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
		// �γɼ�¼
		PushMsg tPushMsg;
		if (!GenerateNoticeMsg(&msg, &tPushMsg))
		{
			DEBUG("GenerateNoticeMsg error");
			continue;
		}
	
		// ��¼��ʷ
		if (!PushBackMsg(UserMapID, tPushMsg))
		{
			DEBUG("PushBackMsg error.");
			continue;
		}
		iChangeCount++;
	}

	return iChangeCount;
}

// ִ����ѡ����Ϣ����ɨ��
const int
CWin8PushUser::ExecuteSelfStkInfoMine(const unsigned int UserMapID)
{
	// �����û�������ɨ��
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

// ִ����ѡ����Ϣ����ͳ��
const int
CWin8PushUser::ProcessSelfStkInfoMineStatis(const unsigned int UserMapID)
{
	CSelfSelectStock *pSelfSelectStock = CSelfSelectStockManage::GetPlatformManage(m_cPlatformCode);
	const int iRecCount = pSelfSelectStock->GetSelfCodeCount(UserMapID);
	char cStkCode[MAX_STKCODE_LEN + 1] = {0};
	unsigned int iTotalInfoMineCount = 0;
	unsigned int uStatisCount = 0;
	
	// �Ѿ����͹�����ɨ��
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
		// �γɼ�¼
		PushMsg tPushMsg;
		if (!GenerateInfoMineStatisMsg(uStatisCount, &tPushMsg))
		{
			DEBUG("GenerateInfoMineMsg error");
			return -3;
		}

		// ��¼��ʷ
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

// ������Ϣ
int
CWin8PushUser::SendOuterMsg(const unsigned int UserMapID, const unsigned char MsgType)
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

// ִ�з���
int
CWin8PushUser::ExecuteSend(const unsigned int UserMapID, const unsigned char PushType)
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
		// �����û���Ϣ
		SetOperDbProperty(PU_DB_UPDATE);
	}
	
	return iRes;
}



