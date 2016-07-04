#ifndef _IMP_THREAD_H
#define _IMP_THREAD_H

#include "../data_type.h"
#include "../../util/util.h"
#include "ThreadMsgProcedure.h"
#include <pthread.h>

enum NetSignalPropertyEnum
{
	// 挂起客户请求
	NSP_HANGUP_REQUEST = 0,
	
	NSP_RESERVE = 10,
};

const int MaxThreadKeyLen = 32;

class ThreadImp : public CThreadMsgProcedure{
public:
	ThreadImp();
	virtual ~ThreadImp();
	friend class ThreadsManage;
	
protected:
	pthread_t m_tThreadID;									// 线程标识ID
	char m_strKey[MaxThreadKeyLen + 1];						// 线程关键字
	enum ThreadRunState m_ThreadState;						// 线程的运行状态
	bool m_bHasInitialed;									// 是否已经初始化
	int m_iInitialDate;										// 初始化日期
	time_t m_JudgeInitialTimer;								// 最近一次执行判断初始化
	unsigned int m_iSignalProperty;							// 接收到的信号量(NetSignalPropertyEnum 扩展定义需要在NSP_RESERVE之后)

protected:
	void SetThreadRunState(enum ThreadRunState);			// 设置线程运行状态
	void SetThreadID(const pthread_t);						// 设置线程标识
	bool CanResetNow();										// 是否需要重新初始化线程
	void SetSignal(const int, bool);						// 设置信号量
	bool GetSignal(const int);								// 获取信号量
	void SetThreadKey(const char*);							// 设置线程字符串标识
	virtual int ExecuteInitial();							// 执行初始化
	
public:	
	virtual int ExitThread(int);							// 退出线程
	virtual bool InitialThreadEnv();						// 初始线程环境	
	virtual int StartThread() = 0;							// 启动线程
	const pthread_t GetThreadID()const;						// 获取线程标识ID
	enum ThreadRunState GetThreadRunState()const;			// 获取线程运行状态
	bool IsHangupRequest();									// 是否挂起请求
	const char* GetThreadKey();								// 获取线程字符串标识
};

typedef ThreadImp* ThreadImpPtr;							// 指针

#endif  /* _IMP_THREAD_H */

