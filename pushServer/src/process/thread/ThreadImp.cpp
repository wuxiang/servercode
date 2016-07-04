#include "ThreadImp.h"
#include "../../util/time/time_util.h"
#include "../../util/common/common_lib.h"
#include "../config.h"
#include "../../controller/runtime.h"

ThreadImp::ThreadImp() 
	: m_tThreadID(0),
	m_ThreadState(Thread_Waiting),
	m_bHasInitialed(false),
	m_iInitialDate(0),
	m_iSignalProperty(0)
{
	bzero(m_strKey, MaxThreadKeyLen + 1);
}


ThreadImp::~ThreadImp() {
}

const pthread_t 
ThreadImp::GetThreadID()const {
	return m_tThreadID;
}

// 获取线程运行状态
enum ThreadRunState 
ThreadImp::GetThreadRunState()const {
	return m_ThreadState;
}

// 设置线程运行状态
void 
ThreadImp::SetThreadRunState(enum ThreadRunState state) {
	m_ThreadState = state;
}

// 设置线程标识
void 
ThreadImp::SetThreadID(const pthread_t id) {
	m_tThreadID = id;
}

// 退出线程
int 
ThreadImp::ExitThread(int ExitCode) 
{
	SetThreadRunState(Thread_Terminate);
	return ExitCode;
}

// 初始线程环境	
bool 
ThreadImp::InitialThreadEnv()
{
	m_JudgeInitialTimer = GetNowTime();
	return true;
}

// 是否需要重新初始化线程
bool
ThreadImp::CanResetNow()
{
	time_t NowTime = GetNowTime();
	if (DiffTimeSecond(NowTime, m_JudgeInitialTimer) < JUDGE_INITIAL_INTERVAL) {		
		return false;
	}
	m_JudgeInitialTimer = NowTime;
	
	// 当前时间
	int iTime = GetIntTime(&NowTime);
	
	// 日期
	int iDate = GetIntDate(&NowTime);
			
	if (m_bHasInitialed)
	{
		if (iDate > m_iInitialDate)
		{
			m_bHasInitialed = false;
		}
		return false;
	}

	if (iTime < GetInitialStartTime() || iTime > GetInitialEndTime())
	{
		return false;
	}
	
	m_iInitialDate = iDate;
	return true;
}

// 执行初始化
int 
ThreadImp::ExecuteInitial()
{
	return -1;
}

// 设置信号量
void
ThreadImp::SetSignal(const int bit, bool SetOrUnset)
{
	if (SetOrUnset)
	{
		BitSet(m_iSignalProperty, bit);
	}
	else
	{
		BitUnSet(m_iSignalProperty, bit);
	}
}

// 获取信号量
bool
ThreadImp::GetSignal(const int bit)
{
	return IsBitSet(m_iSignalProperty, bit);
}

// 设置线程字符串标识
void
ThreadImp::SetThreadKey(const char *key)
{
	if (NULL != key)
	{
		strncpy(m_strKey, key, MaxThreadKeyLen);
	}
}

// 获取线程字符串标识
const char*
ThreadImp::GetThreadKey()
{
	return m_strKey;
}

// 是否挂起请求
bool
ThreadImp::IsHangupRequest()
{
	return GetSignal(NSP_HANGUP_REQUEST);
}

