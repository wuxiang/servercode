#ifndef _INCLUDE_BUFFER_SERVER_H
#define _INCLUDE_BUFFER_SERVER_H

#include "../../../util/util.h"

class CBuffServer
{
public:
	CBuffServer();
	~CBuffServer(void);
};

// �󻺳���
class LargeMemoryCache
{
private:
	// ��ʼ�ڴ��С
	const int mInitialLen;
	// �ڴ�����
	char* mDataBuf;
	// ��ǰ����λ��
	int mCurrentPos;
	// ����һ�εľ�����Ϣ
	int m_iWarning;
	// ��ȫԤ��
	const int mSafeRemainLen;

private:
	bool Initial();

public:
	LargeMemoryCache(const int InitialLen);
	virtual ~LargeMemoryCache();
	
	void * GetRawMemPointer(const int StartPos);
	void ClearAll();
	// ���һ���ڴ�
	void ClearRange(const int startPos, const int length);
	// �ƶ�һ���ڴ�����ݵ���ʼλ��
	bool MoveRangeMemToStart(const int SrcStartPos, const int length);
	// ��ȡ�̶���С�Ļ�����
	char* GetAllocMem(const int length);
	// ��ȡȫ��ʣ�໺����
	char* GetRemainMem(int &RemainLength);
	// ����ƫ����
	void SetRemainMemStart(const int offset);
	// ��ȡָ��λ�õ���β�Ļ���
	char* GetPosData(const int StartPos, int &length);

	// ��ȡ���ݳ���
	const int GetUsedMemoryLen()const
	{
		return mCurrentPos;
	}

	// ���ʣ������ݲ���
	void ClearRemainMem(const int);
	
	// ���ʹ�õĻ���
	void ClearUsedMem();

	// ��ȡ����һ�ε�warning
	const int GetLatestWarning()const
	{
		return m_iWarning;
	}
	
	// ��ȡ��������С
	const int GetRawMemLength()const
	{
		return mInitialLen;
	}

};

// ���ݶ�ȡ
class CBuffReader 
{
private:
	// ���ݻ���
	void *m_pBuf;
	// �����ʼλ��
	size_t m_iStartPos;
	// ����ȫ������ռ�
	size_t m_iTotalDataBufSize;
	// ���ݵ�ǰλ��
	size_t m_iCurrentPos;
	// �������ʱ��λ��
	size_t m_iEndPos;

public:
	CBuffReader()
	{
	}
	
	CBuffReader(void *buf, const int BufLen, const int StartPos)
		: m_pBuf(buf),
		m_iStartPos(StartPos),
		m_iTotalDataBufSize(BufLen),
		m_iCurrentPos(m_iStartPos),
		m_iEndPos(BufLen)
	{
	}

	// ����
	void Initial(void *buf, const int BufLen, const int StartPos);

	// ��λ����ʼ
	void SeekToBegin()
	{
		m_iCurrentPos = m_iStartPos;
	}

	// ��λ��ĩβ
	void SeekToEnd()
	{
		m_iCurrentPos = m_iEndPos;
	}
	
	// �ƶ�λ��
	bool SeekPos(const int step)
	{
		int iPos = m_iCurrentPos;
		iPos += step;
		if (iPos < 0 || iPos > (int)m_iTotalDataBufSize)
		{
			return false;
		}
		
		m_iCurrentPos += step;
		return true;
	}

	// ��ȡ��ǰλ��
	const size_t GetCurrentPos()const
	{
		return m_iCurrentPos;
	}
	
	// ��ȡʣ��δ�����ݳ���
	const size_t GetUnReadLen()const
	{
		return m_iTotalDataBufSize - m_iCurrentPos;
	}

	// �Ƿ�ĩβ
	bool IsEnd()const
	{
		return m_iCurrentPos == m_iEndPos;
	}
	
	// ��ȡ�ӿ�ʼλ�õ�����
	char* GetCurrentStartData()
	{
		return ((char*)m_pBuf + m_iCurrentPos);
	}

	// ��ȡָ������ָ��
	template<typename T>
	T* ReadPoiner()
	{
		int iSizeLen = sizeof(T);
		T* OutRes = (T*)NULL;
		if (NULL == m_pBuf || m_iTotalDataBufSize <= 0 || IsEnd()
			|| (m_iCurrentPos + iSizeLen) > m_iTotalDataBufSize)
		{
			return OutRes;
		}
	
		OutRes = (T*)((char*)m_pBuf + m_iCurrentPos);
		if (NULL != OutRes) 
		{
			m_iCurrentPos += iSizeLen;
		}
	
		return OutRes;
	}
	
	// ��ȡshort
	short ReadShort();
	// ��ȡstring
	char* ReadString(const int sLen, char*, const int);
	// ��ȡfloat
	float ReadFloat();
	// ��ȡbyte
	unsigned char ReadByte();
	// ��ȡint
	int ReadInt();
	// ��ȡuint
	unsigned int ReadUInt();
};

// ����д��
class CBuffWriter
{
private:
	// ���ݻ���
	void *m_pBuf;
	// �����ʼλ��
	size_t m_iStartPos;
	// ����ȫ������ռ�
	size_t m_iTotalDataBufSize;
	// ���ݵ�ǰλ��
	size_t m_iCurrentPos;
	// ����������ݵĳ���
	size_t m_iDataLength;
	// �������ʱ��λ��
	size_t m_iEndPos;
	// ״̬
	unsigned char m_uStatus;

public:
	CBuffWriter()
		: m_uStatus(0)
	{
	}
	
	CBuffWriter(void *buf, const size_t BufSize, const size_t StartPos)
		: m_pBuf(buf),
		m_iStartPos(StartPos),
		m_iTotalDataBufSize(BufSize),
		m_iCurrentPos(m_iStartPos),
		m_iDataLength(0),
		m_iEndPos(StartPos),
		m_uStatus(0)
	{
	}

	void Initial(void *buf, const size_t BufSize, const size_t StartPos);
	// д�룬���ı��α�λ��
	bool Push_back(const void *data, const size_t len);
	// д�룬���ı�λ����Ϣ
	bool Push_set(const void *data, const size_t len);
	// ��λ
	bool SeekPos(const size_t pos);
	// ��λ����ʼ
	void SeekToBegin()
	{
		m_iCurrentPos = m_iStartPos;
	}
	// ��λ��ĩβ
	void SeekToEnd()
	{
		m_iCurrentPos = m_iEndPos;
	}
	// ��ȡд�����ݳ���
	size_t GetNewdataLen() const
	{
		return m_iDataLength;
	}
	// �Ƿ��Ѿ���
	bool IsFull()const
	{
		if (m_iCurrentPos >= m_iTotalDataBufSize 
			|| 0 != m_uStatus)
		{
			return true;
		}
		
		return false;
	}
	// �Ƿ��ܹ�����
	bool CanHold(const unsigned int Len)
	{
		return (m_iTotalDataBufSize - m_iCurrentPos) > Len;
	}
};

#endif		/* _INCLUDE_BUFFER_SERVER_H */
