#include "ReqDynaNewsThread.h"
#include "../../util/util.h"
#include "../../util/log/log.h"
#include "../../util/net/socket_util.h"
#include "../../util/common/common_env.h"
#include "../../util/time/time_util.h"
#include "../config.h"
#include "../../controller/runtime.h"
#include "../model/db/PublicNews.h"
#include "../model/db/InfoMine.h"
#include "../model/db/DatabaseSnapshot.h"
#include "ThreadsManage.h"
#include "CalOperatingThread.h"
#include "../model/pushuser/PushUserManage.h"
#include "../model/ewarning/EarlyWarning.h"
#include "../model/ewarning/EarlyWarningManage.h"

ReqDynaNewsThread::ReqDynaNewsThread()
	: m_bInfoMineInitial(false),
	m_bNoticeInitial(false),
	m_bProcessUserScan(false)
{
}


ReqDynaNewsThread::~ReqDynaNewsThread() {
}

// 初始线程环境
bool
ReqDynaNewsThread::InitialThreadEnv() {

	ThreadImp::InitialThreadEnv();
	time_t NowTime = GetNowTime();
	m_ExecuteUpdatePublicNewsTimer = NowTime;
	m_ExecuteUpdateInfoMineTimer = NowTime;
	m_ScanUserTimer = NowTime;
	m_JudgeInfoMineStatisTimer = NowTime;

	return true;
}

// 启动线程
int
ReqDynaNewsThread::StartThread() {
	pthread_t  tid;
	pthread_attr_t   thread_attr;

	pthread_attr_init(&thread_attr);
	/* 设置堆栈大小*/
	if(pthread_attr_setstacksize(&thread_attr, 1024*1024) != 0)//1024 K
	{
		FATAL("pthread_attr_setstacksize failed.");
		exit(-1);
		return -1;
	}

	if (pthread_create(&tid, &thread_attr, ReqDynaNewsThread::ThreadProc, this))
	{
		FATAL("Create RespPushThread thread error");
		exit(-2);
		return -2;
	}
	SetThreadID(tid);
	pthread_detach(tid);

	return 0;
}

// 线程处理函数
void*
ReqDynaNewsThread::ThreadProc(void *param) {
	ReqDynaNewsThread *pCurrentThread = (ReqDynaNewsThread*)param;
	if (NULL == pCurrentThread) {
		return 0;
	}

	while(1) {
		if (Thread_Terminate == pCurrentThread->GetThreadRunState()) {
			DEBUG("ReqDynaNewsThread[%u] Terminate", (unsigned int)pthread_self());
			break;
		}
		else if (Thread_Running == pCurrentThread->GetThreadRunState()) {
			if (0 == (
				pCurrentThread->ExecuteInitial()
				* pCurrentThread->ProcedureMsgQueue()
				* pCurrentThread->ExecutePublicNewsScan()
				* pCurrentThread->ExecuteInfoMineStatis()
				* pCurrentThread->ExecuteInfoMineScan()
				* pCurrentThread->ProcessUserScan()
				))
			{
				continue;
			}
		}
		else if (Thread_Waiting == pCurrentThread->GetThreadRunState()) {

		}
		else if (Thread_Initial == pCurrentThread->GetThreadRunState()) {

		}
		SleepMs(DYNA_NEWS_THD_TIME_INTERVAL);
	}
	return 0;
}

// 执行初始化
int
ReqDynaNewsThread::ExecuteInitial()
{
	if (!CanResetNow())
	{
		return -1;
	}

	if (IsHangupRequest())
	{
		return -2;
	}

	// 成功执行初始化后，不需要每天都执行初始化
	if (!m_bHasInitialed)
	{
		m_bProcessUserScan = false;

		if (m_bInfoMineInitial && m_bNoticeInitial)
		{
			m_bHasInitialed = true;
			return -3;
		}
	}

	DEBUG("ReqDynaNewsThread::ExecuteInitial");
	if (!m_bInfoMineInitial)
	{
		if (CInfoMine::getInfoMinePointer()->InitialData())
		{
			m_bInfoMineInitial = true;
		}
		else
		{
			ERROR("InfoMine:InitialData failed");
			m_bInfoMineInitial = false;
		}
	}

	if (!m_bNoticeInitial)
	{
		if (CPublicNews::getPublicNewsPointer()->InitialData())
		{
			m_bNoticeInitial = true;
			NotifyExeNoticeCalc(true);
		}
		else
		{
			ERROR("PublicNews:InitialData failed");
			m_bNoticeInitial = false;
		}

	}

	if (m_bInfoMineInitial && m_bNoticeInitial)
	{
		m_bHasInitialed = true;
	}

	return 0;
}

