/*****
** ��̬���ݸ��´����߳�
** ���Դ���̬���ݸ���
********************************/
#ifndef _UPD_DYNA_DATA_THREAD_H
#define _UPD_DYNA_DATA_THREAD_H

#include "ThreadImp.h"
#include "../../util/util.h"

enum UpdDynaSignalPropertyEnum
{
	// �������³�ʼ��
	UPD_DYNA_RESET = NSP_RESERVE + 1,
	
};

class CUpdateDynaDataThread : public ThreadImp {
public:
	CUpdateDynaDataThread();
	~CUpdateDynaDataThread();

protected:
	bool InitialThreadEnv();						// ��ʼ�̻߳���
	int StartThread();								// �����߳�
	static void* ThreadProc(void*);					// �̴߳�����
	
private:
	time_t m_ExecuteUpdateHqDataTimer;				// ���һ��ִ��ȡ���鶯̬����
	time_t m_ExecuteGetHqStaticChangeTimer;			// ���һ��ִ��ȡ���龲̬���ݱ仯��Timer
	time_t m_GetDyanUserChangeTimer;				// ���һ��ִ��ȡ�û��б��ʱ��
	
private:
	int ExecuteInitial();							// ִ�г�ʼ��
	int GetDynaData();								// ȡ��̬����
	int ExecuteGetStaticDataChange();				// ��ʼ����ȡ��̬�����Ƿ����仯
	void NotifyResetStkLatestInfo();				// ֪ͨ���ýڵ��Ӧ���¶�̬����ָ��
	void NotifyExeEWarnCalc(bool);					// ֪ͨ����ɼ�Ԥ����Ϣ
	int ProcedureMsgQueue();						// ������Ϣ����
	int SendInitialRequest();						// ���ͳ�ʼ������
	const int ResumeHangupInitialRequest();			// �ָ����ʼ���������Ӧ
	int ProcessInitialRequest();					// ��Ӧ��ʼ������
	int ProcessGetDynaUserList();					// ��������û��б�
};


#endif  /* _UPD_DYNA_DATA_THREAD_H */
