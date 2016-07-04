#include "SelfSelectStock.h"
#include "StkCodeOperate.h"
#include "../../quota/QuotesSnapshot.h"
#include "../../data_type.h"
#include "../../../util/log/log.h"
#include "../../../util/common/common_file.h"
#include "../../../util/common/common_lib.h"
#include "../../../util/time/time_util.h"
#include "../../../util/string/string_util.h"
#include "../../../controller/runtime.h"
#include <sys/mman.h>

#pragma pack(1)

// 文件结构头定义
struct SelfSelectRecHead
{
private:
	// 记录数量
	unsigned char MemCount;
	// 属性位定义 (0:是否接收标识 1:记录是否有更新 23456789:最近一次发送的统计个数值 10/11/12:已经发送统计信息地雷的次数 13:是否收到已读取回应)
	unsigned short Property;
	// 最近发送的信息地雷ID
	unsigned int LatestInfoMineCrc;
	// 统计的信息地雷总数
	unsigned char InfoMineTotalCount;
	
public:
	// 设置占用记录的数量
	void SetMemCount(const unsigned char Count)
	{
		MemCount = Count;
	}
	
	// 获取占用的记录的数量
	const unsigned char GetMemCount()
	{
		return MemCount;
	}
	
	// 设置接收标识
	void SetRecvMark(bool Accept)
	{
		Accept ? BitSet(Property, 0) : BitUnSet(Property, 0);
	}
	
	// 是否设置了接收标识
	bool HaveSetRecvMark()
	{
		return IsBitSet(Property, 0);
	}
	
	// 设置变更标识
	void SetModifiedMark(bool modified)
	{
		modified ? BitSet(Property, 1) : BitUnSet(Property, 1);
	}
	
	// 记录是否存在变更
	bool HaveModifiedMark()
	{
		return IsBitSet(Property, 1);
	}
	
	// 设置最近一次发送统计信息的次数
	void SetStaticsCount(const unsigned short Count)
	{
		unsigned short uCurrentValue = Property & 0xFC03;
		unsigned short uSetValue = Count;
		if (uSetValue > 255)
		{
			uSetValue = 255;
		}
		Property = uCurrentValue | ((uSetValue << 2) & 0x3FC);
	}
	
	// 获取最近一次发送统计信息的次数
	const unsigned char GetStaticsCount()
	{
		unsigned char uCurrentValue = Property & 0x3FC;
		uCurrentValue >>= 2;
		return uCurrentValue;
	}
	
	// 设置已经发送统计信息地雷的次数(不会大于发送统计的最大区间段)
	void SetHaveSendTimes(const unsigned short Count)
	{
		unsigned short uCurrentValue = Property & 0xE3FF;
		Property = uCurrentValue | ((Count << 10) & 0x1C00);
	}
	
	// 获取已经发送统计信息地雷的次数
	const unsigned char GetHaveSendTimes()
	{
		unsigned short uCurrentValue = Property & 0x1C00;
		uCurrentValue >>= 10;
		return uCurrentValue;
	}
	
	// 设置是否收到已读取回应
	void SetHaveReadMark(bool Accept)
	{
		Accept ? BitSet(Property, 13) : BitUnSet(Property, 13);
	}
	
	// 是否收到已读取回应
	bool HaveReadMark()
	{
		return IsBitSet(Property, 13);
	}
	
	// 设置信息地雷总数
	void SetInfomineTotalCount(const short Count)
	{
		InfoMineTotalCount = Count;
	}
	
	// 获取信息地雷总数
	const unsigned char GetInfomineTotalCount()
	{
		return InfoMineTotalCount;
	}

	// 重置清空
	void Clear()
	{
		SetMemCount(0);
		SetModifiedMark(false);
		SetStaticsCount(0);
		SetHaveSendTimes(0);
		SetHaveReadMark(false);
		SetLatestInfoMineCrc(0);
		SetInfomineTotalCount(0);
	}
	
	// 设置最近发送的信息地雷ID
	void SetLatestInfoMineCrc(const unsigned int Crc)
	{
		LatestInfoMineCrc = Crc;
	}
	
	// 获取最近发送的信息地雷ID
	const unsigned int GetLatestInfoMineCrc()
	{
		return LatestInfoMineCrc;
	}
};

// 文件本体定义
struct SelfSelectRecBody
{
	// 代码
	char StkCodeList[MAX_STKCODE_LEN];
};

#pragma pack()

