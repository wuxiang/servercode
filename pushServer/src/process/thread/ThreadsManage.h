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
	static ThreadImp* GetThread(const char*);						// 获取线程
	
public:
	static bool CreateAllThread();									// 创建所有线程
	static bool StartAllThread();									// 启动所有线程
	static void ExitMange();										// 退出
	static CalOperatingThread* GetIndexCalcThread(const int);		// 获取指定索引的计算线程
	static void SendAllThreadRun();									// 通知所有线程运行
	static NetThread* GetNetThread();								// 获取网络线程
	static CNodeThreadMutex* GetNodeThreadMutexManage();			// 获取资源控制
	static CSendOuterDataThread* GetOutSendThread();				// 获取外部发送线程
	static CUpdateDynaDataThread* GetUpdDataThread();				// 获取请求数据线程
	static void HangupOtherRequest(ThreadImp*, bool);				// 挂起请求
};

#endif  /* _THREADS_MANAGE_H */

