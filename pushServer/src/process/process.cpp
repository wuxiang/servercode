#include "process.h"
#include "../util/util.h"
#include "../util/log/log.h"
#include "../controller/runtime.h"
#include "thread/ThreadsManage.h"
#include "model/history/PushMsgHistoryManage.h"
#include "model/db/DatabaseSnapshot.h"
#include "model/db/PublicNews.h"
#include "model/db/InfoMine.h"
#include "model/pushuser/PushUserManage.h"
#include "model/ewarning/EarlyWarningManage.h"
#include "model/selfstock/SelfSelectStockManage.h"

bool 
ExecuteGuideServer(void) {
	// ����߳���
	int iMaxThread = GetTotoalThreadNum();
	if (iMaxThread < 5) {
		FATAL("iMaxThread error.");
		return false;
	}
	
	// ��ʼ���߳�
	if (!ThreadsManage::CreateAllThread())
	{
		FATAL("CreateAllThread error.");
		return false;
	}
	
	// ��ʼ���û�����
	if (!CPushUserManage::Initial())
	{
		FATAL("PushUserManage Initial failed.");
		return false;
	}
	
	// ��ʼ��Ԥ����¼����
	if (!CEwarningManage::InitialEWarn())
	{
		FATAL("EwarningManage InitialEWarn failed.");
		return false;
	}
	
	// ��ʼ����Ϣ��ʷ��¼
	if (!CPushMsgHistoryManage::InitialMsgHis())
	{
		FATAL("PushMsgHistoryManage InitialMsgHis error.");
		return false;
	}
	
	// ��ʼ����ѡ��
	if (!CSelfSelectStockManage::InitialSelfStk())
	{
		FATAL("SelfSelectStockManage InitialSelfStk error.");
		return false;
	}

	if (!ThreadsManage::StartAllThread())
	{
		FATAL("StartAllThread error.");
		return false;
	}
	
	return true;
}

void 
ExitServerInstance() {
	ThreadsManage::ExitMange();
	CPushUserManage::Release();
	CEwarningManage::ReleaseEWarn();
	CPushMsgHistoryManage::ReleaseMsgHis();
	DatabaseSnapshot::Release();
	CPublicNews::releasePublicNewsPointer();
	CInfoMine::releaseInfoMinePointer();
	CSelfSelectStockManage::ReleaseSelfStk();
}

