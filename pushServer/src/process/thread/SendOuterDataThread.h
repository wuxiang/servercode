#ifndef _SEND_OUTER_DATA_THREAD_H
#define _SEND_OUTER_DATA_THREAD_H

#include "ThreadImp.h"
#include "../model/template/ThreadMutex.h"
#include <queue>
#include <map>

class CPlatFormSendIntegrate;
class COuterSendMark;

class CSendOuterDataThread : public ThreadImp
{
public:
	CSendOuterDataThread();
	~CSendOuterDataThread();
	
private:
	std::map<unsigned short, CPlatFormSendIntegrate*> m_SendIntegrate;
	std::queue<COuterSendMark> m_SendQueue;						// 发送数据队列
	CThreadMutex m_muCriticalArea;								// 保护区
	
private:
	bool InitialOuterSendManage();								// 初始化发送管理
	int ProcessSendMark(COuterSendMark&);						// 处理发送
	int ProcessSend();											// 发送缓存的内容
	CPlatFormSendIntegrate* GetPlatformSendInte(				// 获取发送平台管理
			const unsigned char);
	int ProcedureMsgQueue();									// 处理消息队列
	
protected:
	bool InitialThreadEnv();									// 初始线程环境
	int StartThread();											// 启动线程
	static void* ThreadProc(void*);								// 线程处理函数

public:
	int PushToSendQueue(const unsigned char, 					// 存入发送队列(0成功 -1失败 1已满)
			const unsigned int, const unsigned char);
	int UserExecuteSend(const unsigned char,					// 用户执行发送
			const char*, const void*);
};

#endif  /* _SEND_OUTER_DATA_THREAD_H */
