#include "NetThread.h"
#include "../config.h"
#include "../../util/log/log.h"
#include "../../controller/runtime.h"
#include "../../util/net/socket_util.h"
#include "../../util/string/string_util.h"
#include "../../util/common/common_env.h"
#include "../../util/time/time_util.h"
#include "../data_type.h"
#include "ThreadsManage.h"
#include "UpdateDynaDataThread.h"
#include "../model/hqlink/HqLinkNode.h"
#include "../model/hqlink/HqLinkManage.h"
#include "../model/pushuser/PushUserManage.h"
#include "../model/pushuser/PushUser.h"
#include "../model/outerservpush/AppPushNotice.h"
#include "../model/loguser/Loguser.h"

NetThread::NetThread()
	: m_uListenIndex(0xFFFFFFFF),
	m_pNetEvents(NULL),
	m_iEpollFd(-1),
	m_iListenServerFd(-1),
	m_pHqLinkNodeMange(new CHqLinkManage())
{
}

NetThread::~NetThread() {
	if (NULL != m_pNetEvents){
		delete []m_pNetEvents;
		m_pNetEvents = NULL;
	}

	// 释放epoll
	if(m_iEpollFd > -1) {
		close(m_iEpollFd);
	}
	// 释放监听的服务socket
	if (m_iListenServerFd > -1) {
		close(m_iListenServerFd);
	}

	if (NULL != m_pHqLinkNodeMange)
	{
		m_pHqLinkNodeMange->ReleaseResource();
		delete m_pHqLinkNodeMange;
		m_pHqLinkNodeMange = NULL;
	}
}

// 初始线程环境
bool
NetThread::InitialThreadEnv() {
	// 最大连接数
	int iMaxLink = GetMaxLinkedHq();
	// 创建监听epoll
	if (NULL == m_pNetEvents) {
		m_pNetEvents = new epoll_event[iMaxLink];
	}

	// 初始化死链检查定时器
	time_t NowTime = GetNowTime();
	m_LastCheckHqDeadLinkTimer = NowTime;

	// 置位已经初始化
	m_bHasInitialed = true;
	m_iInitialDate = GetIntDate(&NowTime);

	return ThreadImp::InitialThreadEnv();
}

// 启动线程
int
NetThread::StartThread() {

	// 初始化sockt
	if (!InitialNetListen()) {
		ERROR("InitialNetListen failed");
		return -1;
	}

	// 设置启动所有线程运行
	ThreadsManage::SendAllThreadRun();

	// 启动网络线程运行
	NetThread::ThreadProc(this);
	return 0;
}

// 线程处理函数
void*
NetThread::ThreadProc(void *param) {
	NetThread *pCurrentThread = (NetThread*)param;
	if (NULL == pCurrentThread) {
		return 0;
	}
	DEBUG("NetThread[%u] begin running.%d", (unsigned int)pthread_self(), pCurrentThread->GetThreadRunState());
	while(1) {
		if (Thread_Terminate == pCurrentThread->GetThreadRunState()) {
			DEBUG("NetThread[%u] Terminate", (unsigned int)pthread_self());
			break;
		}
		else if (Thread_Running == pCurrentThread->GetThreadRunState()) {
			// 更新系统时间
			UpdateNowTime();
			if (0 == (
				pCurrentThread->ExecuteInitial()
				* pCurrentThread->ProcedureMsgQueue()
				* pCurrentThread->ProcessDeadLink()
				* pCurrentThread->ProcessRequest()
				))
			{
				continue;
			}
		}
		else if (Thread_Waiting == pCurrentThread->GetThreadRunState()) {

		}
		else if (Thread_Initial == pCurrentThread->GetThreadRunState()) {

		}
		SleepMs(NET_THREAD_TIME_INTERVAL);
	}
	return 0;
}

