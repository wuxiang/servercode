/*****
** 推送发送处理线程
** 用以处理推送消息的发送
********************************/
#ifndef _REQ_DYNA_NEWS_THREAD_H
#define _REQ_DYNA_NEWS_THREAD_H

#include "ThreadImp.h"

class ReqDynaNewsThread : public ThreadImp {
public:
	ReqDynaNewsThread();
	~ReqDynaNewsThread();

protected:
	bool InitialThreadEnv();									// 初始线程环境
	int StartThread();											// 启动线程
	static void* ThreadProc(void*);								// 线程处理函数
	
private:
	time_t m_ExecuteUpdatePublicNewsTimer;						// 最近一次执行取公共新闻动态数据
	time_t m_ExecuteUpdateInfoMineTimer;						// 最近一次执行取信息地雷数据
	time_t m_ScanUserTimer;										// 最近一次执行扫描用户连接时间
	time_t m_JudgeInfoMineStatisTimer;							// 判断执行信息地雷统计
	bool m_bInfoMineInitial;									// 信息地雷是否初始化成功
	bool m_bNoticeInitial;										// 公告是否初始成功
	bool m_bProcessUserScan;									// 是否执行了ProcessUserScan
	
private:
	int DetectInfoMineStatis();
	void NotifyExeInfoMineStatisCalc(bool);						// 通知执行信息地雷统计
	
private:
	int ExecuteInitial();										// 执行初始化
	int ExecuteInfoMineInitial();								// 执行地雷初始化
	int ExecutePublicNewsScan();								// 执行扫描公共信息
	int ExecuteInfoMineScan();									// 执行扫描信息地雷
	void NotifyExeInfoMineCalc(bool);							// 通知执行信息地雷
	void NotifyExeNoticeCalc(bool);								// 通知执行公告扫描
	int ProcessUserScan();										// 执行扫描用户
	int ExecuteInfoMineStatis();								// 执行信息地雷统计
	int ProcedureMsgQueue();									// 处理消息队列
	
};


#endif  /* _REQ_DYNA_NEWS_THREAD_H */
