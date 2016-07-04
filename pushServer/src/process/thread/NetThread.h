/*****
** 网络线程
** 用以处理连接
********************************/
#ifndef _NET_THREAD_H
#define _NET_THREAD_H

#include "ThreadImp.h"
#include "../../util/util.h"
#include "../../util/common/common_types.h"
#include "../model/hqlink/HqLinkNode.h"
#include <sys/epoll.h>

class CHqLinkManage;
class CPushUserManage;
class HqLinkNode;


class NetThread : public ThreadImp {
public:
	NetThread();
	~NetThread();
	
private:
	const unsigned int m_uListenIndex;							// 监听索引
	struct epoll_event *m_pNetEvents;							// 全部NET事件监听
	struct epoll_event m_ExNetEvent;							// 监听Net事件交换
	int m_iEpollFd;												// epoll描述符
	int m_iListenServerFd;										// 监听的服务socket fd
	CHqLinkManage *m_pHqLinkNodeMange;							// Hq链接管理
	time_t m_LastCheckHqDeadLinkTimer;							// 最近一次执行检查行情死链的时间
	HqLinkNode *m_pCurrentProcLinkNode;							// 当前处理的linknode

protected:
	bool InitialThreadEnv();									// 初始线程环境
	int StartThread();											// 启动线程
	static void* ThreadProc(void*);								// 线程处理函数
	
private:
	bool InitialNetListen();									// 初始化监听
	int ProcessRequest();										// 处理网络请求
	int ProcessLinkedHqRequest(const struct epoll_event*);		// 处理已连接行情的请求
	bool ModifyEpollEvent(const HqLinkNode*, unsigned int);		// 修改epoll的event
	bool DelEpollEvent(const HqLinkNode*);						// 移除epoll的event
	int ProcessSendData(const struct epoll_event*);				// 发送数据
	
	int ProcessDeadLink();										// 处理死链接
	int ProcessNewHqLink(const struct epoll_event*);			// 处理新的行情连接请求
	bool AddHqLink(const DWORD, const char*);					// 添加Hq请求链接(NetThread调用)
	void RemoveDeadHqLink();									// 移除行情死链(NetThread调用)
	
	int ExecuteInitial();										// 执行初始化
	int ProcedureMsgQueue();									// 处理消息队列
	
public:
	bool RemoveCloseLinkNode(const HqLinkNode*);				// 移除已经断开的LinkNode
	bool SetLinkNodeSendFlag(const HqLinkNode*);				// 置位发送数据
	HqLinkNode* GetHqLinkNode(const unsigned short);			// 获取指定的HqLinkNode
};


#endif  /* _NET_THREAD_H */
