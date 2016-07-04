#include "PlatformEWarnManage.h"
#include "EarlyWarning.h"
#include "../../../util/util.h"
#include "../../../util/log/log.h"
#include "../../../util/common/common_file.h"
#include "../../../util/common/common_lib.h"
#include "../../../util/time/time_util.h"
#include "../../../util/string/string_util.h"
#include "../../../controller/runtime.h"
#include <sys/mman.h>

CPlatformEWarnManage::CPlatformEWarnManage(const size_t uMaxEWarnUser, const size_t uOccupyCount,
	const BYTE uMaxEWarnPerUser, const BYTE uDefaultEWarnPerUser,
	const size_t RecordLen, const char *strFileName)
	: m_uMaxEWarnUser(uMaxEWarnUser),
	m_uOccupyCount(uOccupyCount),
	m_uMaxEWarnPerUser(uMaxEWarnPerUser),
	m_uDefaultEWarnPerUser(uDefaultEWarnPerUser),
	m_iRecordLen(RecordLen),
	m_pMemAddress(MAP_FAILED),
	m_iMapFileSize(0),
	m_iLatestStatus(-1),
	m_uPerSegOffset(0)
{
	strncpy(m_strFileName, strFileName, MAX_PATH_LEN);
}

CPlatformEWarnManage::~CPlatformEWarnManage()
{
}

// 更新设置新节点
const int
CPlatformEWarnManage::UpdateSetNode(const int HeadIndex, const CEarlyWarningConditionRecord &Node)
{
	CEarlyWarningConditionRecord *pHeadNode = GetIndexNode(HeadIndex);
	if (NULL == pHeadNode)
	{
		return -1;
	}

	bool bAddNew = true;
	unsigned char uCurrentCount = pHeadNode->GetEWarnCount();
	// 匹配代码
	short iIndexMatched = -1;
	unsigned char uCmpCount = 0;
	for (unsigned char i = 0; uCmpCount < uCurrentCount && i < m_uMaxEWarnPerUser; i++)
	{
		if (WARNINGSET_DELDB == pHeadNode[i].GetOperToDb())
		{
			if (-1 == iIndexMatched)
			{
				iIndexMatched = i;
				bAddNew = true;
			}
			continue;
		}

		if (0 == StrNoCaseCmp(pHeadNode[i].GetStkCode(), Node.GetStkCode(), MAX_STKCODE_LEN))
		{
			iIndexMatched = i;
			bAddNew = false;
			break;
		}
		uCmpCount++;
	}

	// 更新
	if (iIndexMatched > -1)
	{
		m_iAttachedPos = iIndexMatched;
		if (bAddNew)
		{
			pHeadNode[m_iAttachedPos] = Node;
			pHeadNode->UpdateSetEWarnCount(++uCurrentCount);
		}
		else
		{
			pHeadNode[m_iAttachedPos].UpdateValidRecord(&Node);
		}
		// 更新属性(原有删除的属性也可能被更新)
		pHeadNode[m_iAttachedPos].UpdateSetOperToDb(WARNINGSET_UPDATEDB);
	}
	// 新加
	else
	{
		m_iAttachedPos = uCurrentCount;
		pHeadNode[m_iAttachedPos] = Node;
		// 更新属性
		pHeadNode[m_iAttachedPos].UpdateSetOperToDb(WARNINGSET_INSERTDB);
		pHeadNode->UpdateSetEGroupUseFlag(WARNINGSET_USE);
		pHeadNode->UpdateSetEWarnCount(++uCurrentCount);
	}
	return 0;
}

// 节点(组)是否有效
bool
CPlatformEWarnManage::IsKeepValid(const int HeadIndex)
{
	CEarlyWarningConditionRecord *pHeadNode = GetIndexNode(HeadIndex);
	if (NULL == pHeadNode)
	{
		return false;
	}
	return (0 != pHeadNode->GetEGroupUseFlag());
}

// 执行整理无效节点(组)
void
CPlatformEWarnManage::ProcessArrangeInvalidNode()
{
	// 标识位已经设置，不做具体的处理
}

