#include "PushMsgHistory.h"
#include "../../../util/log/log.h"
#include "../../../util/common/common_file.h"
#include "../../../util/common/common_lib.h"
#include "../../../util/time/time_util.h"
#include "../../../controller/runtime.h"
#include "../../config.h"
#include "../../data_type.h"
#include <sys/mman.h>

CPushMsgHistory::CPushMsgHistory(const size_t RecordLen, const size_t MaxRemainEWarningCount, const size_t MaxRemainInfomineCount,
		const size_t MaxRemainNoticeCount, const size_t MaxSegNum, const char *FileName)
	: m_iRecordLen(RecordLen),
	m_iMaxRemainEWarningCount(MaxRemainEWarningCount),
	m_iMaxRemainInfomineCount(MaxRemainInfomineCount),
	m_iMaxRemainNoticeCount(MaxRemainNoticeCount),
	m_iMaxSegNum(MaxSegNum),
	m_pMemAddress(MAP_FAILED),
	m_iMapFileSize(0),
	m_iLatestStatus(-1),
	m_uPerSegOffset(0)
{
	strncpy(m_strFileName, FileName, MAX_PATH_LEN);
}

CPushMsgHistory::~CPushMsgHistory()
{
}

// �����ڴ�ӳ��
int
CPushMsgHistory::CreateMemmap()
{
	int iFd = -1;
	
	// �趨��ÿ���û����Ա����������ʷ��¼��
	size_t iHisNumPerUser = m_iMaxRemainEWarningCount + m_iMaxRemainInfomineCount + m_iMaxRemainNoticeCount;
	// �ֶ�ƫ����
	m_uPerSegOffset = iHisNumPerUser * m_iRecordLen;
	// ���������ļ���С
	m_iMapFileSize = (off_t)(m_uPerSegOffset * m_iMaxSegNum);
			
	if (!IsFileExist(m_strFileName))
	{
		iFd = open(m_strFileName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if (iFd < 0)
		{
			m_iLatestStatus = -1;
			return m_iLatestStatus;
		}

		// �����ļ���С
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
			// �����ļ���С
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
	
	// ��ӳ��
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

// ��ʼ����ʷ��¼����
int
CPushMsgHistory::InitialLoadMsgHistory()
{
	if (0 != CreateMemmap())
	{
		ERROR("CreateMemmap error [%d]", m_iLatestStatus);
		return 0;
	}
	
	return 1;
}

// �ͷŷ������Դ
void 
CPushMsgHistory::ReleaseRes()
{
	// �ͷ�ӳ���ַ
	if (0 == m_iLatestStatus && MAP_FAILED != m_pMemAddress)
	{
		munmap(m_pMemAddress, m_iMapFileSize);
		m_pMemAddress = MAP_FAILED;
	}
}

// ��ȡѭ��������������
size_t 
CPushMsgHistory::GetCirculateIndex(const size_t index, const size_t MaxNode)
{
	if (index < MaxNode)
	{
		return index;
	}

	return index % MaxNode;
}

// ��ȡ��Ϣ��¼���׵�ַ
PushMsg* 
CPushMsgHistory::GetPushMsgHead(const size_t serial, const int MsgType,
	size_t &CurrentTypeSegCount)
{
	if (serial >= m_iMaxSegNum || 0 != m_iLatestStatus)
	{
		return NULL;
	}
	
	size_t uLocalOffset = 0;
	switch(MsgType)
	{
		case PTE_EWARNING:
			uLocalOffset = 0;
			CurrentTypeSegCount = m_iMaxRemainEWarningCount;
			break;
			
		case PTE_INFOMINE:
			uLocalOffset = m_iMaxRemainEWarningCount * m_iRecordLen;
			CurrentTypeSegCount = m_iMaxRemainInfomineCount;
			break;
			
		case PTE_NOTICE:
			uLocalOffset = (m_iMaxRemainEWarningCount + m_iMaxRemainInfomineCount) * m_iRecordLen;
			CurrentTypeSegCount = m_iMaxRemainNoticeCount;
			break;
			
		default:
			CurrentTypeSegCount = 0;
			return NULL;
	}
	
	return (PushMsg*)(((char*)m_pMemAddress) + m_uPerSegOffset * serial + uLocalOffset);
}

// �����ʷ��¼
const int
CPushMsgHistory::AddMsgHistory(const size_t serial, const size_t MsgIndex, 
	const size_t HaveSendMark, const PushMsg &msg)
{
	size_t CurrentTypeSegCount = 0;
	pPushMsg PushHisMsgHead = GetPushMsgHead(serial, msg.MsgType, CurrentTypeSegCount);
	if (NULL == PushHisMsgHead )
	{
		DEBUG("NULL == PushHisMsgHead");
		return -1;
	}
	
	// ��ȡ��������
	size_t uOperIndex = GetCirculateIndex(MsgIndex, CurrentTypeSegCount);
		
	PushHisMsgHead[uOperIndex] = msg;

	return 0;
}

// ��ȡָ������ָ��ָ�����͵���Ϣ
const int 
CPushMsgHistory::GetPushMsg(const size_t serial, const int MsgType, 
	const size_t index, pPushMsg &pMsg)
{
	size_t CurrentTypeSegCount = 0;
	pPushMsg PushHisMsgHead = GetPushMsgHead(serial, MsgType, CurrentTypeSegCount);
	
	if (NULL == PushHisMsgHead)
	{
		return -1;
	}
	
	size_t uOperIndex = GetCirculateIndex(index, CurrentTypeSegCount);
	// У��
	if ((unsigned char)-1 == PushHisMsgHead[uOperIndex].MsgType)
	{
		return -2;
	}

	pMsg = &PushHisMsgHead[uOperIndex];
	return 0;
}

// ��սڵ����ʷ��¼��Ϣ
bool
CPushMsgHistory::ClearNodeMsgHistory(const size_t serial)
{
	size_t CurrentTypeSegCount = 0;
	pPushMsg PushHisMsgHead = GetPushMsgHead(serial, PTE_EWARNING, CurrentTypeSegCount);
	for (size_t i = 0; i < CurrentTypeSegCount; i++)
	{
		PushHisMsgHead[i].Clear();
	}
	
	PushHisMsgHead = GetPushMsgHead(serial, PTE_INFOMINE, CurrentTypeSegCount);
	for (size_t i = 0; i < CurrentTypeSegCount; i++)
	{
		PushHisMsgHead[i].Clear();
	}
	
	PushHisMsgHead = GetPushMsgHead(serial, PTE_NOTICE, CurrentTypeSegCount);
	for (size_t i = 0; i < CurrentTypeSegCount; i++)
	{
		PushHisMsgHead[i].Clear();
	}
	
	return true;
}

// ��ȡ��Ϣ����Ч��
time_t 
CPushMsgHistory::GetHisMsgValidTime(const int MsgType)
{
	time_t tValidTime = 0;
	switch(MsgType)
	{
		case PTE_EWARNING:
			tValidTime = GetEWarningRuntime()->RemainValidTime;
			break;
			
		case PTE_INFOMINE:
			tValidTime = GetInfoMineRuntime()->RemainValidTime;
			break;
			
		case PTE_NOTICE:
			tValidTime = GetNoticeRuntime()->RemainValidTime;
			break;
			
		default:
			break;
	}
	
	return (tValidTime * 24 * 60 * 60);
}

// ��Ϣ�Ƿ��Ѿ�����
bool
CPushMsgHistory::IsMsgOutofDate(pPushMsg pPushMsgView, const time_t tv)
{
	if (NULL == pPushMsgView)
	{
		return true;
	}
	
	time_t uValidTime = GetHisMsgValidTime(pPushMsgView->MsgType);
	if ((int)(tv - pPushMsgView->DateTime) >= (int)uValidTime)
	{
		return true;
	}
	
	return false;
}

// ��ȡ�����ʷ��¼��
const size_t 
CPushMsgHistory::GetMaxMsgCacheCount(const int MsgType)
{
	size_t iRes = 0;
	switch(MsgType)
	{
		case PTE_EWARNING:
			iRes = m_iMaxRemainEWarningCount;
			break;
			
		case PTE_INFOMINE:
			iRes = m_iMaxRemainInfomineCount;
			break;
			
		case PTE_NOTICE:
			iRes = m_iMaxRemainNoticeCount;
			break;
			
		default:
			break;
	}
	
	return iRes;
}

