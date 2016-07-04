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

// �ļ��ṹͷ����
struct SelfSelectRecHead
{
private:
	// ��¼����
	unsigned char MemCount;
	// ����λ���� (0:�Ƿ���ձ�ʶ 1:��¼�Ƿ��и��� 23456789:���һ�η��͵�ͳ�Ƹ���ֵ 10/11/12:�Ѿ�����ͳ����Ϣ���׵Ĵ��� 13:�Ƿ��յ��Ѷ�ȡ��Ӧ)
	unsigned short Property;
	// ������͵���Ϣ����ID
	unsigned int LatestInfoMineCrc;
	// ͳ�Ƶ���Ϣ��������
	unsigned char InfoMineTotalCount;
	
public:
	// ����ռ�ü�¼������
	void SetMemCount(const unsigned char Count)
	{
		MemCount = Count;
	}
	
	// ��ȡռ�õļ�¼������
	const unsigned char GetMemCount()
	{
		return MemCount;
	}
	
	// ���ý��ձ�ʶ
	void SetRecvMark(bool Accept)
	{
		Accept ? BitSet(Property, 0) : BitUnSet(Property, 0);
	}
	
	// �Ƿ������˽��ձ�ʶ
	bool HaveSetRecvMark()
	{
		return IsBitSet(Property, 0);
	}
	
	// ���ñ����ʶ
	void SetModifiedMark(bool modified)
	{
		modified ? BitSet(Property, 1) : BitUnSet(Property, 1);
	}
	
	// ��¼�Ƿ���ڱ��
	bool HaveModifiedMark()
	{
		return IsBitSet(Property, 1);
	}
	
	// �������һ�η���ͳ����Ϣ�Ĵ���
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
	
	// ��ȡ���һ�η���ͳ����Ϣ�Ĵ���
	const unsigned char GetStaticsCount()
	{
		unsigned char uCurrentValue = Property & 0x3FC;
		uCurrentValue >>= 2;
		return uCurrentValue;
	}
	
	// �����Ѿ�����ͳ����Ϣ���׵Ĵ���(������ڷ���ͳ�Ƶ���������)
	void SetHaveSendTimes(const unsigned short Count)
	{
		unsigned short uCurrentValue = Property & 0xE3FF;
		Property = uCurrentValue | ((Count << 10) & 0x1C00);
	}
	
	// ��ȡ�Ѿ�����ͳ����Ϣ���׵Ĵ���
	const unsigned char GetHaveSendTimes()
	{
		unsigned short uCurrentValue = Property & 0x1C00;
		uCurrentValue >>= 10;
		return uCurrentValue;
	}
	
	// �����Ƿ��յ��Ѷ�ȡ��Ӧ
	void SetHaveReadMark(bool Accept)
	{
		Accept ? BitSet(Property, 13) : BitUnSet(Property, 13);
	}
	
	// �Ƿ��յ��Ѷ�ȡ��Ӧ
	bool HaveReadMark()
	{
		return IsBitSet(Property, 13);
	}
	
	// ������Ϣ��������
	void SetInfomineTotalCount(const short Count)
	{
		InfoMineTotalCount = Count;
	}
	
	// ��ȡ��Ϣ��������
	const unsigned char GetInfomineTotalCount()
	{
		return InfoMineTotalCount;
	}

	// �������
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
	
	// ����������͵���Ϣ����ID
	void SetLatestInfoMineCrc(const unsigned int Crc)
	{
		LatestInfoMineCrc = Crc;
	}
	
	// ��ȡ������͵���Ϣ����ID
	const unsigned int GetLatestInfoMineCrc()
	{
		return LatestInfoMineCrc;
	}
};

// �ļ����嶨��
struct SelfSelectRecBody
{
	// ����
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

// �����ڴ�ӳ��
int
CSelfSelectStock::CreateMemmap()
{
	int iFd = -1;
	
	// �ֶ�ƫ����
	m_uPerSegOffset = m_iHeadLen + m_uMaxRecPerUser * m_iRecordLen + 1;
	// ���������ļ���С
	m_iMapFileSize = (off_t)(m_uPerSegOffset * m_uMaxUser);
			
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
					ERROR("truncate errno=%d", errno);
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
		ERROR("mmap errno=%d %s  m_strFileName=%s", errno, strerror(errno), m_strFileName);
		m_iLatestStatus = -8;
		return m_iLatestStatus;
	}

	CloseFile(&iFd);
	m_iLatestStatus = 0;
	return m_iLatestStatus;
}

// ��ʼ�����ݶ���
int
CSelfSelectStock::InitialLoad()
{
	// ��ʼ����¼����
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

// �ͷŷ������Դ
void
CSelfSelectStock::ReleaseRes()
{
	// �ͷ�ӳ���ַ
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

// ��ȡָ��������ͷ
SelfSelectRecHead*
CSelfSelectStock::GetIndexHead(const size_t NodeIndex)
{
	if (NodeIndex >= m_uMaxUser || 0 != m_iLatestStatus)
	{
		return NULL;
	}
	
	return (SelfSelectRecHead*)(((char*)m_pMemAddress) + m_uPerSegOffset * NodeIndex);
}

// ��ȡ��¼����
void*
CSelfSelectStock::GetIndexBody(const size_t NodeIndex)
{
	if (NodeIndex >= m_uMaxUser || 0 != m_iLatestStatus)
	{
		return NULL;
	}
	return (SelfSelectRecHead*)(((char*)m_pMemAddress) + m_uPerSegOffset * NodeIndex + m_iHeadLen);
}

// ��Ӵ���
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
			// ѡ��Ĺ�Ʊ��֧��
			return -37;
		}
		// ���ڳ�ʼ��
		return -22;
	}
	
	// ����ָ������
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

// ɾ������
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

// ��մ���
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

// ��ȡָ�������Ĵ���
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

// ��ȡָ���ڵ�ļ�¼����
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

// ����ָ���ڵ�ļ�¼����
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

// �Ƿ������˽�����Ϣ���ױ��
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

// ���ý��ձ�ʶ
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

// ���ñ����ʶ
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

// ��¼�Ƿ���ڱ��
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

// �������һ�η���ͳ����Ϣ�Ĵ���
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

// ��ȡ���һ�η���ͳ����Ϣ�Ĵ���
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

// �����Ѿ�����ͳ����Ϣ���׵Ĵ���(������ڷ���ͳ�Ƶ���������)
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

// ��ȡ�Ѿ�����ͳ����Ϣ���׵Ĵ���
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

// �����Ƿ��յ��Ѷ�ȡ��Ӧ
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

// �Ƿ��յ��Ѷ�ȡ��Ӧ
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

// ����
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

// ��ȡȫ�����봮
const char*
CSelfSelectStock::GetTotalSelfStkCode(const size_t NodeIndex)
{
	return (const char*)GetIndexBody(NodeIndex);
}

// ���ô��봮
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

// ����������͵���Ϣ����ID
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

// ��ȡ������͵���Ϣ����ID
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

// ��ȡ֤ȯ�����������е�����
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

// ������Ϣ��������
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

// ��ȡ��Ϣ��������
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