// 创建内存映射
int
CPlatformEWarnManage::CreateMemmap()
{
	int iFd = -1;

	// 分段偏移量
	m_uPerSegOffset = m_uMaxEWarnPerUser * m_iRecordLen;
	// 计算最终文件大小
	m_iMapFileSize = (off_t)(m_uPerSegOffset * m_uMaxEWarnUser);

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
		m_iLatestStatus = -7;
		return m_iLatestStatus;
	}

	CloseFile(&iFd);
	m_iLatestStatus = 0;
	return m_iLatestStatus;
}

// 初始化数据读入
int
CPlatformEWarnManage::InitialLoad()
{
	if (0 != CreateMemmap())
	{
		ERROR("CreateMemmap error [%d]", m_iLatestStatus);
		return 0;
	}

	int iRes = Construct(m_pMemAddress, m_iRecordLen, m_uMaxEWarnUser, m_uOccupyCount, m_uMaxEWarnPerUser);
	if (iRes < 0)
	{
		ERROR("Construct error ");
		return 0;
	}

	return 1;
}

// 释放分配的资源
void
CPlatformEWarnManage::ReleaseRes()
{
	// 释放映射地址
	if (0 == m_iLatestStatus && MAP_FAILED != m_pMemAddress)
	{
		munmap(m_pMemAddress, m_iMapFileSize);
		m_pMemAddress = MAP_FAILED;
	}
}

// 获取当前设置的有效预警的数目
const short
CPlatformEWarnManage::GetEarlyWarningNum(const size_t serial)
{
	if ((size_t)-1 == serial)
	{
		return 0;
	}

	CEarlyWarningConditionRecord *pHeadNode = GetIndexNode(serial);
	if (NULL == pHeadNode)
	{
		return -1;
	}

	return pHeadNode->GetEWarnCount();
}

// 获取指定的预警记录设置
CEarlyWarningConditionRecord*
CPlatformEWarnManage::GetConditionRecord(const size_t serial, const unsigned int RecordMarkID)
{
	CEarlyWarningConditionRecord *pHeadNode = GetIndexNode(serial);
	if (NULL == pHeadNode)
	{
		return NULL;
	}

	short uCurrentCount = pHeadNode->GetEWarnCount();
	if (uCurrentCount <= 0)
	{
		return NULL;
	}

	if (RecordMarkID >= GetMaxEWarnPerUser())
	{
		return NULL;
	}

	return &pHeadNode[RecordMarkID];
}

// 获取每个用户的最大预警数
const BYTE
CPlatformEWarnManage::GetMaxEWarnPerUser()const
{
	return m_uMaxEWarnPerUser;
}

// 获取每个用户的默认的预警数
const BYTE
CPlatformEWarnManage::GetDefaultEWarnPerUser()const
{
	return m_uDefaultEWarnPerUser;
}

// 获取指定的预警记录设置
CEarlyWarningConditionRecord*
CPlatformEWarnManage::GetStkConditionRecord(const size_t serial, const size_t CurrentMax, const char* stkCode)
{
	CEarlyWarningConditionRecord *pHeadNode = GetIndexNode(serial);
	if (NULL == pHeadNode)
	{
		return NULL;
	}

	for (size_t i = 0; i < CurrentMax; i++)
	{
		if (WARNINGSET_DELDB != pHeadNode[i].GetOperToDb()
			&& 0 == StrNoCaseCmp(stkCode, pHeadNode[i].GetStkCode(), MAX_STKCODE_LEN))
		{
			return &pHeadNode[i];
		}
	}

	return NULL;
}

// 清空一组记录
void
CPlatformEWarnManage::ClearGroupRecord(const size_t serial)
{
	CEarlyWarningConditionRecord *pHeadNode = GetIndexNode(serial);
	if (NULL == pHeadNode)
	{
		return;
	}

	// 设置预警记录组占用标志
	pHeadNode->UpdateSetEGroupUseFlag(0);
	// 设置预警记录数
	pHeadNode->UpdateSetEWarnCount(0);
}


