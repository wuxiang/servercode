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
	// 最大线程数
	int iMaxThread = GetTotoalThreadNum();
	if (iMaxThread < 5) {
		FATAL("iMaxThread error.");
		return false;
	}
	
	// 初始化线程
	if (!ThreadsManage::CreateAllThread())
	{
		FATAL("CreateAllThread error.");
		return false;
	}
	
	// 初始化用户数据
	if (!CPushUserManage::Initial())
	{
		FATAL("PushUserManage Initial failed.");
		return false;
	}
	
	// 初始化预警记录设置
	if (!CEwarningManage::InitialEWarn())
	{
		FATAL("EwarningManage InitialEWarn failed.");
		return false;
	}
	
	// 初始化消息历史记录
	if (!CPushMsgHistoryManage::InitialMsgHis())
	{
		FATAL("PushMsgHistoryManage InitialMsgHis error.");
		return false;
	}
	
	// 初始化自选股
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