// 执行扫描公共信息
int
ReqDynaNewsThread::ExecutePublicNewsScan()
{
	if (!m_bNoticeInitial)
	{
		return -1;
	}

	if (IsHangupRequest())
	{
		return -2;
	}

	time_t NowTime = GetNowTime();

	// 当前时间
	int iTime = GetIntTime(&NowTime);
	if (iTime < GetNoticeRuntime()->RunStartTime || iTime > GetNoticeRuntime()->RunEndTime)
	{
		NotifyExeNoticeCalc(false);
		return -3;
	}

	if (DiffTimeSecond(NowTime, m_ExecuteUpdatePublicNewsTimer) < GetNoticeRuntime()->GetLastestInterval) {
		return -4;
	}
	m_ExecuteUpdatePublicNewsTimer = NowTime;

	// 更新最新公告消息
	if (CPublicNews::getPublicNewsPointer()->GetDynaData() > 0)
	{
		TRACE("NotifyExeNoticeCalc.");
		NotifyExeNoticeCalc(true);
	}
	return 0;
}

// 通知执行公告扫描
void
ReqDynaNewsThread::NotifyExeNoticeCalc(bool bExecute)
{
	int iCalThread = GetCalcThreadNum();
	for (int i = 0; i < iCalThread; i++)
	{
		CalOperatingThread* pThread = (CalOperatingThread*)ThreadsManage::GetIndexCalcThread(i);
		if (NULL != pThread)
		{
			pThread->SendMsg(UPD_EXE_NOTICE, GetThreadKey(), bExecute);
		}
	}
}

// 执行扫描信息地雷
int
ReqDynaNewsThread::ExecuteInfoMineScan()
{
	if (!m_bInfoMineInitial)
	{
		return -1;
	}

	if (IsHangupRequest())
	{
		return -2;
	}

	time_t NowTime = GetNowTime();

	// 当前时间
	int iTime = GetIntTime(&NowTime);
	if (iTime < GetInfoMineRuntime()->RunStartTime || iTime > GetInfoMineRuntime()->RunEndTime)
	{
		NotifyExeInfoMineCalc(false);
		return -3;
	}

	if (DiffTimeSecond(NowTime, m_ExecuteUpdateInfoMineTimer) < GetInfoMineRuntime()->GetLastestInterval) {
		return -4;
	}
	m_ExecuteUpdateInfoMineTimer = NowTime;

	// 更新最新信息地雷
	if (CInfoMine::getInfoMinePointer()->GetDynaData() > 0)
	{
		// 获取当前时刻在时间段中的位置
		SelfSelectStockInfomineConfigT *pSelfSelectStockInfomineConfig = GetSelfSelectStockInfomineConfig();
		int iTimeIndex = -1;
		for(int i = pSelfSelectStockInfomineConfig->StatisticsTimeCount - 1; i >= 0; i--)
		{
			if (iTime >= pSelfSelectStockInfomineConfig->StatisticsTime[i])
			{
				iTimeIndex = i;
				break;
			}
		}
		if (iTimeIndex < 0 || (int)pSelfSelectStockInfomineConfig->InfoMineStatisTimes != iTimeIndex + 1)
		{
			return 0;
		}

		NotifyExeInfoMineCalc(true);
	}

	return 0;
}

// 通知执行信息地雷
void
ReqDynaNewsThread::NotifyExeInfoMineCalc(bool bExecute)
{
	int iCalThread = GetCalcThreadNum();
	for (int i = 0; i < iCalThread; i++)
	{
		CalOperatingThread* pThread = (CalOperatingThread*)ThreadsManage::GetIndexCalcThread(i);
		if (NULL != pThread)
		{
			pThread->SendMsg(UPD_EXE_INFOMINE, GetThreadKey(), bExecute);
		}
	}
}

// 执行扫描用户
int
ReqDynaNewsThread::ProcessUserScan()
{
	time_t NowTime = GetNowTime();
	float fTimeEsc = DiffTimeSecond(NowTime, m_ScanUserTimer) / (60.0);
	unsigned int uJudgeInter = 11;
	OperSyncUserInterval(false, uJudgeInter);
	if (m_bProcessUserScan || fTimeEsc < uJudgeInter)
	{
		return -1;
	}
	m_ScanUserTimer = NowTime;

	if (IsHangupRequest())
	{
		return -2;
	}

	// 当前时间
	int iTime = GetIntTime(&NowTime);

	const UserNodeServerT* pSetting = GetUserNodeServer();
	if (NULL != pSetting && iTime >= pSetting->CheckStart && iTime < pSetting->CheckEnd)
	{
		TRACE("ProcessUserScan Start.");
		ThreadsManage::HangupOtherRequest(this, true);
		SleepMs(60*1000);

		// 同步记录
		CPushUserManage::SyncLatestToDb();
		// 整理死亡的无效用户
		CPushUserManage::RearrangeUserNode();
		// 整理无效预警记录
		CEwarningManage::RearrangeNode();
		// 显示结果
		CPushUserManage::PrintLatesUserInfo();

		ThreadsManage::HangupOtherRequest(this, false);
		TRACE("ProcessUserScan Finish.");
		m_bProcessUserScan = true;
	}

	return 0;
}

