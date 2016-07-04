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
	UPD_DYNA_THREAD_REQUEST_RESET = 0x0A,					// �����߳���������
	UPD_DYNA_THREAD_CONFIRM_RESET,							// �����߳���������
	UPD_HQ_INDEX_RESET,										// Ҫ���������鶯̬��¼ָ��
	UPD_EXE_WARN,											// ��̬�����и��£�ִ�йɼ�Ԥ��
	UPD_EXE_INFOMINE,										// ��Ϣ�����и��£�ִ����Ϣ����ɨ��
	UPD_EXE_NOTICE,											// �����и��£�ִ�й���ɨ��
	UPD_EXE_INFOMINE_STATIS, 								// ִ����Ϣ����ͳ��
	UPD_HUNGUP_THREAD_PROC,									// �����̴߳���
	UPD_HUNGUP_THREAD_CONFIRM,								// �����̴߳���ȷ��
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
