/*****
** �����߳�
** ���Դ�������
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
	const unsigned int m_uListenIndex;							// ��������
	struct epoll_event *m_pNetEvents;							// ȫ��NET�¼�����
	struct epoll_event m_ExNetEvent;							// ����Net�¼�����
	int m_iEpollFd;												// epoll������
	int m_iListenServerFd;										// �����ķ���socket fd
	CHqLinkManage *m_pHqLinkNodeMange;							// Hq���ӹ���
	time_t m_LastCheckHqDeadLinkTimer;							// ���һ��ִ�м������������ʱ��
	HqLinkNode *m_pCurrentProcLinkNode;							// ��ǰ�����linknode

protected:
	bool InitialThreadEnv();									// ��ʼ�̻߳���
	int StartThread();											// �����߳�
	static void* ThreadProc(void*);								// �̴߳�����
	
private:
	bool InitialNetListen();									// ��ʼ������
	int ProcessRequest();										// ������������
	int ProcessLinkedHqRequest(const struct epoll_event*);		// �������������������
	bool ModifyEpollEvent(const HqLinkNode*, unsigned int);		// �޸�epoll��event
	bool DelEpollEvent(const HqLinkNode*);						// �Ƴ�epoll��event
	int ProcessSendData(const struct epoll_event*);				// ��������
	
	int ProcessDeadLink();										// ����������
	int ProcessNewHqLink(const struct epoll_event*);			// �����µ�������������
	bool AddHqLink(const DWORD, const char*);					// ���Hq��������(NetThread����)
	void RemoveDeadHqLink();									// �Ƴ���������(NetThread����)
	
	int ExecuteInitial();										// ִ�г�ʼ��
	int ProcedureMsgQueue();									// ������Ϣ����
	
public:
	bool RemoveCloseLinkNode(const HqLinkNode*);				// �Ƴ��Ѿ��Ͽ���LinkNode
	bool SetLinkNodeSendFlag(const HqLinkNode*);				// ��λ��������
	HqLinkNode* GetHqLinkNode(const unsigned short);			// ��ȡָ����HqLinkNode
};


#endif  /* _NET_THREAD_H */
