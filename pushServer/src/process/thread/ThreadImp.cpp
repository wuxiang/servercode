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

// ��ȡ�߳�����״̬
enum ThreadRunState 
ThreadImp::GetThreadRunState()const {
	return m_ThreadState;
}

// �����߳�����״̬
void 
ThreadImp::SetThreadRunState(enum ThreadRunState state) {
	m_ThreadState = state;
}

// �����̱߳�ʶ
void 
ThreadImp::SetThreadID(const pthread_t id) {
	m_tThreadID = id;
}

// �˳��߳�
int 
ThreadImp::ExitThread(int ExitCode) 
{
	SetThreadRunState(Thread_Terminate);
	return ExitCode;
}

// ��ʼ�̻߳���	
bool 
ThreadImp::InitialThreadEnv()
{
	m_JudgeInitialTimer = GetNowTime();
	return true;
}

// �Ƿ���Ҫ���³�ʼ���߳�
bool
ThreadImp::CanResetNow()
{
	time_t NowTime = GetNowTime();
	if (DiffTimeSecond(NowTime, m_JudgeInitialTimer) < JUDGE_INITIAL_INTERVAL) {		
		return false;
	}
	m_JudgeInitialTimer = NowTime;
	
	// ��ǰʱ��
	int iTime = GetIntTime(&NowTime);
	
	// ����
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

// ִ�г�ʼ��
int 
ThreadImp::ExecuteInitial()
{
	return -1;
}

// �����ź���
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

// ��ȡ�ź���
bool
ThreadImp::GetSignal(const int bit)
{
	return IsBitSet(m_iSignalProperty, bit);
}

// �����߳��ַ�����ʶ
void
ThreadImp::SetThreadKey(const char *key)
{
	if (NULL != key)
	{
		strncpy(m_strKey, key, MaxThreadKeyLen);
	}
}

// ��ȡ�߳��ַ�����ʶ
const char*
ThreadImp::GetThreadKey()
{
	return m_strKey;
}

// �Ƿ��������
bool
ThreadImp::IsHangupRequest()
{
	return GetSignal(NSP_HANGUP_REQUEST);
}

