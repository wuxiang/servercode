/*****
** 运算处理线程
** 用以处理条件判断，数据整合
********************************/
#ifndef _CAL_OPERATING_THREAD_H
#define _CAL_OPERATING_THREAD_H

#include "ThreadImp.h"
#include "../model/template/IndexList.h"
#include <list>

class LargeMemoryCache;

enum TimesliceFlag
{
	// 开始
	TF_BEGIN = 0,
	// 继续上次
	TF_CONTINUE,
	// 结束
	TF_END,
};

// 分段扫描标识
struct StageScanTag
{
	TimesliceFlag uState;
	IndexMap UserMark;
};

class CalOperatingThread : public ThreadImp {
public:
	CalOperatingThread();
	~CalOperatingThread();
	
	enum CalcSignalPropertyEnum
	{
		CSP_HQ_INDEX_RESET = NSP_RESERVE + 1,
		CSP_EXE_WARN,
		CSP_EXE_INFOMINE,
		CSP_EXE_NOTICE,
		CSP_EXE_INFOMINE_STATIS,
	};

	/*** m_iSignalProperty定义
	**	bit						含义说明(0均标识无操作)
	**	CSP_HQ_INDEX_RESET		要求重置行情动态记录指针
	**	CSP_EXE_WARN			动态行情有更新，执行股价预警
	**	CSP_EXE_INFOMINE		信息地雷有更新，执行信息地雷扫描
	**	CSP_EXE_NOTICE			公告有更新，执行公告扫描
	**	CSP_EXE_INFOMINE_STATIS 信息地雷统计
	***************************************************************/

private:
	LargeMemoryCache *m_pSendBuf;								// 发送数据缓存(线程内user用户回复数据用)
	StageScanTag m_PublicNoticeFlag;							// 公告扫描标识
	StageScanTag m_SelfStkInfoMineStatisFlag;					// 自选股信息地雷统计扫描标识

// 分类列表
private:
	CIndexList *m_pEwarningUserList;							// 预警用户列表
	CIndexList *m_pActiveUserList;								// 活跃用户列表

// 定时时钟
private:
	time_t m_ExecuteStkWarningTimer;							// 最近一次执行股价预警计算时间
	time_t m_ExecuteStkPrivateNoticTimer;						// 最近一次执行自选信息地雷计算时间
	time_t m_ExecutePublicNoticTimer;							// 最近一次执行公共信息计算时间
	time_t m_ExecuteUserAliveCheckTimer;						// 最近一次执行检查用户存活状态时间
	time_t m_ExecuteSelfMineStatisTimer;						// 最近一次执行自选股信息地雷统计时间

private:
	int ProcessStkPriceWarningCal();							// 执行节点股价预警计算
	int PlatformStkPriceWarningCal(const IndexMap*);			// 平台股价预警
	
	int ProcessPublicNoticeScan(int &);							// 执行公共新闻公告相关扫描
	int PlatformNoticeScan(const IndexMap*);					// 平台公告
	
	int ProcessUserAliveCheck();								// 执行检查用户存活状态
	int PlatformUserAliveCheck(const IndexMap*, const time_t);	// 平台用户状态检查
	
	int ExecuteInitial();										// 执行初始化
	int PlatformUserInitial(const IndexMap*);					// 平台用户例行初始化
	
	int DetectStkLatestResetSignal();							// 监测重置证券行情最新信息信号
	int PlatformStkLatestReset(const IndexMap*);				// 平台用户处理行情最新信息重置
	
	int ProcedureMsgQueue();									// 处理消息队列
	
	int ProcessSelfStkInfoMineScan();							// 执行自选股信息地雷扫描
	int PlatformSelfStkInfoMineScan(const IndexMap*);			// 平台自选股信息地雷扫描
	
protected:
	bool InitialThreadEnv();									// 初始线程环境
	int StartThread();											// 启动线程
	static void* ThreadProc(void*);								// 线程处理函数

public:
	// 添加预警用户
	void AddEWarningUser(const unsigned int, const unsigned char);
	// 移除预警用户
	void RemoveEWarningUser(const unsigned int, const unsigned char);
	// 添加活跃用户
	void AddActiveUser(const unsigned int, const unsigned char);
	// 移除活跃用户
	void RemoveActiveUser(const unsigned int, const unsigned char);
	// 获取响应资源缓存
	LargeMemoryCache* GetResponseCache();
};


#endif  /* _CAL_OPERATING_THREAD_H */
