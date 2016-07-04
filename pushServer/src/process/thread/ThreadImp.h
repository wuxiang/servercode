#ifndef _IMP_THREAD_H
#define _IMP_THREAD_H

#include "../data_type.h"
#include "../../util/util.h"
#include "ThreadMsgProcedure.h"
#include <pthread.h>

enum NetSignalPropertyEnum
{
	// ����ͻ�����
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
	pthread_t m_tThreadID;									// �̱߳�ʶID
	char m_strKey[MaxThreadKeyLen + 1];						// �̹߳ؼ���
	enum ThreadRunState m_ThreadState;						// �̵߳�����״̬
	bool m_bHasInitialed;									// �Ƿ��Ѿ���ʼ��
	int m_iInitialDate;										// ��ʼ������
	time_t m_JudgeInitialTimer;								// ���һ��ִ���жϳ�ʼ��
	unsigned int m_iSignalProperty;							// ���յ����ź���(NetSignalPropertyEnum ��չ������Ҫ��NSP_RESERVE֮��)

protected:
	void SetThreadRunState(enum ThreadRunState);			// �����߳�����״̬
	void SetThreadID(const pthread_t);						// �����̱߳�ʶ
	bool CanResetNow();										// �Ƿ���Ҫ���³�ʼ���߳�
	void SetSignal(const int, bool);						// �����ź���
	bool GetSignal(const int);								// ��ȡ�ź���
	void SetThreadKey(const char*);							// �����߳��ַ�����ʶ
	virtual int ExecuteInitial();							// ִ�г�ʼ��
	
public:	
	virtual int ExitThread(int);							// �˳��߳�
	virtual bool InitialThreadEnv();						// ��ʼ�̻߳���	
	virtual int StartThread() = 0;							// �����߳�
	const pthread_t GetThreadID()const;						// ��ȡ�̱߳�ʶID
	enum ThreadRunState GetThreadRunState()const;			// ��ȡ�߳�����״̬
	bool IsHangupRequest();									// �Ƿ��������
	const char* GetThreadKey();								// ��ȡ�߳��ַ�����ʶ
};

typedef ThreadImp* ThreadImpPtr;							// ָ��

#endif  /* _IMP_THREAD_H */