CSelfSelectStock::CSelfSelectStock(const size_t MaxUser, const BYTE MaxRecPerUser, 
	const char *FileName)
	: m_uMaxUser(MaxUser),
	m_uMaxRecPerUser(MaxRecPerUser),
	m_iRecordLen(sizeof(SelfSelectRecBody)),
	m_iHeadLen(sizeof(SelfSelectRecHead)),
	m_pMemAddress(NULL),
	m_iMapFileSize(0),
	m_iLatestStatus(-1),
	m_uPerSegOffset(0),
	m_pStkCodeOperate(NULL)
{
	strncpy(m_strFileName, FileName, MAX_PATH_LEN);
}

CSelfSelectStock::~CSelfSelectStock()
{
	
}

// 创建内存映射
int
CSelfSelectStock::CreateMemmap()
{
	int iFd = -1;
	
	// 分段偏移量
	m_uPerSegOffset = m_iHeadLen + m_uMaxRecPerUser * m_iRecordLen + 1;
	// 计算最终文件大小
	m_iMapFileSize = (off_t)(m_uPerSegOffset * m_uMaxUser);
			
	if (!IsFileExist(m_strFileName))
	{
		iFd = open(m_strFileName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if (iFd < 0)
		{
			m_iLatestStatus = -1;
			return m_iLatestStatus;
		}

		// 设置文件大小
		if ((off_t)-1 == lseek(iFd, m_iMapFileSize + 1, SEEK_SET))
		{
			CloseFile(&iFd);
			m_iLatestStatus = -2;
			return m_iLatestStatus;
		}
		if (write(iFd, "\0", 1) < 0)
		{
			CloseFile(&iFd);
			m_iLatestStatus = -3;
			return m_iLatestStatus;
		}
		lseek(iFd, 0, SEEK_SET);
	}
	else
	{
		iFd = open(m_strFileName, O_RDWR, S_IRUSR | S_IWUSR);
		if (iFd < 0)
		{
			m_iLatestStatus = -4;
			return m_iLatestStatus;
		}
		
		long lFileLen = GetFileLength(m_strFileName);
		if ((m_iMapFileSize + 2) != lFileLen)
		{
			// 设置文件大小
			if ((m_iMapFileSize + 2) > lFileLen)
			{
				if ((off_t)-1 == lseek(iFd, m_iMapFileSize + 1, SEEK_SET))
				{
					CloseFile(&iFd);
					m_iLatestStatus = -5;
					return m_iLatestStatus;
				}
				if (write(iFd, "\0", 1) < 0)
				{
					CloseFile(&iFd);
					m_iLatestStatus = -6;
					return m_iLatestStatus;
				}
				lseek(iFd, 0, SEEK_SET);
			}
			else 
			{
				if (-1 == truncate(m_strFileName, m_iMapFileSize + 2))
				{
					CloseFile(&iFd);
					ERROR("truncate errno=%d", errno);
					m_iLatestStatus = -7;
					return m_iLatestStatus;
				}
			}
		}
	}
	
	// 打开映射
	m_pMemAddress = mmap(NULL, m_iMapFileSize, PROT_WRITE | PROT_READ,
						MAP_SHARED, iFd, 0);
	if (MAP_FAILED == m_pMemAddress)
	{
		CloseFile(&iFd);
		ERROR("mmap errno=%d %s  m_strFileName=%s", errno, strerror(errno), m_strFileName);
		m_iLatestStatus = -8;
		return m_iLatestStatus;
	}

	CloseFile(&iFd);
	m_iLatestStatus = 0;
	return m_iLatestStatus;
}

// 初始化数据读入
int
CSelfSelectStock::InitialLoad()
{
	// 初始化记录管理
	if (NULL == m_pStkCodeOperate)
	{
		m_pStkCodeOperate = new CStkCodeOperate(MAX_STKCODE_LEN);
	}
	
	if (0 != CreateMemmap())
	{
		ERROR("CreateMemmap error [%d]", m_iLatestStatus);
		return 0;
	}
	
	return 1;
}

// 释放分配的资源
void
CSelfSelectStock::ReleaseRes()
{
	// 释放映射地址
	if (0 == m_iLatestStatus && MAP_FAILED != m_pMemAddress)
	{
		munmap(m_pMemAddress, m_iMapFileSize);
		m_pMemAddress = MAP_FAILED;
	}
	
	if (NULL != m_pStkCodeOperate)
	{
		delete m_pStkCodeOperate;
		m_pStkCodeOperate = NULL;
	}
}

// 获取指定索引的头
SelfSelectRecHead*
CSelfSelectStock::GetIndexHead(const size_t NodeIndex)
{
	if (NodeIndex >= m_uMaxUser || 0 != m_iLatestStatus)
	{
		return NULL;
	}
	
	return (SelfSelectRecHead*)(((char*)m_pMemAddress) + m_uPerSegOffset * NodeIndex);
}

// 获取记录本体
void*
CSelfSelectStock::GetIndexBody(const size_t NodeIndex)
{
	if (NodeIndex >= m_uMaxUser || 0 != m_iLatestStatus)
	{
		return NULL;
	}
	return (SelfSelectRecHead*)(((char*)m_pMemAddress) + m_uPerSegOffset * NodeIndex + m_iHeadLen);
}

// 添加代码
const int
CSelfSelectStock::AddSelfCode(const size_t NodeIndex, const char *StkCode)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	char *pBody = (char*)GetIndexBody(NodeIndex);
	
	if (NULL == pHead || NULL == pBody || NULL == m_pStkCodeOperate)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return -36;
	}
	
	SelfSelectStockInfomineConfigT *pSelfSelectStockInfomineConfig = GetSelfSelectStockInfomineConfig();
	if (pSelfSelectStockInfomineConfig->ExcludeCodeCount > 0)
	{
		if (NULL != strstr(pSelfSelectStockInfomineConfig->ExcludeCode, StkCode))
		{
			DEBUG("ExcludeCode found");
			return 0;
		}
	}
	
	unsigned short nStkIndex = CQuotesSnapshot::findSecurityIndex(StkCode);
	if (0xFFFF == nStkIndex)
	{
		if (CQuotesSnapshot::HasInitialed())
		{
			// 选择的股票不支持
			return -37;
		}
		// 正在初始化
		return -22;
	}
	
	// 过滤指数代码
	ACC_STK_STATIC *pStaticStk = CQuotesSnapshot::GetIndexStatic(nStkIndex, StkCode);
	if (NULL == pStaticStk || ACC_ORIGINSTK_STATIC::INDEX == pStaticStk->m_cType)
	{
		DEBUG("Index code has been found[%s]", StkCode);
		return 0;
	}
	
	unsigned int OccupySize = pHead->GetMemCount() * m_iRecordLen;
	int iRes = m_pStkCodeOperate->AddCode(StkCode, pBody, OccupySize, m_iRecordLen * m_uMaxRecPerUser);
	
	if (0 != iRes)
	{
		TRACE("self stock AddCode failed, res = %d", iRes);
		return -38;
	}
	
	pHead->SetMemCount(OccupySize / m_iRecordLen);
	return 0;
}

