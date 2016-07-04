#include "BuffServer.h"

CBuffServer::CBuffServer()
{
	
}

CBuffServer::~CBuffServer(void)
{
	
}

LargeMemoryCache::LargeMemoryCache(const int InitialLen)
	: mInitialLen(InitialLen),
	mDataBuf(NULL),
	mCurrentPos(0),
	m_iWarning(0),
	mSafeRemainLen(128)
{
	Initial();
}

LargeMemoryCache::~LargeMemoryCache()
{
	if (NULL != mDataBuf)
	{
		delete []mDataBuf;
		mDataBuf = NULL;
	}
}

bool 
LargeMemoryCache::Initial()
{
	if (mInitialLen <= 0)
	{
		return false;
	}
	if (NULL == mDataBuf)
	{
		mDataBuf = new char[mInitialLen];
		memset(mDataBuf, 0, mInitialLen);
	}
	return true;
}

void * 
LargeMemoryCache::GetRawMemPointer(const int StartPos)
{
	if (StartPos >= mInitialLen || StartPos <= (mInitialLen * -1))
	{
		return NULL;
	}
	return (mDataBuf + StartPos);
}

// ��ȡָ��λ�õ���β�Ļ���
char*
LargeMemoryCache::GetPosData(const int StartPos, int &length)
{
	length = 0;
	if (StartPos < 0 || StartPos > mCurrentPos)
	{
		return NULL;
	}
	
	char *pGot = (char*)GetRawMemPointer(StartPos);
	if (NULL != pGot)
	{
		length = mCurrentPos - StartPos;
	}
	return pGot;
}

void 
LargeMemoryCache::ClearAll()
{
	if (NULL != mDataBuf)
	{
		memset(mDataBuf, 0, mInitialLen);
	}
	mCurrentPos = 0;
	m_iWarning = 0;
}

void 
LargeMemoryCache::ClearRange(const int startPos, const int length)
{
	if (NULL != mDataBuf && length > 0)
	{
		if (startPos >= 0 && startPos < mInitialLen && (startPos + length) <= mInitialLen)
		{
			memset(mDataBuf + startPos, 0, length);
		}
	}
}

bool 
LargeMemoryCache::MoveRangeMemToStart(const int SrcStartPos, const int length)
{
	if (SrcStartPos > mCurrentPos || length > mCurrentPos)
	{
		return false;
	}
	if (0 == SrcStartPos && length == mCurrentPos)
	{
		return true;
	}
	memcpy(mDataBuf, mDataBuf + SrcStartPos, length);
	mCurrentPos = length;
	if (0 != m_iWarning)
	{
		if (mCurrentPos < (mInitialLen - mSafeRemainLen))
		{
			m_iWarning = 0;
		}
	}
	return true;
}

// ��ȡ�̶���С�Ļ�����
char* 
LargeMemoryCache::GetAllocMem(const int length)
{
	char* outP = NULL;
	if (mCurrentPos >= 0 && mCurrentPos < mInitialLen && (mCurrentPos + length) <= mInitialLen)
	{
		outP = (char*)(mDataBuf + mCurrentPos);
	}
	mCurrentPos += length;
	if (mCurrentPos >= (mInitialLen - mSafeRemainLen))
	{
		m_iWarning = 1;
	}
	return outP;
}

// ��ȡȫ��ʣ�໺����
char* 
LargeMemoryCache::GetRemainMem(int &RemainLength)
{
	char* outP = NULL;
	if (mCurrentPos >= 0 && mCurrentPos < mInitialLen)
	{
		outP = (char*)(mDataBuf + mCurrentPos);
	}

	RemainLength = mInitialLen - mCurrentPos;
	if (mCurrentPos >= (mInitialLen - mSafeRemainLen))
	{
		m_iWarning = 1;
	}
	return outP;
}

// ����ƫ����
void 
LargeMemoryCache::SetRemainMemStart(const int offset)
{
	if (mCurrentPos >= 0 && mCurrentPos < mInitialLen && (mCurrentPos + offset) <= mInitialLen)
	{
		mCurrentPos += offset;
	}

	if (mCurrentPos >= (mInitialLen - mSafeRemainLen))
	{
		m_iWarning = 1;
	}
}

