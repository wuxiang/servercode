
#include "ThreadsManage.h"
#include "../../util/log/log.h"
#include "../../controller/runtime.h"
#include "NetThread.h"
#include "CalOperatingThread.h"
#include "UpdateDynaDataThread.h"
#include "ReqDynaNewsThread.h"
#include "SendOuterDataThread.h"
#include "../model/template/NodeThreadMutex.h"
#include "../model/template/ContainerCmp.h"
#include <map>

using namespace std;

// 成员变量定义
const unsigned char uMaxKeyLen = 32;								// 最大key长度
ThreadImpPtr *sThreadImpList = NULL;								// 线程指针数组
static int sHaveStartedCount = 0;									// 已经启动的线程数
CNodeThreadMutex *sNodeThreadMutex = NULL;							// 节点操作保护
map<string, const unsigned int> sThreadMap;							// 线程管理
///----------------------------------			/


// 创建所有线程
bool
ThreadsManage::CreateAllThread()
{
	char strKey[uMaxKeyLen] = {0};
	sThreadMap.clear();
	
	// 最大线程数
	int iMaxThread = GetTotoalThreadNum();
	sThreadImpList = new ThreadImpPtr[iMaxThread];
	memset(sThreadImpList, 0, iMaxThread);
	
	// 分配各类线程
	int iCalThread = GetCalcThreadNum();
	sNodeThreadMutex = new CNodeThreadMutex(iCalThread);
	unsigned int iPtIndex = 0;
	
	// 创建计算线程
	for (int i = 0; i < iCalThread; i++)
	{
		sThreadImpList[iPtIndex] = new CalOperatingThread();
		if (NULL == sThreadImpList[iPtIndex])
		{
			FATAL("Create CalOperatingThread error.");
			delete sThreadImpList[iPtIndex];
			sThreadImpList[iPtIndex] = NULL;
			sHaveStartedCount = iPtIndex;
			return false;
		}
		
		snprintf(strKey, uMaxKeyLen, "CalcThread_%d", i);
		sThreadMap.insert(pair<string, const unsigned int>(strKey, iPtIndex));
		sThreadImpList[iPtIndex]->SetThreadKey(strKey);
		iPtIndex++;
	}
	sHaveStartedCount = iPtIndex;
	
	// 创建行情相关动态数据更新线程
	sThreadImpList[iPtIndex] = new CUpdateDynaDataThread();
	if (NULL == sThreadImpList[iPtIndex])
	{
		FATAL("Create ReqDynaHqThread error.");
		delete sThreadImpList[iPtIndex];
		sThreadImpList[iPtIndex] = NULL;
		return false;
	}
	snprintf(strKey, uMaxKeyLen, "%s", "UpdDynaThread");
	sThreadMap.insert(pair<string, const unsigned int>(strKey, iPtIndex));
	sThreadImpList[iPtIndex]->SetThreadKey(strKey);
	iPtIndex++;
	sHaveStartedCount = iPtIndex;
	
	// 创建资讯新闻相关动态数据更新线程
	sThreadImpList[iPtIndex] = new ReqDynaNewsThread();
	if (NULL == sThreadImpList[iPtIndex])
	{
		FATAL("Create ReqDynaNewsThread error.");
		delete sThreadImpList[iPtIndex];
		sThreadImpList[iPtIndex] = NULL;
		return false;
	}
	snprintf(strKey, uMaxKeyLen, "%s", "ReqNewsThread");
	sThreadMap.insert(pair<string, const unsigned int>(strKey, iPtIndex));
	sThreadImpList[iPtIndex]->SetThreadKey(strKey);
	iPtIndex++;
	sHaveStartedCount = iPtIndex;
	
	// 创建外部数据发送线程
	sThreadImpList[iPtIndex] = new CSendOuterDataThread();
	if (NULL == sThreadImpList[iPtIndex])
	{
		FATAL("Create SendOuterDataThread error.");
		delete sThreadImpList[iPtIndex];
		sThreadImpList[iPtIndex] = NULL;
		return false;
	}
	snprintf(strKey, uMaxKeyLen, "%s", "SendOuterThread");
	sThreadMap.insert(pair<string, const unsigned int>(strKey, iPtIndex));
	sThreadImpList[iPtIndex]->SetThreadKey(strKey);
	iPtIndex++;
	sHaveStartedCount = iPtIndex;
	
	// 创建网络线程(一定放到最后一个启动，作为主线程启动运行)
	sThreadImpList[iPtIndex] = new NetThread();
	if (NULL == sThreadImpList[iPtIndex])
	{
		FATAL("Create NetThread error.");
		delete sThreadImpList[iPtIndex];
		sThreadImpList[iPtIndex] = NULL;
		return false;
	}
	snprintf(strKey, uMaxKeyLen, "%s", "NetThread");
	sThreadMap.insert(pair<string, const unsigned int>(strKey, iPtIndex));
	sThreadImpList[iPtIndex]->SetThreadKey(strKey);
	iPtIndex++;
	sHaveStartedCount = iPtIndex;
	
	return true;
}

