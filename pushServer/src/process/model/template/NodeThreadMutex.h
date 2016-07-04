#ifndef _INCLUDE_NODE_THREAD_MUTEX_H
#define _INCLUDE_NODE_THREAD_MUTEX_H


struct ThreadMutexMap;

enum LockResType
{
	// 加锁失败
	LOCK_FAILED = -1,
	// 需要加锁的节点尚未被锁定，可以使用
	LOCK_SHARED,
	// 访问的节点正在被锁定，需要等待
	LOCK_REQUIRED
};

class CNodeThreadMutex
{
public:
	CNodeThreadMutex(const int);
	~CNodeThreadMutex();

private:
	const int m_iInitialNum;
	ThreadMutexMap *m_pThreadMutexList;
	
public:
	LockResType ConfigLock(const int Serial, const unsigned int ObjID);
	bool AskOperateAuthority(const int Serial, const unsigned int ObjID);
    void CommitAuthority(const int Serial);
    bool TryAskOperateAuthority(const int Serial, const unsigned int ObjID);
    const unsigned int GenerateObjID(const unsigned int, const unsigned char);
};

#endif		/* _INCLUDE_NODE_THREAD_MUTEX_H */
