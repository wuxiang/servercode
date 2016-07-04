#include "InfoMine.h"
#include "CppODBC.h"
#include "../../../util/util.h"
#include "../../../util/log/log.h"
#include "../../../util/string/string_util.h"
#include "../../../util/time/time_util.h"
#include "../../../controller/runtime.h"
#include "../../config.h"
#include <iostream>
#include <algorithm>
#include <map>


/**************************************
** 由于数据库设置无请求用户的存活时间为15秒
** 程序不需要如此密集的请求，因此将连接改为短链
** 即每次请求的时候建立连接，完成后断开连接
***************************************/

using namespace std;

CInfoMine * CInfoMine::m_pInfoMine = NULL;
struct InfoMineStatisTag
{
	// 统计的信息地雷数目
	unsigned int MemCount;
	// 统计时刻的信息地雷最大ID
	unsigned long EndMsgID;
	
	InfoMineStatisTag()
	{
	}
	
	InfoMineStatisTag(const unsigned int Count, const unsigned long MaxID)
		: MemCount(Count),EndMsgID(MaxID)
	{
	}
	
	InfoMineStatisTag& operator= (const InfoMineStatisTag &item)
	{
		MemCount = item.MemCount;
		EndMsgID = item.EndMsgID;
		return *this;
	}
};
	
CInfoMine::CInfoMine(const unsigned int MaxNum, const char*dsn, 
	const char*name, const char*pwd)
	: m_uMaxCacheNum(MaxNum),
	m_pCppODBC(new CppODBC()),
	m_pRecordCache(new IMmessage[m_uMaxCacheNum]),
	m_uOccupyIndex(0),
	m_luCurrentMaxRecID(0),
	m_bStartRTStatis(false),
	m_iCurrentStatus(-1)
{
	StrCopy(m_dsn, dsn, INFOMINE_LINK_LEN);
	StrCopy(m_uname, name, INFOMINE_LINK_LEN);
	StrCopy(m_upass, pwd, INFOMINE_LINK_LEN);
	memset(m_pRecordCache, -1, m_uMaxCacheNum * sizeof(IMmessage));
	InitialRes();
}

CInfoMine::~CInfoMine(void)
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
	
	m_mpMarkNodes.clear();
	m_mpInfoMineStatics.clear();
}

const unsigned int 
CInfoMine::GetCirculateIndex(const unsigned int index)
{
	if (index < m_uMaxCacheNum)
	{
		return index;
	}

	return index % m_uMaxCacheNum;
}

const int
CInfoMine::AddNewRecord(const IMmessage &record, unsigned int &RecordIndex)
{
	unsigned int uCurrentId = GetCirculateIndex(m_uOccupyIndex++);
	
	IMmessage *pSetRecord = GetIndexIMmessage(uCurrentId);
	if (NULL == pSetRecord)
	{
		return -1;
	}

	// 处理当前位置
	map<const char*, unsigned int, NodeMarkCmpFun>::iterator iterFind;
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

	bzero(pSetRecord, sizeof(IMmessage));
	*pSetRecord = record;
	UpdateLinkNode(pSetRecord->strStockCode, uCurrentId);
	RecordIndex = uCurrentId;

	return 0;
}

// 获取指定证券的头位置索引
const unsigned int
CInfoMine::GetStkInfoMineHeadIndex(const char *stkCode)
{
	unsigned int uIndex = (unsigned int)-1;
	if (NULL == stkCode)
	{
		return uIndex;
	}

	map<const char*, unsigned int, NodeMarkCmpFun>::iterator iterFind = m_mpMarkNodes.find(stkCode);
	if (m_mpMarkNodes.end() == iterFind)
	{
		return uIndex;
	}

	return iterFind->second;
}

// 获取指定证券的尾位置索引
const unsigned int
CInfoMine::GetStkInfoMineTailIndex(const unsigned int HeadIndex)
{
	unsigned int uIndex = (unsigned int)-1;
	IMmessage *pCurrent = GetIndexIMmessage(HeadIndex);
	if (NULL == pCurrent)
	{
		return uIndex;
	}

	uIndex = HeadIndex;
	while(NULL != pCurrent && (unsigned int)-1 != pCurrent->Previous)
	{
		uIndex = pCurrent->Previous;
		pCurrent = GetIndexIMmessage(uIndex);
	}

	return uIndex;
}

