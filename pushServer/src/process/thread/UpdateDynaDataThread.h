/*****
** 动态数据更新处理线程
** 用以处理态数据更新
********************************/
#ifndef _UPD_DYNA_DATA_THREAD_H
#define _UPD_DYNA_DATA_THREAD_H

#include "ThreadImp.h"
#include "../../util/util.h"

enum UpdDynaSignalPropertyEnum
{
	// 请求重新初始化
	UPD_DYNA_RESET = NSP_RESERVE + 1,
	
};

class CUpdateDynaDataThread : public ThreadImp {
public:
	CUpdateDynaDataThread();
	~CUpdateDynaDataThread();

protected:
	bool InitialThreadEnv();						// 初始线程环境
	int StartThread();								// 启动线程
	static void* ThreadProc(void*);					// 线程处理函数
	
private:
	time_t m_ExecuteUpdateHqDataTimer;				// 最近一次执行取行情动态数据
	time_t m_ExecuteGetHqStaticChangeTimer;			// 最近一次执行取行情静态数据变化的Timer
	time_t m_GetDyanUserChangeTimer;				// 最近一次执行取用户列表的时间
	
private:
	int ExecuteInitial();							// 执行初始化
	int GetDynaData();								// 取动态数据
	int ExecuteGetStaticDataChange();				// 初始化后取静态数据是否发生变化
	void NotifyResetStkLatestInfo();				// 通知重置节点对应最新动态数据指针
	void NotifyExeEWarnCalc(bool);					// 通知计算股价预警信息
	int ProcedureMsgQueue();						// 处理消息队列
	int SendInitialRequest();						// 发送初始化请求
	const int ResumeHangupInitialRequest();			// 恢复因初始化挂起的响应
	int ProcessInitialRequest();					// 响应初始化请求
	int ProcessGetDynaUserList();					// 请求跟踪用户列表
};


#endif  /* _UPD_DYNA_DATA_THREAD_H */
