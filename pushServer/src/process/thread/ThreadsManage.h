#ifndef _THREADS_MANAGE_H
#define _THREADS_MANAGE_H

class CalOperatingThread;
class ThreadImp;
class CNodeThreadMutex;
class CSendOuterDataThread;
class NetThread;
class CUpdateDynaDataThread;

class ThreadsManage
{
private:
	ThreadsManage();
	~ThreadsManage();
	
private:
	static ThreadImp* GetThread(const char*);						// ��ȡ�߳�
	
public:
	static bool CreateAllThread();									// ���������߳�
	static bool StartAllThread();									// ���������߳�
	static void ExitMange();										// �˳�
	static CalOperatingThread* GetIndexCalcThread(const int);		// ��ȡָ�������ļ����߳�
	static void SendAllThreadRun();									// ֪ͨ�����߳�����
	static NetThread* GetNetThread();								// ��ȡ�����߳�
	static CNodeThreadMutex* GetNodeThreadMutexManage();			// ��ȡ��Դ����
	static CSendOuterDataThread* GetOutSendThread();				// ��ȡ�ⲿ�����߳�
	static CUpdateDynaDataThread* GetUpdDataThread();				// ��ȡ���������߳�
	static void HangupOtherRequest(ThreadImp*, bool);				// ��������
};

#endif  /* _THREADS_MANAGE_H */