// 查找指定MsgID的信息位置索引
const unsigned int
CInfoMine::GetIMmessageIndexByMsgID(const unsigned int HeadIndex, const unsigned long MsgID)
{
	unsigned int uIndex = (unsigned int)-1;
	IMmessage *pCurrent = GetIndexIMmessage(HeadIndex);
	if (NULL == pCurrent)
	{
		return uIndex;
	}
	
	if (pCurrent->MsgID == MsgID)
	{
		uIndex = HeadIndex;
		return uIndex;
	}
	
	while(NULL != pCurrent && (unsigned int)-1 != pCurrent->Previous)
	{
		uIndex = pCurrent->Previous;
		pCurrent = GetIndexIMmessage(uIndex);
		if (NULL == pCurrent)
		{
			uIndex = (unsigned int)-1;
			break;
		}
		else if (pCurrent->MsgID == MsgID)
		{
			break;
		}
	}
	
	if ((unsigned int)-1 != uIndex)
	{
		pCurrent = GetIndexIMmessage(uIndex);
		if (NULL == pCurrent || pCurrent->MsgID != MsgID)
		{
			uIndex = (unsigned int)-1;
		}
	}

	return uIndex;
}

// 查找比指定ID大的信息位置索引
const unsigned int
CInfoMine::GetIMmessageIndexLargerThanMsgID(const unsigned int HeadIndex, const unsigned long MsgID)
{
	unsigned int uIndex = (unsigned int)-1;
	IMmessage *pCurrent = GetIndexIMmessage(HeadIndex);
	if (NULL == pCurrent || MsgID >= pCurrent->MsgID)
	{
		return uIndex;
	}
	
	if (0 == MsgID)
	{
		uIndex = HeadIndex;
		while(NULL != pCurrent && (unsigned int)-1 != pCurrent->Previous)
		{
			uIndex = pCurrent->Previous;
			pCurrent = GetIndexIMmessage(uIndex);
			if (NULL == pCurrent)
			{
				uIndex = (unsigned int)-1;
				break;
			}
		}
		return uIndex;
	}
	
	if ((unsigned int)-1 == pCurrent->Previous)
	{
		if (MsgID > pCurrent->MsgID)
		{
			uIndex = pCurrent->MsgID;
			return uIndex;
		}
	}
	
	while(NULL != pCurrent && (unsigned int)-1 != pCurrent->Previous)
	{
		uIndex = pCurrent->Previous;
		pCurrent = GetIndexIMmessage(uIndex);
		if (NULL == pCurrent)
		{
			uIndex = (unsigned int)-1;
			break;
		}
		else if (MsgID >= pCurrent->MsgID)
		{
			uIndex = pCurrent->Next;
			break;
		}
	}
	
	return uIndex;
}

// 获取指定索引的记录指针
IMmessage*
CInfoMine::GetIndexIMmessage(const unsigned int NodeIndex)
{
	if (NodeIndex >= m_uMaxCacheNum)
	{
		return NULL;
	}

	return &m_pRecordCache[NodeIndex];
}

// 移除指定索引的记录
const int
CInfoMine::RemoveIndexNode(const unsigned int index)
{
	IMmessage *pCurrentNode = GetIndexIMmessage(index);
	if (NULL == pCurrentNode)
	{
		return (unsigned int)-1;
	}

	map<const char*, unsigned int, NodeMarkCmpFun>::iterator iterFind;
	if ((unsigned int)-1 == pCurrentNode->Next && (unsigned int)-1 == pCurrentNode->Previous)
	{
		m_mpMarkNodes.erase(pCurrentNode->strStockCode);
	}
	else if ((unsigned int)-1 == pCurrentNode->Next && (unsigned int)-1 != pCurrentNode->Previous)
	{
		iterFind = m_mpMarkNodes.find(pCurrentNode->strStockCode);
		if (m_mpMarkNodes.end() == iterFind)
		{
			return -2;
		}

		IMmessage *pPreviousNode = GetIndexIMmessage(pCurrentNode->Previous);
		if (NULL == pPreviousNode)
		{
			return -3;
		}

		pPreviousNode->Next = (unsigned int)-1;
		iterFind->second = pCurrentNode->Previous;
	}
	else if ((unsigned int)-1 != pCurrentNode->Next && (unsigned int)-1 == pCurrentNode->Previous)
	{
		IMmessage *pNextNode = GetIndexIMmessage(pCurrentNode->Next);
		if (NULL == pNextNode)
		{
			return -4;
		}

		pNextNode->Previous = (unsigned int)-1;
	}
	else 
	{
		IMmessage *pNextNode = GetIndexIMmessage(pCurrentNode->Next);
		IMmessage *pPreviousNode = GetIndexIMmessage(pCurrentNode->Previous);

		if (NULL == pNextNode || NULL == pPreviousNode)
		{
			return -5;
		}

		pNextNode->Previous = pCurrentNode->Previous;
		pPreviousNode->Next = pCurrentNode->Next;
	}
	
	return 0;
}