// 初始化监听
bool
NetThread::InitialNetListen() {
	char cPort[16] = {0};

	m_iEpollFd = epoll_create(GetMaxLinkedHq());
	if (0 >= m_iEpollFd) {
		DEBUG("epoll_create failed");
		return false;
	}

	// socket 创建及启动监听
	m_iListenServerFd = Listen(NULL, Itoa(cPort, GetListenPort()), NULL, DEFAULT_LISTEN_BACKLOG);
	if (0 >= m_iListenServerFd) {
		DEBUG("Listen failed[%s]", strerror(errno));
		close(m_iEpollFd);
		return false;
	}
	SetNoBlockMode(m_iListenServerFd, true);

	// 注册epoll
	m_ExNetEvent.data.u64 = m_iListenServerFd;
	m_ExNetEvent.data.u64 <<= 32;
	m_ExNetEvent.data.u64 |= m_uListenIndex;
	m_ExNetEvent.events = EPOLLIN | EPOLLERR;
	if(epoll_ctl(m_iEpollFd, EPOLL_CTL_ADD, m_iListenServerFd, &m_ExNetEvent) < 0) {
		DEBUG("epoll_ctl failed[%s]", strerror(errno));
		close(m_iEpollFd);
		close(m_iListenServerFd);
		return false;
	}

	return true;
}

// 处理网络请求
int
NetThread::ProcessRequest()
{
	int	nEventNum = epoll_wait(m_iEpollFd, m_pNetEvents, GetMaxLinkedHq(), 0);
	int iCurrentType = 0;
	for (int i = 0; i < nEventNum; i++)
	{
		m_pCurrentProcLinkNode = NULL;
		unsigned int iNodeIndex = (unsigned int)m_pNetEvents[i].data.u64;
		// 新的用户连接请求
		if (m_uListenIndex == iNodeIndex)
		{
			iCurrentType = 0;
			ProcessNewHqLink(&m_pNetEvents[i]);
		}
		// 已连接用户接收数据
		else if (m_pNetEvents[i].events &(EPOLLIN|EPOLLERR))
		{
			iCurrentType = 1;
			ProcessLinkedHqRequest(&m_pNetEvents[i]);
		}
		// 有数据需要发送
		else if (m_pNetEvents[i].events &(EPOLLOUT|EPOLLERR))
		{
			iCurrentType = 2;
			ProcessSendData(&m_pNetEvents[i]);
		}

		// 置位信号量
		if (NULL != m_pCurrentProcLinkNode)
		{
			// 同步发送数据
			m_pCurrentProcLinkNode->ProcessCacheQueue();
			if (m_pCurrentProcLinkNode->CanSend())
			{
				if (1 == iCurrentType)
				{
					ModifyEpollEvent(m_pCurrentProcLinkNode, EPOLLOUT | EPOLLERR);
				}
			}
			else if (m_pCurrentProcLinkNode->CanReceive())
			{
				ModifyEpollEvent(m_pCurrentProcLinkNode, EPOLLIN | EPOLLERR);
			}
			else
			{
				if (0 != iCurrentType)
				{
					DelEpollEvent(m_pCurrentProcLinkNode);
					m_pHqLinkNodeMange->RemoveInvalidNode(m_pCurrentProcLinkNode->GetNodeIndex());
				}
			}
		}
	}

	return 0 == nEventNum ? -1 : 0;
}

// 处理死链接
int
NetThread::ProcessDeadLink()
{
	time_t NowTime = GetNowTime();
	if (DiffTimeSecond(NowTime, m_LastCheckHqDeadLinkTimer) < CHECK_HQ_LINK_ALIVE_TIME)
	{
		return -1;
	}
	m_LastCheckHqDeadLinkTimer = NowTime;
	RemoveDeadHqLink();
	return 0;
}

// 处理新的连接请求
int
NetThread::ProcessNewHqLink(const struct epoll_event *CurrentEvent) {
	int fd;
	struct sockaddr_in sockAddr;
	socklen_t nAddrLen;

	if (NULL != CurrentEvent) {
		while (1) {
			nAddrLen = sizeof(sockAddr);
			fd = accept(m_iListenServerFd, (struct sockaddr*)&sockAddr, &nAddrLen);
			if (fd < 0) {
				if(errno == EAGAIN) {
					return 0;
				}
				return -1;
			}

			// 比较连接数是否已经达到设定的最大值
			if (m_pHqLinkNodeMange->GetUsedNodeNum() >= GetMaxLinkedHq())
			{
				ERROR("Can not allow more connection.");
				Close(fd);
				continue;
			}

			DEBUG("Accept new user link [Add=%s Fd=%d]", inet_ntoa(sockAddr.sin_addr), fd);
			SetNoBlockMode(fd, true);

			if (!AddHqLink(fd, inet_ntoa(sockAddr.sin_addr)))
			{
				DEBUG("AddHqLink failed");
				Close(fd);
			}
		}
	}

	return 0;
}