// 删除代码
const int
CSelfSelectStock::RemoveSelfCode(const size_t NodeIndex, const char *StkCode)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	char *pBody = (char*)GetIndexBody(NodeIndex);
	
	if (NULL == pHead || NULL == pBody || NULL == m_pStkCodeOperate)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return -36;
	}
	
	unsigned int OccupySize = pHead->GetMemCount() * m_iRecordLen;
	int iRes = m_pStkCodeOperate->RemoveCode(StkCode, pBody, OccupySize, m_iRecordLen * m_uMaxRecPerUser);
	
	if (0 != iRes)
	{
		return -39;
	}
	
	pHead->SetMemCount(OccupySize / m_iRecordLen);
	return 0;
}

// 清空代码
const int
CSelfSelectStock::ClearAllSelfCode(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	char *pBody = (char*)GetIndexBody(NodeIndex);
	
	if (NULL == pHead || NULL == pBody || NULL == m_pStkCodeOperate)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return -36;
	}
	
	pHead->Clear();
	memset(pBody, 0, m_iRecordLen * m_uMaxRecPerUser);
	return 0;
}

// 读取指定索引的代码
const char*
CSelfSelectStock::ReadSelfIndexCode(const size_t NodeIndex, const size_t CodeIndex, 
	char *OutBuf, const unsigned int OutBufSize)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	char *pBody = (char*)GetIndexBody(NodeIndex);
	
	if (NULL == pHead || NULL == pBody || NULL == m_pStkCodeOperate)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return NULL;
	}
	
	unsigned int OccupySize = pHead->GetMemCount() * m_iRecordLen;
	return m_pStkCodeOperate->ReadIndexCode(pBody, OccupySize, CodeIndex, 
			OutBuf, OutBufSize);
}

// 读取指定节点的记录数量
const int
CSelfSelectStock::GetSelfCodeCount(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return -1;
	}
	return pHead->GetMemCount();
}

// 设置指定节点的记录数量
void
CSelfSelectStock::SetSelfCodeCount(const size_t NodeIndex, const unsigned char MemCount)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return;
	}
	pHead->SetMemCount(MemCount);
}

// 是否设置了接收信息地雷标记
bool
CSelfSelectStock::IsRecvMarkSet(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return false;
	}
	return pHead->HaveSetRecvMark();
}

// 设置接收标识
void
CSelfSelectStock::SetRecvMark(const size_t NodeIndex, bool MarkValue)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return;
	}
	pHead->SetRecvMark(MarkValue);
}

// 设置变更标识
void
CSelfSelectStock::SetModifiedMark(const size_t NodeIndex, bool MarkValue)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return;
	}
	pHead->SetModifiedMark(MarkValue);
}