// 更新证券的信息头位置索引
bool
CInfoMine::UpdateLinkNode(const char *stkCode, const unsigned int Index)
{
	map<const char*, unsigned int, NodeMarkCmpFun>::iterator iterFind = m_mpMarkNodes.find(stkCode);
	if (m_mpMarkNodes.end() == iterFind)
	{
		return m_mpMarkNodes.insert(pair<const char*, unsigned int>(stkCode, Index)).second;
	}

	unsigned int uFoundIndex = iterFind->second;
	IMmessage *pPreviousNode = GetIndexIMmessage(uFoundIndex);
	IMmessage *pHeadNode = GetIndexIMmessage(Index);
	if (NULL == pPreviousNode || NULL == pHeadNode)
	{
		return false;
	}
	pPreviousNode->Next = Index;
	pHeadNode->Previous = uFoundIndex;
	pHeadNode->Next = (unsigned int)-1;

	m_mpMarkNodes.erase(iterFind);
	return m_mpMarkNodes.insert(pair<const char*, unsigned int>(stkCode, Index)).second;
}

// 初始化资源
void
CInfoMine::InitialRes()
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
CInfoMine::resetData()
{
	memset(m_pRecordCache, -1, m_uMaxCacheNum * sizeof(IMmessage));
	m_uOccupyIndex = 0;
	m_mpMarkNodes.clear();
	m_mpInfoMineStatics.clear();
	m_mpInfoMineRT.clear();
}

bool
CInfoMine::DBconnect()
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
CInfoMine::DBdisconnect()
{
	if (NULL != m_pCppODBC)
	{
		return m_pCppODBC->DisConnect();
	}
	
    return false;
}