// 处理已连接行情的请求
int
NetThread::ProcessLinkedHqRequest(const struct epoll_event *CurrentEvent) {
	if (NULL != CurrentEvent) {
		unsigned int iNodeIndex = (unsigned int)CurrentEvent->data.u64;
		HqLinkNode *pLinkNode = m_pHqLinkNodeMange->GetUsedNode(iNodeIndex);
		if (NULL == pLinkNode) {
			DEBUG("pLinkNode null error.");
			return -1;
		}

		if (!pLinkNode->CanReceive())
		{
			DEBUG("Force close node[ip=%s fd=%d index=%d markid=%s]", pLinkNode->GetIp().c_str(),
				pLinkNode->GetFd(), pLinkNode->GetNodeIndex(), pLinkNode->GetMarkID().c_str());
			DelEpollEvent(pLinkNode);
			m_pHqLinkNodeMange->RemoveInvalidNode(pLinkNode->GetNodeIndex());
			return -2;
		}

		int iRes = pLinkNode->NodeReceiveProcess();
		if (iRes < 0 && HQLK_CLOSE == pLinkNode->GetLinkState())
		{
			DEBUG("Delete node[%d]", pLinkNode->GetNodeIndex());
			DelEpollEvent(pLinkNode);
			m_pHqLinkNodeMange->RemoveInvalidNode(pLinkNode->GetNodeIndex());
			return -3;
		}
		m_pCurrentProcLinkNode = pLinkNode;
	}

	return 0;
}

// 修改epoll的event
bool
NetThread::ModifyEpollEvent(const HqLinkNode *CurrentNode,
		unsigned int EventValue) {
	if (NULL != CurrentNode) {
		m_ExNetEvent.data.u64 = ((HqLinkNode*)CurrentNode)->GetFd();
		m_ExNetEvent.data.u64 <<= 32;
		m_ExNetEvent.data.u64 |= ((HqLinkNode*)CurrentNode)->GetNodeIndex();
		m_ExNetEvent.events = EventValue;

		if(epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, CurrentNode->GetFd(), &m_ExNetEvent) < 0) {
			return false;
		}

		return true;
	}
	return false;
}

// 移除epoll的event
bool
NetThread::DelEpollEvent(const HqLinkNode *CurrentNode)
{
	if (NULL != CurrentNode) {
		if(epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, CurrentNode->GetFd(), &m_ExNetEvent) < 0) {
			TRACE("EPOLL_CTL_DEL failed.");
			return false;
		}
	}
	return true;
}

// 发送数据
int
NetThread::ProcessSendData(const struct epoll_event* CurrentEvent) {
	int iRes = -1;
	if (NULL != CurrentEvent) {
		unsigned int iNodeIndex = (unsigned int)CurrentEvent->data.u64;
		HqLinkNode *pLinkNode = m_pHqLinkNodeMange->GetUsedNode(iNodeIndex);
		if (NULL == pLinkNode) {
			DEBUG("pLinkNode null error.");
			return -1;
		}

		if (!pLinkNode->CanSend())
		{
			if (HQLK_CLOSE == pLinkNode->GetLinkState())
			{
				DEBUG("Link in error Remove Node");
				DelEpollEvent(pLinkNode);
				m_pHqLinkNodeMange->RemoveInvalidNode(pLinkNode->GetNodeIndex());
				return -2;
			}
		}

		iRes = pLinkNode->SendLinkCacheData();
		if (0 == iRes) {
			DEBUG("Send failed, close the socket connect[%d]", iRes);
			DelEpollEvent(pLinkNode);
			m_pHqLinkNodeMange->RemoveInvalidNode(pLinkNode->GetNodeIndex());
			return -3;
		}
		else {
			m_pCurrentProcLinkNode = pLinkNode;

			if (0 > iRes) {
				return -4;
			}
		}
	}

	return 0;
}