int
ReqDynaNewsThread::DetectInfoMineStatis()
{
	const int iTodayStart = 53000;
	if (!m_bInfoMineInitial)
	{
		return -1;
	}

	if (IsHangupRequest())
	{
		return -2;
	}

	// 当前时间
	time_t NowTime = GetNowTime();
	int iTime = GetIntTime(&NowTime);

	if (iTime < GetInfoMineRuntime()->RunStartTime || iTime > GetInfoMineRuntime()->RunEndTime)
	{
		CInfoMine::getInfoMinePointer()->SetStartRTInfoMineFlag(false);
		return -3;
	}

	if (DiffTimeSecond(NowTime, m_JudgeInfoMineStatisTimer) < 93) {
		return -4;
	}
	m_JudgeInfoMineStatisTimer = NowTime;

	SelfSelectStockInfomineConfigT *pSelfSelectStockInfomineConfig = GetSelfSelectStockInfomineConfig();
	// 判断是否当天已经执行完毕
	if ((short)pSelfSelectStockInfomineConfig->InfoMineStatisTimes >= pSelfSelectStockInfomineConfig->StatisticsTimeCount)
	{
		return 0;
	}

	// 获取当前时刻在时间段中的位置
	int iTimeIndex = -2;
	for(int i = pSelfSelectStockInfomineConfig->StatisticsTimeCount - 1; i >= 0; i--)
	{
		if (iTime >= pSelfSelectStockInfomineConfig->StatisticsTime[i])
		{
			iTimeIndex = i;
			break;
		}
	}
	if (iTimeIndex < 0)
	{
		return 0;
	}
	if (pSelfSelectStockInfomineConfig->InfoMineStatisTimes == iTimeIndex + 1)
	{
		return 0;
	}

	TRACE("ExecuteInfoMineStatis[iTimeIndex=%d]", iTimeIndex);
	// 执行
	if (CInfoMine::getInfoMinePointer()->ExecuteInfoMineStatis(iTodayStart) > 0)
	{
		pSelfSelectStockInfomineConfig->InfoMineStatisTimes = iTimeIndex + 1;
		pSelfSelectStockInfomineConfig->LastestUpdStatistics = ExchangeSixTimeToTime_t(pSelfSelectStockInfomineConfig->StatisticsTime[iTimeIndex]);
		CInfoMine::getInfoMinePointer()->SetStartRTInfoMineFlag(true);
	}

	return 0;
}

// 执行信息地雷统计
int
ReqDynaNewsThread::ExecuteInfoMineStatis()
{
	int iRes = DetectInfoMineStatis();
	if (iRes < 0)
	{
		NotifyExeInfoMineStatisCalc(false);
		return iRes;
	}

	if (CInfoMine::getInfoMinePointer()->IsInfoMineStatisSuccess())
	{
		NotifyExeInfoMineStatisCalc(true);
	}
	return iRes;
}

// 通知执行信息地雷统计
void
ReqDynaNewsThread::NotifyExeInfoMineStatisCalc(bool bExecute)
{
	int iCalThread = GetCalcThreadNum();
	for (int i = 0; i < iCalThread; i++)
	{
		CalOperatingThread* pThread = (CalOperatingThread*)ThreadsManage::GetIndexCalcThread(i);
		if (NULL != pThread)
		{
			pThread->SendMsg(UPD_EXE_INFOMINE_STATIS, GetThreadKey(), bExecute);
		}
	}
}

// 处理消息队列
int
ReqDynaNewsThread::ProcedureMsgQueue()
{
	int iCount = -1;

	while(!IsEmpty())
	{
		iCount++;
		CMsgMark Msg;
		if (-1 != GetFront(Msg))
		{
			switch(Msg.m_Msg)
			{
				case UPD_HUNGUP_THREAD_PROC:
					{
						SetSignal(NSP_HANGUP_REQUEST, 1 == Msg.m_wParam);
					}
					break;

				default:
					DEBUG("unused msg found[%u]", Msg.m_Msg);
					break;
			}
			PopFront();
		}
	}

	return -1 == iCount ? -1 : 0;
}



