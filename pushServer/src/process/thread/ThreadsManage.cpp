
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

// ��Ա��������
const unsigned char uMaxKeyLen = 32;								// ���key����
ThreadImpPtr *sThreadImpList = NULL;								// �߳�ָ������
static int sHaveStartedCount = 0;									// �Ѿ��������߳���
CNodeThreadMutex *sNodeThreadMutex = NULL;							// �ڵ��������
map<string, const unsigned int> sThreadMap;							// �̹߳���
///----------------------------------			/


// ���������߳�
bool
ThreadsManage::CreateAllThread()
{
	char strKey[uMaxKeyLen] = {0};
	sThreadMap.clear();
	
	// ����߳���
	int iMaxThread = GetTotoalThreadNum();
	sThreadImpList = new ThreadImpPtr[iMaxThread];
	memset(sThreadImpList, 0, iMaxThread);
	
	// ��������߳�
	int iCalThread = GetCalcThreadNum();
	sNodeThreadMutex = new CNodeThreadMutex(iCalThread);
	unsigned int iPtIndex = 0;
	
	// ���������߳�
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
	
	// ����������ض�̬���ݸ����߳�
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
	
	// ������Ѷ������ض�̬���ݸ����߳�
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
	
	// �����ⲿ���ݷ����߳�
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
	
	// ���������߳�(һ���ŵ����һ����������Ϊ���߳���������)
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

// ���������߳�
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

// �˳�
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

// ��ȡ�߳�
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

// ��ȡָ�������ļ����߳�
CalOperatingThread* 
ThreadsManage::GetIndexCalcThread(const int index) {
	char strKey[uMaxKeyLen] = {0};
	snprintf(strKey, uMaxKeyLen, "CalcThread_%d", index);
	
	return (CalOperatingThread*)GetThread(strKey);
}

// ֪ͨ�����߳�����
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

// ��ȡ�����߳�
NetThread*
ThreadsManage::GetNetThread()
{
	const char *pKey = "NetThread";
	return (NetThread*)GetThread(pKey);
}

// ��ȡ��Դ����
CNodeThreadMutex* 
ThreadsManage::GetNodeThreadMutexManage()
{
	return sNodeThreadMutex;
}

// ��ȡ�ⲿ�����߳�
CSendOuterDataThread* 
ThreadsManage::GetOutSendThread()
{
	const char *pKey = "SendOuterThread";
	return (CSendOuterDataThread*)GetThread(pKey);
}

// ��ȡ���������߳�
CUpdateDynaDataThread* 
ThreadsManage::GetUpdDataThread()
{
	const char *pKey = "UpdDynaThread";
	return (CUpdateDynaDataThread*)GetThread(pKey);
}

// ��������(��ReqDynaNewsThread��)
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

