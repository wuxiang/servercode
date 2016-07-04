#include "UpdateDynaDataThread.h"
#include "../../util/log/log.h"
#include "../../util/net/socket_util.h"
#include "../../util/time/time_util.h"
#include "../../util/common/common_env.h"
#include "../../controller/runtime.h"
#include "../config.h"
#include "../quota/QuotesSnapshot.h"
#include "ThreadsManage.h"
#include "CalOperatingThread.h"
#include "NetThread.h"
#include "../model/db/DatabaseSnapshot.h"
#include "../model/loguser/Loguser.h"

CUpdateDynaDataThread::CUpdateDynaDataThread()
{
}


CUpdateDynaDataThread::~CUpdateDynaDataThread()
{
	CQuotesSnapshot::ReleaseRes();
}

// 初始线程环境
bool
CUpdateDynaDataThread::InitialThreadEnv()
{
	m_ExecuteUpdateHqDataTimer = GetNowTime();
	m_ExecuteGetHqStaticChangeTimer = GetNowTime();
	m_GetDyanUserChangeTimer = GetNowTime();
	return ThreadImp::InitialThreadEnv();
}

// 启动线程
int
CUpdateDynaDataThread::StartThread()
{
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

	if (pthread_create(&tid, &thread_attr, CUpdateDynaDataThread::ThreadProc, this))
	{
		FATAL("Create CUpdateDynaDataThread thread error");
		exit(-2);
		return -2;
	}
	SetThreadID(tid);
	pthread_detach(tid);
	return 0;
}

// 线程处理函数
void*
CUpdateDynaDataThread::ThreadProc(void *param)
{
	CUpdateDynaDataThread *pCurrentThread = (CUpdateDynaDataThread*)param;
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
				* pCurrentThread->ExecuteGetStaticDataChange()
				* pCurrentThread->GetDynaData()
				* pCurrentThread->ProcessGetDynaUserList()
				))
			{
				continue;
			}
		}
		else if (Thread_Waiting == pCurrentThread->GetThreadRunState()) {

		}
		else if (Thread_Initial == pCurrentThread->GetThreadRunState()) {

		}
		SleepMs(DYNA_HQTHREAD_TIME_INTERVAL);
	}
	return 0;
}

// 执行初始化
int
CUpdateDynaDataThread::ExecuteInitial()
{
	if (!CanResetNow())
	{
		return -1;
	}

	if (IsHangupRequest())
	{
		return -2;
	}

	if (!UnderWait(UPD_DYNA_THREAD_CONFIRM_RESET))
	{
		NotifyExeEWarnCalc(false);
		SendInitialRequest();
	}

	if (!GetSignal(UPD_DYNA_RESET))
	{
		return -3;
	}

	DEBUG("CUpdateDynaDataThread::ExecuteInitial");
	ResetWaitFlag(UPD_DYNA_THREAD_CONFIRM_RESET);
	SetSignal(UPD_DYNA_RESET, false);

	// 取数据
	if (CQuotesSnapshot::InitialData())
	{
		ResumeHangupInitialRequest();
		NotifyResetStkLatestInfo();
		m_bHasInitialed = true;
	}

	return 0;
}

// 取动态数据
int
CUpdateDynaDataThread::GetDynaData()
{
	if (!m_bHasInitialed)
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
	if (iTime < GetEWarningRuntime()->RunStartTime || iTime > GetEWarningRuntime()->RunEndTime)
	{
		NotifyExeEWarnCalc(false);
		return -3;
	}

	if (DiffTimeSecond(NowTime, m_ExecuteUpdateHqDataTimer) < GetEWarningRuntime()->GetLastestInterval) {
		return -4;
	}
	m_ExecuteUpdateHqDataTimer = NowTime;

	if (CQuotesSnapshot::UpdateDynaData())
	{
		NotifyExeEWarnCalc(true);
	}
	return 0;
}

// 初始化后取静态数据是否发生变化
int
CUpdateDynaDataThread::ExecuteGetStaticDataChange()
{
	if (!m_bHasInitialed)
	{
		return -1;
	}

	if (IsHangupRequest())
	{
		return -2;
	}

	time_t NowTime = GetNowTime();
	if (DiffTimeSecond(NowTime, m_ExecuteGetHqStaticChangeTimer) < GET_STATCI_HQ_DATA_INTERVAL) {
		return -3;
	}
	m_ExecuteGetHqStaticChangeTimer = NowTime;

	// 当前时间
	int iTime = GetIntTime(&NowTime);
	if (iTime < GetInitialStartTime() || iTime > GetInitialEndTime())
	{
		return -4;
	}

	if (CQuotesSnapshot::HasStaticDateUpdate())
	{
		TRACE("Request static again.");
		// 置位重新初始化
		m_bHasInitialed = false;
		return 0;
	}
	return -1;
}

