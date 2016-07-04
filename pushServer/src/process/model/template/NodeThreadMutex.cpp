#include "NodeThreadMutex.h"
#include "ThreadMutex.h"
#include "../../../util/log/log.h"

const unsigned int ZeroJugeValue = 0xFFFFFFFF;

struct ThreadMutexMap
{
	unsigned int m_iLockID;
	CThreadMutex m_muLockObj;
	
public:
	ThreadMutexMap()
	{
		m_iLockID = ZeroJugeValue;
	}
	
	~ThreadMutexMap()
	{
	}
	
	void Reset()
	{
		m_iLockID = ZeroJugeValue;
	}
};

CNodeThreadMutex::CNodeThreadMutex(const int num)
	: m_iInitialNum(num)
{
	m_pThreadMutexList = new ThreadMutexMap[m_iInitialNum];
}

CNodeThreadMutex::~CNodeThreadMutex()
{
	if (NULL != m_pThreadMutexList)
	{
		delete []m_pThreadMutexList;
	}
}

LockResType
CNodeThreadMutex::ConfigLock(const int Serial, const unsigned int ObjID)
{
	if (Serial < 0 || Serial >= m_iInitialNum)
	{
		return LOCK_FAILED;
	}
	
	if (ZeroJugeValue == m_pThreadMutexList[Serial].m_iLockID
		|| ObjID != m_pThreadMutexList[Serial].m_iLockID)
	{
		return LOCK_SHARED;
	}
	
	return LOCK_REQUIRED;
}

bool
CNodeThreadMutex::AskOperateAuthority(const int Serial, const unsigned int ObjID)
{
	if (Serial < 0 || Serial >= m_iInitialNum)
	{
		return false;
	}
	
	if (m_pThreadMutexList[Serial].m_muLockObj.lock())
	{
		DEBUG("Ask Serial=%d ObjID = %d", Serial, ObjID);
		m_pThreadMutexList[Serial].m_iLockID = ObjID;
		return true;
	}
	
	return false;
}

void 
CNodeThreadMutex::CommitAuthority(const int Serial)
{
	if (Serial < 0 || Serial >= m_iInitialNum)
	{
		return;
	}
	
	if (ZeroJugeValue == m_pThreadMutexList[Serial].m_iLockID)
	{
		return;
	}
	
	DEBUG("Commit Serial=%d ObjID = %d", Serial, m_pThreadMutexList[Serial].m_iLockID);
	m_pThreadMutexList[Serial].m_muLockObj.unlock();
	m_pThreadMutexList[Serial].Reset();
}

bool
CNodeThreadMutex::TryAskOperateAuthority(const int Serial, const unsigned int ObjID)
{
	if (Serial < 0 || Serial >= m_iInitialNum)
	{
		return false;
	}
	
	if (m_pThreadMutexList[Serial].m_muLockObj.try_lock())
	{
		DEBUG("TryAsk Serial=%d ObjID = %d", Serial, ObjID);
		m_pThreadMutexList[Serial].m_iLockID = ObjID;
		return true;
	}
	
	return false;
}

const unsigned int
CNodeThreadMutex::GenerateObjID(const unsigned int Index1, const unsigned char Index2)
{
	unsigned int uOut = Index2;
	uOut <<= 28;
	
	uOut |= (Index1 & 0x0FFFFFFF);
	return uOut;
}

