#include "PublicNews.h"
#include "CppODBC.h"
#include "../pushuser/PushUser.h"
#include "../../../controller/runtime.h"
#include "../../config.h"
#include "../../../util/log/log.h"
#include "../../data_type.h"
#include "../../../util/common/common_lib.h"
#include "../../../util/string/string_util.h"
#include <iostream>
#include <algorithm>
#include <time.h> 

/**************************************
** 由于数据库设置无请求用户的存活时间为15秒
** 程序不需要如此密集的请求，因此将连接改为短链
** 即每次请求的时候建立连接，完成后断开连接
***************************************/

using namespace std;

CPublicNews *CPublicNews::m_pPublicNews = NULL;

CPublicNews::CPublicNews(const unsigned int MaxNum, const char*dsn, 
	const char*name, const char*pwd)
	: m_uMaxCacheNum(MaxNum),
	m_pCppODBC(new CppODBC()),
	m_pRecordCache(new PNmessage[m_uMaxCacheNum]),
	m_uOccupyIndex(0),
	m_luCurrentMaxRecID(0),
	m_iCurrentStatus(-1)
{
	StrCopy(m_dsn, dsn, NOTICE_LINK_LEN);
	StrCopy(m_uname, name, NOTICE_LINK_LEN);
	StrCopy(m_upass, pwd, NOTICE_LINK_LEN);
	memset(m_pRecordCache, -1, m_uMaxCacheNum * sizeof(PNmessage));
	m_GotNewMsgID.reserve(5);
	
	InitialRes();
}

CPublicNews::~CPublicNews(void)
{
	if (NULL != m_pCppODBC)
	{
		m_pCppODBC->Close();
		delete m_pCppODBC;
		m_pCppODBC = NULL;
	}

	if (NULL != m_pRecordCache)
	{
		delete []m_pRecordCache;
		m_pRecordCache = NULL;
	}
	
	m_GotNewMsgID.clear();
}

const unsigned int 
CPublicNews::GetCirculateIndex(const unsigned int index)
{
	if (index < m_uMaxCacheNum)
	{
		return index;
	}

	return index % m_uMaxCacheNum;
}

// 添加记录
const int
CPublicNews::AddNewRecord(const PNmessage &record)
{
	unsigned int uCurrentId = GetCirculateIndex(m_uOccupyIndex++);
	
	PNmessage *pSetRecord = GetIndexPNmessage(uCurrentId);
	if (NULL == pSetRecord)
	{
		return -1;
	}

	// 处理当前位置
	if ((unsigned long)-1 == pSetRecord->MsgID)
	{
		// 没有链接
	}
	else
	{
		// 已经被使用，覆盖更新
		if (0 != RemoveIndexNode(uCurrentId))
		{
			return -2;
		}	
	}

	bzero(pSetRecord, sizeof(PNmessage));
	*pSetRecord = record;
	UpdateLinkNode(pSetRecord->MsgID, uCurrentId);

	return 0;
}

// 获取指定索引的记录指针
PNmessage* 
CPublicNews::GetIndexPNmessage(const unsigned int NodeIndex)
{
	if (NodeIndex >= m_uMaxCacheNum)
	{
		return NULL;
	}

	return &m_pRecordCache[NodeIndex];
}

// 移除指定索引的记录
const int
CPublicNews::RemoveIndexNode(const unsigned int index)
{
	PNmessage *pCurrentNode = GetIndexPNmessage(index);
	if (NULL == pCurrentNode)
	{
		return (unsigned int)-1;
	}
	
	m_mpMarkNodes.erase(pCurrentNode->MsgID);
	
	return 0;
}

// 更新节点链接
bool
CPublicNews::UpdateLinkNode(const unsigned long Key, const unsigned int Index)
{
	return (m_mpMarkNodes.insert(pair<const unsigned long, unsigned int>(Key, Index))).second;
}

// 初始化资源
void
CPublicNews::InitialRes()
{
	if (NULL != m_pCppODBC)
	{
	    m_pCppODBC->Initialize();
	    if(!m_pCppODBC->Open())
	    {
	    	ERROR("Failure - Open PublicNews EnvirHandle ODBC \n");
	    }
	}
}

