#include "SendOuterDataThread.h"
#include "../../util/util.h"
#include "../../util/log/log.h"
#include "../../util/time/time_util.h"
#include "../../util/common/common_env.h"
#include "../../controller/runtime.h"
#include "../config.h"
#include "../data_type.h"
#include "../model/outerservpush/SendToOuterBase.h"
#include "../model/outerservpush/Push2WP.h"
#include "../model/outerservpush/AppPushNotice.h"
#include "../model/outerservpush/SSLCommon.h"
#include "../model/outerservpush/Push2Win8.h"
#include "../model/pushuser/PushUserManage.h"
#include "../model/pushuser/PushUser.h"
#include "../model/pushuser/AndroidPushUser.h"
#include "../model/pushuser/IphonePushUser.h"
#include "../model/pushuser/Wp7PushUser.h"
#include "../model/pushuser/AndroidPushUserManage.h"
#include "../model/pushuser/IosPushUserManage.h"
#include "../model/pushuser/Wp7PushUserManage.h"
#include "../model/pushuser/Win8PushUser.h"
#include "../model/pushuser/Win8PushUserManage.h"
#include "../model/template/OuterSendMark.h"


// 平台发送集成管理
class CPlatFormSendIntegrate
{
private:
	// 发送指针
	PTR_SendOuter *m_pSendBase;
	// 实例数目
	const int m_iMemCount;
	// 目前的状态
	int m_iStatus;
	// 当前活跃的节点索引
	int m_iCurrentActiveIndex;

public:	
	CPlatFormSendIntegrate(const int MemCount, const enum PlatFormClassifiedCodeEnum PlatformClassified)
		: m_iMemCount(MemCount),
		m_iStatus(-1),
		m_iCurrentActiveIndex(0)
	{
		m_pSendBase = GenerateNewCase(PlatformClassified);
	}
	
	~CPlatFormSendIntegrate()
	{
		if (0 == m_iStatus && NULL != m_pSendBase)
		{
			for (int i = 0 ; i < m_iMemCount; i++)
			{
				delete m_pSendBase[i];
			}
			m_iMemCount > 1 ? (delete []m_pSendBase) : (delete m_pSendBase);
			m_pSendBase = NULL;
		}
	}
	
	CSendToOuterBase* GetIndexPlatFormSend(const int index)
	{
		if (NULL == m_pSendBase || index < 0 || index >= m_iMemCount)
		{
			return NULL;
		}
		
		return m_pSendBase[index];
	}
	
	const int GetLatestStatus()const
	{
		return m_iStatus;
	}
	
	int ExecuteSend(const char *token, const void *data)
	{
		CSendToOuterBase *pT = NULL;
		if (m_iMemCount < 1 || 0 != GetLatestStatus())
		{
			return -1;
		}
		
		pT = GetIndexPlatFormSend(m_iCurrentActiveIndex);
		if (NULL == pT)
		{
			return -2;
		}

		m_iCurrentActiveIndex++;
		if (m_iCurrentActiveIndex >= m_iMemCount)
		{
			m_iCurrentActiveIndex = 0;
		}
		
		return pT->PushNotification(token, data);
	}

private:	
	PTR_SendOuter* GenerateNewCase(const enum PlatFormClassifiedCodeEnum PlatformClassified)
	{
		PTR_SendOuter* pOut = NULL;
		switch(PlatformClassified)
		{
			case PFCC_WP7:
				{
					pOut = m_iMemCount > 1 ? (new PTR_SendOuter[m_iMemCount]) : (new PTR_SendOuter);
					bzero(pOut, sizeof(PTR_SendOuter) * m_iMemCount);
					for (int i = 0; i < m_iMemCount; i++)
					{
						pOut[i] = new Push2WP();
					}
				}
				m_iStatus = 0;
				break;
				
			case PFCC_IOS:
				{
					pOut = m_iMemCount > 1 ? (new PTR_SendOuter[m_iMemCount]) : (new PTR_SendOuter);
					bzero(pOut, sizeof(PTR_SendOuter) * m_iMemCount);
					for (int i = 0; i < m_iMemCount; i++)
					{
						pOut[i] = new CApplePushNotice();
					}
				}
				m_iStatus = 0;
				break;
				
			case PFCC_WIN8:
				{
					pOut = m_iMemCount > 1 ? (new PTR_SendOuter[m_iMemCount]) : (new PTR_SendOuter);
					bzero(pOut, sizeof(PTR_SendOuter) * m_iMemCount);
					for (int i = 0; i < m_iMemCount; i++)
					{
						pOut[i] = new CPushToWin8();
					}
				}
				m_iStatus = 0;
				
			default:
				break;
		}
		return pOut;
	}
};



