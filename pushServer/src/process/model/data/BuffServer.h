#ifndef _INCLUDE_BUFFER_SERVER_H
#define _INCLUDE_BUFFER_SERVER_H

#include "../../../util/util.h"

class CBuffServer
{
public:
	CBuffServer();
	~CBuffServer(void);
};

// 大缓冲区
class LargeMemoryCache
{
private:
	// 初始内存大小
	const int mInitialLen;
	// 内存区域
	char* mDataBuf;
	// 当前数据位置
	int mCurrentPos;
	// 最新一次的警告信息
	int m_iWarning;
	// 安全预留
	const int mSafeRemainLen;

private:
	bool Initial();

public:
	LargeMemoryCache(const int InitialLen);
	virtual ~LargeMemoryCache();
	
	void * GetRawMemPointer(const int StartPos);
	void ClearAll();
	// 清空一段内存
	void ClearRange(const int startPos, const int length);
	// 移动一段内存的内容到开始位置
	bool MoveRangeMemToStart(const int SrcStartPos, const int length);
	// 获取固定大小的缓冲区
	char* GetAllocMem(const int length);
	// 获取全部剩余缓冲区
	char* GetRemainMem(int &RemainLength);
	// 设置偏移量
	void SetRemainMemStart(const int offset);
	// 获取指定位置到结尾的缓存
	char* GetPosData(const int StartPos, int &length);

	// 获取数据长度
	const int GetUsedMemoryLen()const
	{
		return mCurrentPos;
	}

	// 清空剩余的数据部分
	void ClearRemainMem(const int);
	
	// 清空使用的缓存
	void ClearUsedMem();

	// 获取最新一次的warning
	const int GetLatestWarning()const
	{
		return m_iWarning;
	}
	
	// 获取数据区大小
	const int GetRawMemLength()const
	{
		return mInitialLen;
	}

};

// 数据读取
class CBuffReader 
{
private:
	// 数据缓存
	void *m_pBuf;
	// 输出初始位置
	size_t m_iStartPos;
	// 缓存全部输出空间
	size_t m_iTotalDataBufSize;
	// 数据当前位置
	size_t m_iCurrentPos;
	// 输出结束时的位置
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

	// 重置
	void Initial(void *buf, const int BufLen, const int StartPos);

	// 置位到开始
	void SeekToBegin()
	{
		m_iCurrentPos = m_iStartPos;
	}

	// 置位到末尾
	void SeekToEnd()
	{
		m_iCurrentPos = m_iEndPos;
	}
	
	// 移动位置
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

	// 获取当前位置
	const size_t GetCurrentPos()const
	{
		return m_iCurrentPos;
	}
	
	// 获取剩余未读数据长度
	const size_t GetUnReadLen()const
	{
		return m_iTotalDataBufSize - m_iCurrentPos;
	}

	// 是否到末尾
	bool IsEnd()const
	{
		return m_iCurrentPos == m_iEndPos;
	}
	
	// 获取从开始位置的数据
	char* GetCurrentStartData()
	{
		return ((char*)m_pBuf + m_iCurrentPos);
	}

	// 读取指定类型指针
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
	
	// 读取short
	short ReadShort();
	// 读取string
	char* ReadString(const int sLen, char*, const int);
	// 读取float
	float ReadFloat();
	// 读取byte
	unsigned char ReadByte();
	// 读取int
	int ReadInt();
	// 读取uint
	unsigned int ReadUInt();
};

// 数据写入
class CBuffWriter
{
private:
	// 数据缓存
	void *m_pBuf;
	// 输出初始位置
	size_t m_iStartPos;
	// 缓存全部输出空间
	size_t m_iTotalDataBufSize;
	// 数据当前位置
	size_t m_iCurrentPos;
	// 新输出的数据的长度
	size_t m_iDataLength;
	// 输出结束时的位置
	size_t m_iEndPos;
	// 状态
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
	// 写入，并改变游标位置
	bool Push_back(const void *data, const size_t len);
	// 写入，不改变位置信息
	bool Push_set(const void *data, const size_t len);
	// 置位
	bool SeekPos(const size_t pos);
	// 置位到开始
	void SeekToBegin()
	{
		m_iCurrentPos = m_iStartPos;
	}
	// 置位到末尾
	void SeekToEnd()
	{
		m_iCurrentPos = m_iEndPos;
	}
	// 获取写入数据长度
	size_t GetNewdataLen() const
	{
		return m_iDataLength;
	}
	// 是否已经满
	bool IsFull()const
	{
		if (m_iCurrentPos >= m_iTotalDataBufSize 
			|| 0 != m_uStatus)
		{
			return true;
		}
		
		return false;
	}
	// 是否能够容纳
	bool CanHold(const unsigned int Len)
	{
		return (m_iTotalDataBufSize - m_iCurrentPos) > Len;
	}
};

#endif		/* _INCLUDE_BUFFER_SERVER_H */