// 重置请求
void 
CPublicNews::resetData()
{
	memset(m_pRecordCache, -1, m_uMaxCacheNum * sizeof(PNmessage));
	m_uOccupyIndex = 0;
	m_mpMarkNodes.clear();
}

bool
CPublicNews::DBconnect()
{	
	if (NULL != m_pCppODBC)
	{
		if (m_pCppODBC->Connect(m_dsn, m_uname, m_upass))
		{
			m_pCppODBC->SQLQuery("SET names utf8;");
			return true;
		}
	}
	
	ERROR("Connect data base error[%s-%s-%s]", m_dsn, m_uname, m_upass);

    return false;
}

// 断开数据库连接
bool
CPublicNews::DBdisconnect()
{
	if (NULL != m_pCppODBC)
	{
		return m_pCppODBC->DisConnect();
	}
	
    return false;
}

// 请求数据
const int
CPublicNews::RequestMsg(const char *sql, bool MarkNewID)
{
	int column = 0;
    int iRet = 0;
    int iGotCount = 0;
    
    iRet = m_pCppODBC->SQLQuery(sql);
    if( (iRet <= 0) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
    {
    	return -1;
    }
    
    if(iRet > 0)
    {
    	char cContent[MAXCONTENTLEN + 1] = {0};
    	iGotCount += iRet;
        while( !m_pCppODBC->Eof())
        {
        	column = 0;
        	PNmessage msg;
        	bzero(&msg, sizeof(PNmessage));
        	msg.MsgID = m_pCppODBC->GetULongValue(column++);
        	m_luCurrentMaxRecID = msg.MsgID;
			msg.ttNewsTime = m_pCppODBC->GetTimetValue(column++);
			msg.ttAreaBegin = m_pCppODBC->GetTimetValue(column++);
			msg.ttAreaEnd = m_pCppODBC->GetTimetValue(column++);
			bzero(cContent, MAXCONTENTLEN + 1);
			StrCopy(cContent, m_pCppODBC->GetStrValue(column++), MAXCONTENTLEN);
			StrCopy(msg.strURL, m_pCppODBC->GetStrValue(column++), MAXURLLEN);
			char strPlatform[MAXPLATFORMLEN + 1] = {0};	
			StrCopy(strPlatform, m_pCppODBC->GetStrValue(column++), MAXPLATFORMLEN);		
			ParsePlatformCode(strPlatform, &msg);
			char strVersion[MAXVERSION_MER + 1] = {0};
			StrCopy(strVersion, m_pCppODBC->GetStrValue(column++), MAXVERSION_MER);
			ParseVersionRange(strVersion, &msg);
			unsigned char pntype = m_pCppODBC->GetUCharValue(column++);
			if (1 == pntype)
			{
				LeftUtf8String(MAXCONTENTLEN, cContent, msg.strContent, MAXCONTENTLEN);
				msg.PublicNoticeType = NOTICE_NORMAL;
			}
			else if (2 == pntype)
			{
				LeftUtf8String(MAXCONTENTLEN_EX, cContent, msg.strContent, MAXCONTENTLEN);
				msg.PublicNoticeType = NOTICE_EXTENSION;
			}
			msg.ReceiveUserMark = (unsigned int)-1;
			
			if (IsEmptyString(msg.strContent))
			{
				ERROR("ERROR:Notice[ID=%lu] contents can not be empty.", msg.MsgID);
				m_pCppODBC->Next();
				continue;
			}
			
        	iRet = AddNewRecord(msg);
			if (0 != iRet)
			{
				DEBUG("AddNewRecord error[%d]", iRet);
			}
			else
			{
				if (MarkNewID)
				{
					NOTE("Read New Notice[MsgID=%lu content=%s]", msg.MsgID, msg.strContent);
					m_GotNewMsgID.push_back(msg.MsgID);
				}
			}
			
			m_pCppODBC->Next();
        }
    }

    return iGotCount;
}

// 读取最近10天的公告
const int
CPublicNews::ReadLatest10DaysMsg()
{
	char sql[1024] = {0};
    sprintf(sql, "SELECT pn.id, UNIX_TIMESTAMP(pn.createtime) AS createtime, \
		UNIX_TIMESTAMP(pn.areabegin) AS areabegin, UNIX_TIMESTAMP(pn.areaend) AS areaend, \
		LEFT(pn.content, %u) AS content, LEFT(pn.url, %u), LEFT(pn.platform, %u), LEFT(pn.version, %u), pn.msgstyle \
		FROM T_DT_Information pn WHERE pn.id > %lu AND pn.style = '1' \
		AND pn.createtime > DATE_SUB(CURDATE(), INTERVAL 10 DAY) ORDER BY pn.id ASC;",
		MAXCONTENTLEN / 3, MAXURLLEN, MAXPLATFORMLEN, MAXVERSION_MER, m_luCurrentMaxRecID);
		
	if (!DBconnect())
	{
		return -1;
	}
	
	int iRes = RequestMsg(sql, false);
	if (0 == m_luCurrentMaxRecID)
	{
		m_luCurrentMaxRecID = GetMaxDbIDMarkByOne();
		TRACE("Current Max ID=%lu", m_luCurrentMaxRecID);
	}
	DBdisconnect();

    return iRes;
}

// 解析平台
const int
CPublicNews::ParsePlatformCode(char *src, PNmessage *msg)
{
	int uResult = 0;
	if (NULL == src)
	{
		return uResult;
	}
	
	char seps[] = " ,\t\n";
	char *token = strtok(src, seps);
	int iMark = 0;
	while(token != NULL && msg->PlatformNum < MAXPLATNUM)
	{
		uResult++;
		// 顺序标识
		iMark = atoi(token);
		msg->cPlatformMark[msg->PlatformNum++] = iMark;
		
		token = strtok( NULL, seps);
	}
	
	return uResult;
}

// 解析版本号
const int
CPublicNews::ParseVersionRange(char *src, PNmessage *msg)
{
	int uResult = 0;
	if (NULL == src)
	{
		return uResult;
	}
	
	char seps[] = " -\t\n";
	char *token = strtok(src, seps);
	unsigned short uVersionMin = 0, uVersionMax = 0;
	while(token != NULL)
	{
		if (0 == uResult % 2)
		{
			uVersionMin = ChangeToUINT(atof(token), 2);
		}
		else if (1 == uResult % 2)
		{
			uVersionMax = ChangeToUINT(atof(token), 2);
		}
		
		token = strtok( NULL, seps);
		uResult++;
	}
	
	if (uResult > 0)
	{
		if (1 == uResult % 2)
		{
			uVersionMax = uVersionMin;
		}
		msg->uVersionMin = MIN(uVersionMin, uVersionMax);
		msg->uVersionMax = MAX(uVersionMin, uVersionMax);
		return uResult;
	}
	
	return -1;
}

// 读取已经发送的最大数据ID
const unsigned long 
CPublicNews::GetMaxDbIDMarkByOne()
{
	char *sql = "SELECT pn.id FROM T_DT_Information pn WHERE pn.style = 1 ORDER BY pn.id DESC LIMIT 1;";
	
	int iRet = m_pCppODBC->SQLQuery(sql);
    if( (iRet <= 0) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
    {
    	return 0;
    }
    
    int column = 0;
    if(iRet > 0)
    {
        column = 0;
        return m_pCppODBC->GetULongValue(column++);
    }

    return 0;
}

// 匹配公告消息发送属性
const int
CPublicNews::MatchNoticeSend(const CPushUser *pUser, const PNmessage *msg)
{
	int iRes = -1;
	if (NULL == pUser || NULL == msg)
	{
		return -2;
	}
	
	// 公告归类
	unsigned short uNoticeClassifiedCode = GetPlatformNoticeClassifiedMap(pUser->GetUserPlatform());
	if (NCCE_OTHER == uNoticeClassifiedCode)
	{
		return -3;
	}
	// 平台判断
	for (unsigned int i = 0; i < msg->PlatformNum; i++)
	{
		if (msg->cPlatformMark[i] == uNoticeClassifiedCode)
		{
			iRes = 0;
			break;
		}
	}

	if (0 != iRes)
	{
		return iRes;
	}

	// 版本判断
	if (pUser->GetUserLocalVersion() < msg->uVersionMin
		|| pUser->GetUserLocalVersion() > msg->uVersionMax)
	{
		return -4;
	}

	// 时间判断
	if (pUser->GetUserRegTime() > 0 
		&& ((time_t)pUser->GetUserRegTime() > msg->ttAreaEnd || (time_t)pUser->GetUserRegTime() < msg->ttAreaBegin))
	{
		return -5;
	}

	return 0;
}

CPublicNews * 
CPublicNews::getPublicNewsPointer()
{
	const unsigned int uMaxRecordNum = 500;
	if( NULL == m_pPublicNews )
	{
		const WapPublicNewsConfig *pPNConfig = GetPublicNewsSetting();
		m_pPublicNews = new CPublicNews(uMaxRecordNum, pPNConfig->DSN, pPNConfig->LogUser, pPNConfig->LogPwd);
	}
	return m_pPublicNews;
}

void 
CPublicNews::releasePublicNewsPointer()
{
	if (NULL != m_pPublicNews)
		delete m_pPublicNews;
}

// 初始化请求
bool
CPublicNews::InitialData()
{
	int iRet = -1;
	resetData();
	
	SetCurrentStatus(1);
    iRet = ReadLatest10DaysMsg();
    SetCurrentStatus(0);
    if (iRet < 0)
	{
		DEBUG("ReadLatest10DaysMsg failed");
		return false;
	}
	
    return true;
}

// 增量请求
const int
CPublicNews::GetDynaData()
{
	char sql[1024] = {0};
    sprintf(sql, "SELECT pn.id, UNIX_TIMESTAMP(pn.createtime) AS createtime, \
		UNIX_TIMESTAMP(pn.areabegin) AS areabegin, UNIX_TIMESTAMP(pn.areaend) AS areaend, \
		LEFT(pn.content, %u) AS content, LEFT(pn.url, %u), LEFT(pn.platform, %u), LEFT(pn.version, %u), pn.msgstyle \
		FROM T_DT_Information pn WHERE pn.id > %lu AND pn.style = '0' ORDER BY pn.id ASC;",
		MAXCONTENTLEN / 3, MAXURLLEN, MAXPLATFORMLEN, MAXVERSION_MER, m_luCurrentMaxRecID);

	if (!DBconnect())
	{
		return -1;
	}
	
	SetCurrentStatus(2);
	m_GotNewMsgID.clear();
	int iRes = RequestMsg(sql, true);
	if (iRes > 0 && m_GotNewMsgID.size() > 0)
	{
		SendCallBackToDb();
		m_GotNewMsgID.clear();
	}
	SetCurrentStatus(0);
	DBdisconnect();
	
    return iRes;
}

// 请求最新的消息
const unsigned long
CPublicNews::GetLatestMsg(const unsigned long MsgID, const CPushUser *pUser, PNmessage &msg)
{
	unsigned long uResult = (unsigned long)-1;
	if (MsgID >= m_luCurrentMaxRecID || 0 == m_luCurrentMaxRecID)
	{
		return uResult;
	}
	
	if (0 == MsgID)
	{
		// 比较最近三条
		if (m_luCurrentMaxRecID > 2)
		{
			uResult = m_luCurrentMaxRecID - 2;
		}
		else if (2 == m_luCurrentMaxRecID)
		{
			uResult = m_luCurrentMaxRecID - 1;
		}
		else if (1 == m_luCurrentMaxRecID)
		{
			uResult = m_luCurrentMaxRecID;
		}
		else
		{
			return uResult;
		}
	}
	else
	{
		uResult = MsgID + 1;
	}

	while (uResult <= m_luCurrentMaxRecID)
	{
		map<const unsigned long, unsigned int>::iterator iterFind = m_mpMarkNodes.find(uResult);
		
		if (m_mpMarkNodes.end() != iterFind)
		{
			PNmessage *pSetRecord = GetIndexPNmessage(iterFind->second);
			if (NULL != pSetRecord && 0 == MatchNoticeSend(pUser, pSetRecord))
			{
				bzero(&msg, sizeof(PNmessage));
				msg = *pSetRecord;
				return uResult;
			}
		}
		
		uResult++;
	}
	
	return (unsigned long)-1;
}

// 获取指定的消息
bool
CPublicNews::GetIndexPNmessage(const unsigned long MsgID, PNmessage &msg)
{
	map<const unsigned long, unsigned int>::iterator iterFind = m_mpMarkNodes.find(MsgID);
	if (m_mpMarkNodes.end() != iterFind)
	{
		PNmessage *pSetRecord = GetIndexPNmessage(iterFind->second);
		if (NULL != pSetRecord)
		{
			bzero(&msg, sizeof(PNmessage));
			msg = *pSetRecord;
			return true;
		}
	}
	
	return false;
}

// 设置最新状态
void 
CPublicNews::SetCurrentStatus(const int Value)
{
	m_iCurrentStatus = Value;
}

// 获取当前的状态值
const int
CPublicNews::GetCurrentStatus()const
{
	return m_iCurrentStatus;
}

// 置位数据库标识为已读取
const int
CPublicNews::SendCallBackToDb()
{
	int iRet = 0;
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iOffset = 0;
	
	iOffset = snprintf(sql, MaxBufLen, "UPDATE T_DT_Information SET style = '1' WHERE id IN (%lu", m_GotNewMsgID[0]);
	int iIDCount = m_GotNewMsgID.size();
	for (int i = 1; i < iIDCount; i++)
	{
		iRet = snprintf(sql + iOffset, MaxBufLen, ", %lu", m_GotNewMsgID[i]);
		if (iRet > MaxBufLen)
		{
			break;
		}
		
		iOffset += iRet;
	}
	iRet = snprintf(sql + iOffset, MaxBufLen, ");");
	if (iRet > MaxBufLen)
	{
		return -3;
	}
	
	TRACE("SendCallBack:%s", sql);
	
	iRet = m_pCppODBC->SQLExec(sql);
	if( ( iRet <= 0 ) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
	{
		DBdisconnect();
		if (!DBconnect())
		{
			return -1;
		}
		
		iRet = m_pCppODBC->SQLExec(sql);
	}
	
	if( ( iRet <= 0 ) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
	{
		return -2;
	}
	
	return iRet;
}

char*
CPublicNews::LeftUtf8String(const int StrLen, const char *Src,
	char *BufRecv, const int BufSize)
{
	 int iSrcLen = strlen(Src);

	 if (0 == iSrcLen || NULL == BufRecv || StrLen > BufSize)
	 {
		 return NULL;
	 }

	 if (iSrcLen <= StrLen)
	 {
		 strncpy(BufRecv, Src, iSrcLen);
		 return BufRecv;
	 }

	 int i = 0, n = 0;
	 unsigned char cHead;
	 for(i = 0; i < iSrcLen; i += n)
	 {
		 cHead = Src[i];
		 if (cHead >= 252)
		 {
			 n = 6;
		 }
		 else if (cHead >= 248)
		 {
			 n = 5;
		 }
		 else if (cHead >= 240)
		 {
			 n = 4;
		 }
		 else if (cHead >= 224)
		 {
			 n = 3;
		 }
		 else if (cHead >= 192)
		 {
			 n = 2;
		 }
		 else if (cHead >= 65 && cHead <= 90)
		 {
			 n = 1;
		 }
		 else
		 {
			 n = 1;
		 }

		 if ((i + n) > StrLen)
		 {
			 break;
		 }
	 }

	 if (i > 0)
	 {
		 strncpy(BufRecv, Src, i);
		 return BufRecv;
	 }

	 return NULL;
}