// 记录是否存在变更
bool
CSelfSelectStock::IsModified(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return false;
	}
	return pHead->HaveModifiedMark();
}

// 设置最近一次发送统计信息的次数
void
CSelfSelectStock::SetStaticsCount(const size_t NodeIndex, const unsigned short Count)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return;
	}
	pHead->SetStaticsCount(Count);
}

// 获取最近一次发送统计信息的次数
const int
CSelfSelectStock::GetStaticsCount(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return -1;
	}
	return pHead->GetStaticsCount();
}

// 设置已经发送统计信息地雷的次数(不会大于发送统计的最大区间段)
void
CSelfSelectStock::SetHaveSendTimes(const size_t NodeIndex, const unsigned short Count)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return;
	}
	pHead->SetHaveSendTimes(Count);
}

// 获取已经发送统计信息地雷的次数
const int
CSelfSelectStock::GetHaveSendTimes(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return -1;
	}
	return pHead->GetHaveSendTimes();
}

// 设置是否收到已读取回应
void
CSelfSelectStock::SetHaveReadMark(const size_t NodeIndex, bool Accept)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return;
	}
	pHead->SetHaveReadMark(Accept);
}

// 是否收到已读取回应
bool
CSelfSelectStock::HaveReadMark(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return false;
	}
	return pHead->HaveReadMark();
}

// 重置
void
CSelfSelectStock::Reset(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return;
	}
	
	pHead->SetModifiedMark(false);
	pHead->SetStaticsCount(0);
	pHead->SetHaveSendTimes(0);
	pHead->SetHaveReadMark(0);
	pHead->SetInfomineTotalCount(0);
}

// 获取全部代码串
const char*
CSelfSelectStock::GetTotalSelfStkCode(const size_t NodeIndex)
{
	return (const char*)GetIndexBody(NodeIndex);
}

// 设置代码串
void
CSelfSelectStock::SetTotalSelfStkCode(const size_t NodeIndex, const char *CodeList)
{
	ClearAllSelfCode(NodeIndex);
	char cStkCode[MAX_STKCODE_LEN + 1] = {0};
	int iCount = strlen(CodeList) / MAX_STKCODE_LEN;
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	char *pBody = (char*)GetIndexBody(NodeIndex);
	
	int iOffset = 0;
	for (int i = iCount - 1; i >= 0; i--)
	{
		memset(cStkCode, 0, MAX_STKCODE_LEN);
		iOffset = i * MAX_STKCODE_LEN;
		StrCopy(cStkCode, CodeList + iOffset, MAX_STKCODE_LEN);
	
		if (!IsEmptyString(cStkCode))
		{
			unsigned int OccupySize = pHead->GetMemCount() * m_iRecordLen;
			int iRes = m_pStkCodeOperate->AddCode(cStkCode, pBody, OccupySize, m_iRecordLen * m_uMaxRecPerUser);
			
			if (0 <= iRes)
			{
				pHead->SetMemCount(OccupySize / m_iRecordLen);
				pHead->SetRecvMark(true);
			}
		}
	}
}

// 设置最近发送的信息地雷ID
void
CSelfSelectStock::SetLatestInfoMineCrc(const size_t NodeIndex, const unsigned int Crc)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return;
	}
	pHead->SetLatestInfoMineCrc(Crc);
}

// 获取最近发送的信息地雷ID
const unsigned int 
CSelfSelectStock::GetLatestInfoMineCrc(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return (unsigned int)-1;
	}
	return pHead->GetLatestInfoMineCrc();
}

// 获取证券代码在整体中的索引
const int
CSelfSelectStock::GetStkCodeIndex(const size_t NodeIndex, const char *StkCode)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	char *pBody = (char*)GetIndexBody(NodeIndex);
	if (NULL == pHead || NULL == pBody || NULL == m_pStkCodeOperate)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return -1;
	}

	unsigned int OccupySize = pHead->GetMemCount() * m_iRecordLen;
	return m_pStkCodeOperate->FindStkCode(pBody, OccupySize, StkCode);
}

// 设置信息地雷总数
void
CSelfSelectStock::SetInfomineTotalCount(const size_t NodeIndex, const short Count)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return;
	}
	pHead->SetInfomineTotalCount(Count);
}

// 获取信息地雷总数
const unsigned char
CSelfSelectStock::GetInfomineTotalCount(const size_t NodeIndex)
{
	SelfSelectRecHead *pHead = GetIndexHead(NodeIndex);
	if (NULL == pHead)
	{
		DEBUG("SelfSelectStock In error [%d]", m_iLatestStatus);
		return (unsigned char)-1;
	}
	return pHead->GetInfomineTotalCount();
}