CSendOuterDataThread::CSendOuterDataThread()
{
	m_SendIntegrate.clear();
}

CSendOuterDataThread::~CSendOuterDataThread()
{
	std::map<unsigned short, CPlatFormSendIntegrate*>::iterator iterScan = m_SendIntegrate.begin();
	while(m_SendIntegrate.end() != iterScan)
	{
		CPlatFormSendIntegrate *pPlatFormSend = iterScan->second;
		if (NULL != pPlatFormSend)
		{
			delete pPlatFormSend;
		}
		iterScan++;
	}
	m_SendIntegrate.clear();
	
	while(!m_SendQueue.empty())
	{
		m_SendQueue.pop();
	}
	
	// 释放SSL 
	CSslCommon::Release();
}

// 初始线程环境
bool
CSendOuterDataThread::InitialThreadEnv() {
	ThreadImp::InitialThreadEnv();
	time_t NowTime = GetNowTime();
	
	// 初始化SSl
	CSslCommon::Initial();
		
	if (!InitialOuterSendManage())
	{
		return false;
	}
	
	// 置位已经初始化(启动后不需要马上进程初始化)
	// 日期
	m_bHasInitialed = true;
	m_iInitialDate = GetIntDate(&NowTime);
	
	return true;
}

// 启动线程
int
CSendOuterDataThread::StartThread() {
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

	if (pthread_create(&tid, &thread_attr, CSendOuterDataThread::ThreadProc, this))
	{
		FATAL("Create SendOuterDataThread thread error");
		exit(-2);
		return -2;
	}

	SetThreadID(tid);
	pthread_detach(tid);
	return 0;
}

// 线程处理函数
void*
CSendOuterDataThread::ThreadProc(void *param) {
	CSendOuterDataThread *pCurrentThread = (CSendOuterDataThread*)param;
	if (NULL == pCurrentThread) {
		return 0;
	}
	TRACE("CSendOuterDataThread[%u] Started.", (unsigned int)pthread_self());

	while(1) {
		if (Thread_Terminate == pCurrentThread->GetThreadRunState()) {
			DEBUG("CSendOuterDataThread[%u] Terminate", (unsigned int)pthread_self());
			break;
		}
		else if (Thread_Running == pCurrentThread->GetThreadRunState()) {
			// 执行处理
			if (0 == (
				pCurrentThread->ProcedureMsgQueue()
				* pCurrentThread->ProcessSend()
				))
			{
				continue;
			}
		}
		else if (Thread_Waiting == pCurrentThread->GetThreadRunState()) {

		}
		else if (Thread_Initial == pCurrentThread->GetThreadRunState()) {

		}
		SleepMs(SEND_OUTER_THREAD_TIME_INTERVAL);
	}
	return 0;
}

// 初始化发送管理
bool
CSendOuterDataThread::InitialOuterSendManage()
{
	std::map<unsigned short, OuterPushServ> &OuterPushServList = GetOuterPushServMap();
	
	std::map<unsigned short, OuterPushServ>::iterator iterScan = OuterPushServList.begin();
	while(OuterPushServList.end() != iterScan)
	{
		OuterPushServ *pServPush = &iterScan->second;
		if (NULL == pServPush || pServPush->ConnectNum < 1)
		{
			ERROR("iCnnNum error, PlatFormCode=%u", iterScan->first);
			return false;
		}
		
		unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(pServPush->PlatformCode);
		CPlatFormSendIntegrate *pPlatFormSend = new CPlatFormSendIntegrate(pServPush->ConnectNum, 
				(PlatFormClassifiedCodeEnum)uPlatFormClassifiedCode);
		
		int iInitialed = 0;
		for (int i = 0; i < pServPush->ConnectNum; i++)
		{
			CSendToOuterBase *pNode = pPlatFormSend->GetIndexPlatFormSend(pServPush->ConnectNum - 1 - i);
			if (NULL == pNode)
			{
				ERROR("pNode = NULL[index=%d cer=%s]", pServPush->ConnectNum - 1 - i, pServPush->CertFile);
				continue;
			}
			if (!pNode->Initialize(pServPush->CertFile, pServPush->KeyFile, pServPush))
			{
				DEBUG("Initialize failed.");
				continue;
			}
			iInitialed++;
		}
		
		if (iInitialed != pServPush->ConnectNum)
		{
			ERROR("Initialize error, PlatFormCode=%u", iterScan->first);
			return false;
		}
		
		m_SendIntegrate.insert(std::pair<unsigned short, CPlatFormSendIntegrate*>(pServPush->PlatformCode,
					pPlatFormSend));
		
		iterScan++;
	}
		
	return true;
}

