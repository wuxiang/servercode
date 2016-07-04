/*****
** ���ͷ��ʹ����߳�
** ���Դ���������Ϣ�ķ���
********************************/
#ifndef _REQ_DYNA_NEWS_THREAD_H
#define _REQ_DYNA_NEWS_THREAD_H

#include "ThreadImp.h"

class ReqDynaNewsThread : public ThreadImp {
public:
	ReqDynaNewsThread();
	~ReqDynaNewsThread();

protected:
	bool InitialThreadEnv();									// ��ʼ�̻߳���
	int StartThread();											// �����߳�
	static void* ThreadProc(void*);								// �̴߳�����
	
private:
	time_t m_ExecuteUpdatePublicNewsTimer;						// ���һ��ִ��ȡ�������Ŷ�̬����
	time_t m_ExecuteUpdateInfoMineTimer;						// ���һ��ִ��ȡ��Ϣ��������
	time_t m_ScanUserTimer;										// ���һ��ִ��ɨ���û�����ʱ��
	time_t m_JudgeInfoMineStatisTimer;							// �ж�ִ����Ϣ����ͳ��
	bool m_bInfoMineInitial;									// ��Ϣ�����Ƿ��ʼ���ɹ�
	bool m_bNoticeInitial;										// �����Ƿ��ʼ�ɹ�
	bool m_bProcessUserScan;									// �Ƿ�ִ����ProcessUserScan
	
private:
	int DetectInfoMineStatis();
	void NotifyExeInfoMineStatisCalc(bool);						// ִ֪ͨ����Ϣ����ͳ��
	
private:
	int ExecuteInitial();										// ִ�г�ʼ��
	int ExecuteInfoMineInitial();								// ִ�е��׳�ʼ��
	int ExecutePublicNewsScan();								// ִ��ɨ�蹫����Ϣ
	int ExecuteInfoMineScan();									// ִ��ɨ����Ϣ����
	void NotifyExeInfoMineCalc(bool);							// ִ֪ͨ����Ϣ����
	void NotifyExeNoticeCalc(bool);								// ִ֪ͨ�й���ɨ��
	int ProcessUserScan();										// ִ��ɨ���û�
	int ExecuteInfoMineStatis();								// ִ����Ϣ����ͳ��
	int ProcedureMsgQueue();									// ������Ϣ����
	
};


#endif  /* _REQ_DYNA_NEWS_THREAD_H */