// ���ʣ������ݲ���
void 
LargeMemoryCache::ClearRemainMem(const int ResetLen)
{
	if (NULL == mDataBuf || ResetLen <= 0)
	{
		return;
	}
	
	ClearRange(mCurrentPos, ResetLen);
}

// ���ʹ�õĻ���
void
LargeMemoryCache::ClearUsedMem()
{
	ClearRange(0, GetUsedMemoryLen());
	mCurrentPos = 0;
	m_iWarning = 0;
}

// ����
void 
CBuffReader::Initial(void *buf, const int BufLen, const int StartPos)
{
	m_pBuf = buf;
	m_iStartPos = StartPos;
	m_iTotalDataBufSize = BufLen;
	m_iCurrentPos = m_iStartPos;
	m_iEndPos = BufLen;
}

// ��ȡshort
short
CBuffReader::ReadShort()
{
	short *pShort = ReadPoiner<short>();
	if (NULL == pShort)
	{
		return 0;
	}
	return *pShort;
}

// ��ȡint
int 
CBuffReader::ReadInt()
{
	int *pInt = ReadPoiner<int>();
	if (NULL == pInt)
	{
		return 0;
	}
	return *pInt;
}

// ��ȡuint
unsigned int
CBuffReader::ReadUInt()
{
	unsigned int *pInt = ReadPoiner<unsigned int>();
	if (NULL == pInt)
	{
		return 0;
	}
	return *pInt;
}

// ��ȡstring
char* 
CBuffReader::ReadString(const int sLen, char *OutBuf, const int BufSize)
{	
	if (NULL != OutBuf && sLen <= BufSize)
	{
		memset(OutBuf, 0, BufSize);
		if (0 >= sLen || 0 >= BufSize)
		{
			return NULL;
		}
		
		if (!IsEnd() && (m_iCurrentPos + sLen) <= m_iTotalDataBufSize)
		{
			strncpy(OutBuf, (char*)m_pBuf + m_iCurrentPos, sLen);
			SeekPos(sLen);
			return OutBuf;
		}
	}
	
	return NULL;
}

// ��ȡfloat
float 
CBuffReader::ReadFloat()
{
	float *pFloat = ReadPoiner<float>();
	if (NULL == pFloat)
	{
		return 0.0;
	}
	return *pFloat;
}

// ��ȡbyte
unsigned char 
CBuffReader::ReadByte()
{
	unsigned char *pChar = ReadPoiner<unsigned char>();
	if (NULL == pChar)
	{
		return 0;
	}
	return *pChar;
}

void 
CBuffWriter::Initial(void *buf, const size_t BufSize, const size_t StartPos)
{
	m_uStatus = 0;
	m_pBuf= buf;
	m_iStartPos = StartPos;
	m_iTotalDataBufSize = BufSize;
	m_iCurrentPos = m_iStartPos;
	m_iDataLength = 0;
	m_iEndPos = StartPos;
}

// д�룬���ı��α�λ��
bool 
CBuffWriter::Push_back(const void *data, const size_t len)
{
	if (NULL == m_pBuf || NULL == data || 0 >= len)
	{
		return false;
	}
	
	if (IsFull())
	{
		return false;
	}
	
	if ((m_iCurrentPos + len) > m_iTotalDataBufSize)
	{
		m_uStatus = 1;
		return false;
	}
	
	memcpy((char*)m_pBuf + m_iCurrentPos, data, len);
	m_iCurrentPos += len;
	m_iDataLength += len;
	m_iEndPos += len;
	return true;
}

// д�룬���ı�λ����Ϣ
bool 
CBuffWriter::Push_set(const void *data, const size_t len)
{
	if (NULL == m_pBuf || NULL == data)
	{
		return false;
	}
	
	if (0 != m_uStatus)
	{
		return false;
	}
	
	memcpy((char*)m_pBuf + m_iCurrentPos, data, len);
	m_iDataLength += len;
	return true;
}

// ��λ
bool 
CBuffWriter::SeekPos(const size_t pos)
{
	if (pos >= m_iTotalDataBufSize)
	{
		m_uStatus = 2;
		return false;
	}
	if (pos > m_iCurrentPos)
	{
		m_iEndPos = pos;
	}
	m_iCurrentPos = pos;
	return true;
}