// 移除死链
void
NetThread::RemoveDeadHqLink()
{
	if (NULL != m_pHqLinkNodeMange)
	{
		m_pHqLinkNodeMange->RemoveDeadLinkHq();
	}
}

// 添加Hq请求链接
bool
NetThread::AddHqLink(const DWORD fd, const char *ip) {
	if (NULL == m_pHqLinkNodeMange)
	{
		return false;
	}
	HqLinkNode item(fd, ip);
	int iIndex = -1;
	NodeAddResType ResType = m_pHqLinkNodeMange->ConfigNewNodeRes(iIndex);
	if (NA_FULL == ResType)
	{
		FATAL("!!!!!!!!HqLinkNodeMange is full.!!!!!!!!!!");
		return false;
	}
	else if (NA_ERROR == ResType)
	{
		WARN("error state, canceled add HqLinkNode.");
		return false;
	}

	m_pHqLinkNodeMange->AddNewNode(item, iIndex);
	DEBUG("Mark index = %d", iIndex);
	if (0 > iIndex)
	{
		ERROR("AddHqLink failed [res=%d]", iIndex);
		return false;
	}
	HqLinkNode *pHqLinkNode = m_pHqLinkNodeMange->GetUsedNode((unsigned int)iIndex);
	m_pCurrentProcLinkNode = pHqLinkNode;
	if (NULL != pHqLinkNode)
	{
		pHqLinkNode->SetNodeIndex((unsigned int)iIndex);
	}

	// 注册epoll
	m_ExNetEvent.data.u64 = fd;
	m_ExNetEvent.data.u64 <<= 32;
	m_ExNetEvent.data.u64 |= iIndex;
	m_ExNetEvent.events = EPOLLIN | EPOLLERR;
	if(epoll_ctl(m_iEpollFd, EPOLL_CTL_ADD, fd, &m_ExNetEvent) < 0) {
		DEBUG("epoll_ctl failed");
		return false;
	}

	return true;
}

// 移除已经断开的LinkNode
bool
NetThread::RemoveCloseLinkNode(const HqLinkNode *CurrentNode)
{
	bool bRes = DelEpollEvent(CurrentNode);
	m_pHqLinkNodeMange->RemoveInvalidNode(((HqLinkNode*)CurrentNode)->GetNodeIndex());
	return bRes;
}

// 置位发送数据
bool
NetThread::SetLinkNodeSendFlag(const HqLinkNode *CurrentNode)
{
	if (NULL != CurrentNode && CurrentNode->CanSend())
	{
		return ModifyEpollEvent(CurrentNode, EPOLLOUT | EPOLLERR);
	}

	return false;
}

// 执行初始化
int
NetThread::ExecuteInitial()
{
	if (!CanResetNow())
	{
		return -1;
	}

	// 初始化日志
	int iFd = -1;
	const LogControlTag *pLogControl = GetLogControl();
	ReleaseLog();
	if (!InitialLog((1 == pLogControl->LogRoute ? NULL : &iFd), pLogControl->LogLevel)) {
		printf("InitialLog failed!\n");
		return 0;
	}

	// 初始化跟踪用户日志
	LogUserClose();
	LogUserInit("log_user_list.txt");

	// 置位已经初始化
	m_bHasInitialed = true;
	return 0;
}

// 获取指定的HqLinkNode
HqLinkNode*
NetThread::GetHqLinkNode(const unsigned short NodeIndex)
{
	return m_pHqLinkNodeMange->GetUsedNode(NodeIndex);
}

// 处理消息队列
int
NetThread::ProcedureMsgQueue()
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
				case UPD_DYNA_THREAD_REQUEST_RESET:
					{
						DEBUG("Send UPD_DYNA_THREAD_REQUEST_RESET");
						SetSignal(NSP_HANGUP_REQUEST, true);
						ThreadsManage::GetUpdDataThread()->SendMsg(UPD_DYNA_THREAD_CONFIRM_RESET, GetThreadKey(), -1);
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

	return -1 == iCount ? -1 : 0;
}



