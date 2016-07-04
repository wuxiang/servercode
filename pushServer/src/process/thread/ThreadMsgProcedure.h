#ifndef _INCLUDE_THREAD_MSG_PROCEDURE_H
#define _INCLUDE_THREAD_MSG_PROCEDURE_H

#include "../model/template/ThreadMutex.h"
#include <queue>
#include <map>

static const int Max_Parm_Len = 32;
struct CMsgMark
{
	unsigned short m_Msg;
	char m_lParam[Max_Parm_Len];
	short m_wParam;

public:
	CMsgMark()
	{
		m_Msg = 0xFFFF;
		memset(m_lParam, 0, Max_Parm_Len);
		m_wParam = -1;
	}

	CMsgMark(const unsigned short msg, const char *lParam, const short wParam)
	{
		m_Msg = msg;
		if (NULL != lParam)
		{
			memcpy(m_lParam, lParam, Max_Parm_Len);
		}
		m_wParam = wParam;
	}

	CMsgMark& operator=(const CMsgMark &item)
	{
		m_Msg = item.m_Msg;
		if (NULL != item.m_lParam)
		{
			memcpy(m_lParam, item.m_lParam, Max_Parm_Len);
		}
		m_wParam = item.m_wParam;
		return *this;
	}
};

enum ThreadMsgID
{
	UPD_DYNA_THREAD_REQUEST_RESET = 0x0A,					// 行情线程请求重启
	UPD_DYNA_THREAD_CONFIRM_RESET,							// 行情线程允许重启
	UPD_HQ_INDEX_RESET,										// 要求重置行情动态记录指针
	UPD_EXE_WARN,											// 动态行情有更新，执行股价预警
	UPD_EXE_INFOMINE,										// 信息地雷有更新，执行信息地雷扫描
	UPD_EXE_NOTICE,											// 公告有更新，执行公告扫描
	UPD_EXE_INFOMINE_STATIS, 								// 执行信息地雷统计
	UPD_HUNGUP_THREAD_PROC,									// 挂起线程处理
	UPD_HUNGUP_THREAD_CONFIRM,								// 挂起线程处理确认
};

class CThreadMsgProcedure
{
public:
	CThreadMsgProcedure(void);
	virtual ~CThreadMsgProcedure(void);

private:
	std::queue<CMsgMark> m_MsgQueue;
	std::map<unsigned short, int> m_WaitList;
	CThreadMutex m_muCriticalArea;

public:
	bool IsEmpty();
	const int GetFront(CMsgMark &Msg);
	void PopFront();
	const int SendMsg(const CMsgMark &Msg);
	const int SendMsg(const unsigned short msg, const char *lParam, const short wParam);
	void AddWaitFlag(const unsigned short ResponeMsgID);
	int RemoveWaitFlag(const unsigned short ResponeMsgID);
	void ResetWaitFlag(const unsigned short ResponeMsgID);
	bool UnderWait(const unsigned short ResponeMsgID);
	virtual int ProcedureMsgQueue() = 0;
};

#endif	/* _INCLUDE_THREAD_MSG_PROCEDURE_H */