// 启动所有线程
bool 
ThreadsManage::StartAllThread()
{
	
	if (NULL == sThreadImpList)
	{
		return false;
	}
	
	for (int i = 0; i < sHaveStartedCount; i++) {
		if (NULL == sThreadImpList[i] 
			|| !sThreadImpList[i]->InitialThreadEnv()
			|| 0 != sThreadImpList[i]->StartThread())
		{
			FATAL("Start Thread[%d] error.", i);
			return false;
		}
	}
	
	return true;
}

// 退出
void 
ThreadsManage::ExitMange() 
{
	if (NULL == sThreadImpList)
	{
		return;
	}
	
	for (int i = 0; i < sHaveStartedCount; i++) {
		if (NULL != sThreadImpList[i]) {
			sThreadImpList[i]->ExitThread(0);			
		}
	}
	
	usleep(1000 * 700);
	for (int i = 0; i < sHaveStartedCount; i++)
	{
		if (NULL != sThreadImpList[i])
		{
			delete sThreadImpList[i];
			sThreadImpList[i] = NULL;
		}
	}
	
	if (NULL != sThreadImpList) {
		delete []sThreadImpList;
	}
	
	if (NULL != sNodeThreadMutex)
	{
		delete sNodeThreadMutex;
		sNodeThreadMutex = NULL;
	}
	
	sThreadMap.clear();
}

// 获取线程
ThreadImp* 
ThreadsManage::GetThread(const char *key)
{
	map<string, const unsigned int>::const_iterator findInterator = sThreadMap.find(key);
	if (sThreadMap.end() == findInterator)
	{
		return NULL;
	}
	
	unsigned int iMaxThread = GetTotoalThreadNum();
	if (findInterator->second >= iMaxThread)
	{
		return NULL;
	}
	
	return (sThreadImpList[findInterator->second]);
}

// 获取指定索引的计算线程
CalOperatingThread* 
ThreadsManage::GetIndexCalcThread(const int index) {
	char strKey[uMaxKeyLen] = {0};
	snprintf(strKey, uMaxKeyLen, "CalcThread_%d", index);
	
	return (CalOperatingThread*)GetThread(strKey);
}

// 通知所有线程运行
void 
ThreadsManage::SendAllThreadRun()
{
	if (NULL == sThreadImpList)
	{
		return;
	}
	
	for (int i = 0; i < sHaveStartedCount; i++) {
		if (NULL != sThreadImpList[i]) {
			sThreadImpList[i]->SetThreadRunState(Thread_Running);
		}
	}
}

// 获取网络线程
NetThread*
ThreadsManage::GetNetThread()
{
	const char *pKey = "NetThread";
	return (NetThread*)GetThread(pKey);
}

// 获取资源控制
CNodeThreadMutex* 
ThreadsManage::GetNodeThreadMutexManage()
{
	return sNodeThreadMutex;
}

// 获取外部发送线程
CSendOuterDataThread* 
ThreadsManage::GetOutSendThread()
{
	const char *pKey = "SendOuterThread";
	return (CSendOuterDataThread*)GetThread(pKey);
}

// 获取请求数据线程
CUpdateDynaDataThread* 
ThreadsManage::GetUpdDataThread()
{
	const char *pKey = "UpdDynaThread";
	return (CUpdateDynaDataThread*)GetThread(pKey);
}

// 挂起请求(除ReqDynaNewsThread外)
void 
ThreadsManage::HangupOtherRequest(ThreadImp *pCallThread, bool hang)
{
	if (NULL == sThreadImpList || NULL == pCallThread)
	{
		return;
	}
	
	for (int i = 0; i < sHaveStartedCount; i++) {
		if (NULL != sThreadImpList[i]) {
			if (0 == strncmp(sThreadImpList[i]->GetThreadKey(), pCallThread->GetThreadKey(), uMaxKeyLen))
			{
				continue;
			}
			sThreadImpList[i]->SendMsg(UPD_HUNGUP_THREAD_PROC, pCallThread->GetThreadKey(), hang);
		}
	}
}

