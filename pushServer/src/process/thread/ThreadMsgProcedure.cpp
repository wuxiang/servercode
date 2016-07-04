#include "ThreadMsgProcedure.h"

using namespace std;


CThreadMsgProcedure::CThreadMsgProcedure(void)
{
	
}

CThreadMsgProcedure::~CThreadMsgProcedure(void)
{
	while(!IsEmpty())
	{
		m_MsgQueue.pop();
	}
	m_WaitList.clear();
}

bool 
CThreadMsgProcedure::IsEmpty()
{
	bool bRes = true;
	if (m_muCriticalArea.lock())
	{
		bRes = m_MsgQueue.empty();
		m_muCriticalArea.unlock();
	}
	
	return bRes;
}

const int
CThreadMsgProcedure::GetFront(CMsgMark &Msg)
{
	int iCount = -1;
	if (m_muCriticalArea.lock())
	{
		Msg = m_MsgQueue.front();
		iCount = m_MsgQueue.size();
		m_muCriticalArea.unlock();
	}
	
	return iCount;
}

void
CThreadMsgProcedure::PopFront()
{
	if (m_muCriticalArea.lock())
	{
		m_MsgQueue.pop();
		m_muCriticalArea.unlock();
	}
}

const int
CThreadMsgProcedure::SendMsg(const CMsgMark &Msg)
{
	int iCount = -1;
	if (m_muCriticalArea.lock())
	{
		m_MsgQueue.push(Msg);
		iCount = m_MsgQueue.size();
		m_muCriticalArea.unlock();
	}
	
	return iCount;
}

const int 
CThreadMsgProcedure::SendMsg(const unsigned short msg, const char *lParam, const short wParam)
{
	int iCount = -1;
	if (m_muCriticalArea.lock())
	{
		CMsgMark Msg(msg, lParam, wParam);
		m_MsgQueue.push(Msg);
		iCount = m_MsgQueue.size();
		m_muCriticalArea.unlock();
	}
	
	return iCount;
}

void
CThreadMsgProcedure::AddWaitFlag(const unsigned short ResponeMsgID)
{
	map<unsigned short, int>::iterator iterFind = m_WaitList.find(ResponeMsgID);
	if (m_WaitList.end() == iterFind)
	{
		m_WaitList.insert(pair<unsigned short, int>(ResponeMsgID, 1));
	}
	else
	{
		iterFind->second++;
	}
}

int
CThreadMsgProcedure::RemoveWaitFlag(const unsigned short ResponeMsgID)
{
	map<unsigned short, int>::iterator iterFind = m_WaitList.find(ResponeMsgID);
	if (m_WaitList.end() == iterFind)
	{
		return -1;
	}
	
	iterFind->second--;
	return iterFind->second;
}

void
CThreadMsgProcedure::ResetWaitFlag(const unsigned short ResponeMsgID)
{
	m_WaitList.erase(ResponeMsgID);
}

bool
CThreadMsgProcedure::UnderWait(const unsigned short ResponeMsgID)
{
	map<unsigned short, int>::iterator iterFind = m_WaitList.find(ResponeMsgID);
	if (m_WaitList.end() == iterFind)
	{
		return false;
	}
	
	return true;
}