// 通知重置节点对应最新动态数据指针
void
CUpdateDynaDataThread::NotifyResetStkLatestInfo()
{
	int iCalThread = GetCalcThreadNum();
	for (int i = 0; i < iCalThread; i++)
	{
		CalOperatingThread* pThread = (CalOperatingThread*)ThreadsManage::GetIndexCalcThread(i);
		if (NULL != pThread)
		{
			pThread->SendMsg(UPD_HQ_INDEX_RESET, GetThreadKey(), -1);
		}
	}
}

// 通知计算股价预警信息
void
CUpdateDynaDataThread::NotifyExeEWarnCalc(bool bExecute)
{
	int iCalThread = GetCalcThreadNum();
	for (int i = 0; i < iCalThread; i++)
	{
		CalOperatingThread* pThread = (CalOperatingThread*)ThreadsManage::GetIndexCalcThread(i);
		if (NULL != pThread)
		{
			pThread->SendMsg(UPD_EXE_WARN, GetThreadKey(), -1);
		}
	}
}

// 处理消息队列
int
CUpdateDynaDataThread::ProcedureMsgQueue()
{
	int iRes = -1;

	while(!IsEmpty())
	{
		iRes++;
		CMsgMark Msg;
		if (-1 != GetFront(Msg))
		{
			switch(Msg.m_Msg)
			{
				case UPD_DYNA_THREAD_CONFIRM_RESET:
					{
						iRes = ProcessInitialRequest();
					}
					break;

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

	return -1 == iRes ? -1 : 0;
}

// 发送初始化请求
int
CUpdateDynaDataThread::SendInitialRequest()
{
	// 初始化响应
	ResetWaitFlag(UPD_DYNA_THREAD_CONFIRM_RESET);
	// 计算线程
	int iCalThread = GetCalcThreadNum();
	for (int i = 0; i < iCalThread; i++)
	{
		CalOperatingThread* pThread = ThreadsManage::GetIndexCalcThread(i);
		if (NULL != pThread)
		{
			if (-1 < pThread->SendMsg(UPD_DYNA_THREAD_REQUEST_RESET, GetThreadKey(), 1))
			{
				AddWaitFlag(UPD_DYNA_THREAD_CONFIRM_RESET);
			}
		}
	}

	// 网络线程
	if (-1 < ThreadsManage::GetNetThread()->SendMsg(UPD_DYNA_THREAD_REQUEST_RESET, GetThreadKey(), 1))
	{
		AddWaitFlag(UPD_DYNA_THREAD_CONFIRM_RESET);
	}

	return 0;
}

// 恢复因初始化挂起的响应
const int
CUpdateDynaDataThread::ResumeHangupInitialRequest()
{
	// 计算线程
	int iCalThread = GetCalcThreadNum();
	for (int i = 0; i < iCalThread; i++)
	{
		CalOperatingThread* pThread = ThreadsManage::GetIndexCalcThread(i);
		if (NULL != pThread)
		{
			pThread->SendMsg(UPD_HUNGUP_THREAD_PROC, GetThreadKey(), 0);
		}
	}

	// 网络线程
	ThreadsManage::GetNetThread()->SendMsg(UPD_HUNGUP_THREAD_PROC, GetThreadKey(), 0);
	return 0;
}

// 响应初始化请求
int
CUpdateDynaDataThread::ProcessInitialRequest()
{
	if (0 == RemoveWaitFlag(UPD_DYNA_THREAD_CONFIRM_RESET))
	{
		DEBUG("Reveive UPD_DYNA_THREAD_REQUEST_RESET");
		SetSignal(UPD_DYNA_RESET, true);
	}
	return 0;
}

// 请求跟踪用户列表
int
CUpdateDynaDataThread::ProcessGetDynaUserList()
{
	time_t NowTime = GetNowTime();

	if (DiffTimeSecond(NowTime, m_GetDyanUserChangeTimer) < (5 * 60)) {
		return -1;
	}

	if (IsHangupRequest())
	{
		return -2;
	}
	m_GetDyanUserChangeTimer = NowTime;
	GetDynaUserList();
	return 0;
}