// 读取当天的信息地雷
int 
CInfoMine::CompanyNews_Query_CurDate()
{
	char sql[1024] = {0};
    int column = 0;
    int iRet = 0;
    int iGotCount = 0;
    unsigned int AddIndex = (unsigned int)-1;

    sprintf(sql, "SELECT cn.stockcode, cn.id, LEFT(cn.title, 63), UNIX_TIMESTAMP(cn.newsdate) \
			FROM TB_GG_Stock cn WHERE cn.localtype = 1 \
			AND cn.newsdate > CURDATE() AND cn.id > %lu ORDER BY cn.id ASC;", 
			m_luCurrentMaxRecID);
			
    iRet = m_pCppODBC->SQLQuery(sql);
    if( (iRet <= 0) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
    {
    	return -1;
    }
    
    if(iRet > 0)
    {
    	iGotCount += iRet;
        while( !m_pCppODBC->Eof())
        {
        	column = 0;
        	IMmessage msg;
        	bzero(&msg, sizeof(IMmessage));
        	// 证券名称
        	StrCopy(msg.strStockCode, m_pCppODBC->GetStrValue(column++), MAX_STKCODE_LEN);
        	// 数据库中ID
            msg.MsgID = m_pCppODBC->GetULongValue(column++);
            m_luCurrentMaxRecID = msg.MsgID;
            // 标题
            bzero(msg.strTitle, INFOMINE_MAXTITLELEN);
			StrCopy(msg.strTitle, m_pCppODBC->GetStrValue(column++), INFOMINE_MAXTITLELEN);
            // 发表时间
			msg.ttNewsTime = m_pCppODBC->GetTimetValue(column++);
			
			AddIndex = (unsigned int)-1;
			if (0 > AddNewRecord(msg, AddIndex))
			{
				DEBUG("AddNewRecord error.");
			}
			
			AddRTInfoMine(AddIndex, msg);
			
			m_pCppODBC->Next();
        }
        
        TRACE("Read MaxID[%lu] from Db.", m_luCurrentMaxRecID);
    }

    return iGotCount;
}

// 初始读取有效的信息地雷
int
CInfoMine::ReadValidInfoMine()
{
	char sql[1024] = {0};
    int column = 0;
    int iRet = 0;
    int iGotCount = 0;
    unsigned int AddIndex = (unsigned int)-1;

    sprintf(sql, "SELECT cn.stockcode, cn.id, LEFT(cn.title, 63), UNIX_TIMESTAMP(cn.newsdate) \
			FROM TB_GG_Stock cn WHERE cn.localtype = 1 \
			AND cn.newsdate > DATE_SUB(CURDATE(), INTERVAL 1 DAY) AND cn.id > %lu ORDER BY cn.id ASC;", 
			m_luCurrentMaxRecID);
			
    iRet = m_pCppODBC->SQLQuery(sql);
    if( (iRet <= 0) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
    {
    	return -1;
    }
    
    if(iRet > 0)
    {
    	iGotCount += iRet;
        while( !m_pCppODBC->Eof() )
        {
        	column = 0;
        	IMmessage msg;
        	bzero(&msg, sizeof(IMmessage));
        	// 证券名称
        	StrCopy(msg.strStockCode, m_pCppODBC->GetStrValue(column++), MAX_STKCODE_LEN);
        	// 数据库中ID
            msg.MsgID = m_pCppODBC->GetULongValue(column++);
            m_luCurrentMaxRecID = msg.MsgID;
            // 标题
            bzero(msg.strTitle, INFOMINE_MAXTITLELEN);
			StrCopy(msg.strTitle, m_pCppODBC->GetStrValue(column++), INFOMINE_MAXTITLELEN);
            // 发表时间
			msg.ttNewsTime = m_pCppODBC->GetTimetValue(column++);
			
			AddIndex = (unsigned int)-1;
			if (0 > AddNewRecord(msg, AddIndex))
			{
				DEBUG("AddNewRecord error.");
			}
			
			m_pCppODBC->Next();
        }
    }

    return iGotCount;
}

CInfoMine * 
CInfoMine::getInfoMinePointer()
{
	const unsigned int uMaxRecordNum = 5000;
	if( NULL == m_pInfoMine)
	{
		const WapInfoMineConfig *pIMConfig = GetInfoMineSetting();
		m_pInfoMine = new CInfoMine(uMaxRecordNum, pIMConfig->DSN, pIMConfig->LogUser, pIMConfig->LogPwd);
	}
	return m_pInfoMine;
}

void 
CInfoMine::releaseInfoMinePointer()
{
	if (NULL != m_pInfoMine)
		delete m_pInfoMine;
}

// 初始化请求
bool
CInfoMine::InitialData()
{
	int iRet = -1;
	resetData();							// 重置数据
	
	if (!DBconnect())
	{
		return false;
	}
	
	SetCurrentStatus(1);
    iRet = ReadValidInfoMine();
    SetCurrentStatus(0);
    DBdisconnect();
    if (iRet < 0)
	{
		DEBUG("ReadValidInfoMine failed");
		return false;
	}

    return true;
}

// 增量请求
const int
CInfoMine::GetDynaData()
{
	if (!DBconnect())
	{
		return -1;
	}
	
	SetCurrentStatus(2);
	int iRet = CompanyNews_Query_CurDate();
	SetCurrentStatus(0);
	DBdisconnect();
	return iRet;
}

// 设置最新状态
void 
CInfoMine::SetCurrentStatus(const int Value)
{
	m_iCurrentStatus = Value;
}

// 请求最新的消息
const unsigned long
CInfoMine::GetLatestMsg(const char *stkCode, const unsigned long MsgID, IMmessage &msg)
{
	unsigned long uOutID = (unsigned long)-1;
	unsigned int uHeadIndex = GetStkInfoMineHeadIndex(stkCode);
	if ((unsigned int)-1 == uHeadIndex)
	{
		return uOutID;
	}
	
	unsigned int uFindIndex = GetIMmessageIndexLargerThanMsgID(uHeadIndex, MsgID);
	if ((unsigned int)-1 == uFindIndex)
	{
		return uOutID;
	}
	
	IMmessage *pResult = GetIndexIMmessage(uFindIndex);
	if (NULL == pResult)
	{
		return uOutID;
	}
	
	bzero(&msg, sizeof(IMmessage));
	msg = *pResult;
	return pResult->MsgID;
}

// 获取指定的消息
bool
CInfoMine::GetIndexIMmessage(const char *stkCode, const unsigned long MsgID, IMmessage &msg)
{
	unsigned int uHeadIndex = GetStkInfoMineHeadIndex(stkCode);
	if ((unsigned int)-1 == uHeadIndex)
	{
		return false;
	}
	
	unsigned int uFindIndex = GetIMmessageIndexByMsgID(uHeadIndex, MsgID);
	if ((unsigned int)-1 == uFindIndex)
	{
		return false;
	}
	
	IMmessage *pResult = GetIndexIMmessage(uFindIndex);
	if (NULL == pResult)
	{
		return false;
	}
	
	bzero(&msg, sizeof(IMmessage));
	msg = *pResult;
	return true;
}

// 执行信息地雷的个数统计
const int
CInfoMine::ExecuteInfoMineStatis(const int CriterionalTime)
{	
	time_t uCmpTimeValue = ExchangeSixTimeToTime_t(CriterionalTime);
	map<const char*, unsigned int, NodeMarkCmpFun>::iterator iterFind = m_mpMarkNodes.begin();
	IMmessage *pCurrent = NULL;
	unsigned int uIndex = (unsigned int)-1;
	int iInfoMineCount = 0;
	m_mpInfoMineStatics.clear();
	unsigned long uHeadID = 0;
	while(m_mpMarkNodes.end() != iterFind)
	{
		iInfoMineCount = 0;
		pCurrent = GetIndexIMmessage(iterFind->second);
		if (NULL== pCurrent)
		{
			iterFind++;
			continue;
		}
		
		uHeadID = pCurrent->MsgID;
		do 
		{
			if (pCurrent->ttNewsTime >= uCmpTimeValue)
			{
				iInfoMineCount++;;
			}
			
			uIndex = pCurrent->Previous;
			if ((unsigned int)-1 == uIndex)
			{
				break;
			}
			pCurrent = GetIndexIMmessage(uIndex);
			
		} while(NULL != pCurrent);
		
		// 记录结果
		if (iInfoMineCount > 0)
		{
			m_mpInfoMineStatics[iterFind->first] = InfoMineStatisTag(iInfoMineCount, uHeadID);
		}
		
		iterFind++;
	}
	
	TRACE("Static Info Show CurrentMaxRecID=%lu, StkCodeCount=%u", m_luCurrentMaxRecID, (unsigned int)m_mpInfoMineStatics.size());
	
	return m_mpInfoMineStatics.size();
}

// 获取证券的信息地雷个数
const unsigned int
CInfoMine::GetStockInfoMineCount(const char *stkCode)
{
	if (NULL == stkCode || m_mpInfoMineStatics.empty())
	{
		return 0;
	}
	
	map<string, InfoMineStatisTag>::iterator iterFind = m_mpInfoMineStatics.find(stkCode);
	if (m_mpInfoMineStatics.end() != iterFind)
	{
		return iterFind->second.MemCount;
	}
	return 0;
}

// 信息地雷统计是否执行成功
bool
CInfoMine::IsInfoMineStatisSuccess()
{
	return m_mpInfoMineStatics.size() > 0;
}

// 添加实时发送的信息地雷索引
bool
CInfoMine::AddRTInfoMine(const unsigned int MsgIndex, const IMmessage &msg)
{
	if (!m_bStartRTStatis)
	{
		return false;
	}
	
	m_mpInfoMineRT[msg.MsgID] = MsgIndex;
	return true;
}

// 设置是否开始实时发送信息地雷
void
CInfoMine::SetStartRTInfoMineFlag(bool Flag)
{
	m_bStartRTStatis = Flag;
}

// 请求最新的消息
const int
CInfoMine::GetLatestMsg(const unsigned long MsgID, IMmessage &msg)
{
	map<unsigned long, unsigned int>::reverse_iterator iterFind = m_mpInfoMineRT.rbegin();
	unsigned int MsgIndex = (unsigned int)-1;
	unsigned int uMaxLoop = m_mpInfoMineRT.size();
	if (0 == MsgID)
	{
		uMaxLoop = 3;
	}
	
	unsigned int i = 0;
	while(m_mpInfoMineRT.rend() != iterFind && i < uMaxLoop)
	{
		if (iterFind->first <= MsgID)
		{
			break;
		}
		MsgIndex = iterFind->second;
		iterFind++;
		i++;
	}
	
	if ((unsigned int)-1 == MsgIndex)
	{
		return -1;
	}
	
	IMmessage *pSetRecord = GetIndexIMmessage(MsgIndex);
	if (NULL == pSetRecord)
	{
		return -2;
	}
	
	bzero(&msg, sizeof(IMmessage));
	msg = *pSetRecord;
	return 0;
}

// 获取当前的状态值
const int
CInfoMine::GetCurrentStatus()const
{
	return m_iCurrentStatus;
}