// 存入发送队列(0成功 -1失败 1已满)
int 
CSendOuterDataThread::PushToSendQueue(const unsigned char PlatFormCode, 
	const unsigned int UserMapID, const unsigned char MsgType)
{
	const size_t uMaxCacheCount = 1500000;
	
	if (m_muCriticalArea.try_lock())
	{
		if (m_SendQueue.size() >= uMaxCacheCount)
		{
			// Full
			NOTE("Warn >>>OuterDataThread queue is full. <<<");
			goto CACHE_FULL;
		}
		
		COuterSendMark temMark(PlatFormCode, UserMapID, MsgType);
		m_SendQueue.push(temMark);
		
		m_muCriticalArea.unlock();
		return 0;
	}
	
	return -1;
	
CACHE_FULL:
	m_muCriticalArea.unlock();
	return 1;
}

// 获取发送平台管理
CPlatFormSendIntegrate*
CSendOuterDataThread::GetPlatformSendInte(const unsigned char PlatFormCode)
{
	CPlatFormSendIntegrate *pCurrentInteg = NULL;
	std::map<unsigned short, CPlatFormSendIntegrate*>::iterator iterFind = 
		m_SendIntegrate.find(PlatFormCode);
	
	if (m_SendIntegrate.end() != iterFind)
	{
		pCurrentInteg = iterFind->second;
	}
	
	return pCurrentInteg;
}

// 处理发送
int
CSendOuterDataThread::ProcessSendMark(COuterSendMark &item)
{
	const unsigned char cPlatFormCode = item.GetPlatFormCode();
	const unsigned int uUserMapId = item.GetUserMapID();
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(cPlatFormCode);
	
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			{
				CWp7PushUserManage *pUserManage = CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
				CWp7PushUser *pUser = pUserManage->GetUser(uUserMapId);
				if (NULL == pUser || pUser->IsDead())
				{
					DEBUG("Invalid user.");
					return -2;
				}
				
				return pUser->SendOuterMsg(uUserMapId, item.GetMsgType());
			}
			break;
		
		case PFCC_IOS:
			{
				CIosPushUserManage *pUserManage = CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
				CIphonePushUser *pUser = pUserManage->GetUser(uUserMapId);
				if (NULL == pUser || pUser->IsDead())
				{
					DEBUG("Invalid user.");
					return -2;
				}
				
				return pUser->SendOuterMsg(uUserMapId, item.GetMsgType());
			}
			break;
			
		case PFCC_WIN8:
			{
				CWin8PushUserManage *pUserManage = CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
				CWin8PushUser *pUser = pUserManage->GetUser(uUserMapId);
				if (NULL == pUser || pUser->IsDead())
				{
					DEBUG("Invalid user.");
					return -2;
				}
				
				return pUser->SendOuterMsg(uUserMapId, item.GetMsgType());
			}
			break;
			
		default:
			break;
	}
	
	return -3;
}

// 发送缓存的内容
int
CSendOuterDataThread::ProcessSend()
{
	if (m_SendQueue.empty())
	{
		return -1;
	}
	
	if (IsHangupRequest())
	{
		return -2;
	}
	
	const unsigned int uMaxPerCycle = 50000;
	unsigned int uExeCount = 0;
	if (m_muCriticalArea.lock())
	{
		if (m_SendQueue.empty())
		{
			m_muCriticalArea.unlock();
			return -3;
		}
		
		while(!m_SendQueue.empty())
		{
			COuterSendMark temMark = m_SendQueue.front();
			m_SendQueue.pop();
			ProcessSendMark(temMark);
			++uExeCount;
			
			if (uExeCount >= uMaxPerCycle)
			{
				TRACE("Send %u records, now sleep %u ms.", uMaxPerCycle, SEND_OUTER_THREAD_TIME_INTERVAL * 100);
				SleepMs(SEND_OUTER_THREAD_TIME_INTERVAL * 100);
				break;
			}
		}
		m_muCriticalArea.unlock();
		
		return 0;
	}
	
	return -4;
}

// 用户执行发送
int
CSendOuterDataThread::UserExecuteSend(const unsigned char cPlatFormCode, 
	const char *token, const void *data)
{
	CPlatFormSendIntegrate *pCurrentInteg = GetPlatformSendInte(cPlatFormCode);
	if (NULL == pCurrentInteg)
	{
		ERROR("Invalid platformcode");
		return -1;
	}
	
	return pCurrentInteg->ExecuteSend(token, data);
}

// 处理消息队列
int
CSendOuterDataThread::ProcedureMsgQueue()
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

