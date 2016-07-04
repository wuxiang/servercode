#include "PushMsgHistoryManage.h"
#include "../../../util/log/log.h"
#include "../../../controller/runtime.h"
#include "../../../util/common/common_file.h"

// 静态成员初始化
CPushMsgHistory *CPushMsgHistoryManage::m_pAndroidMsgHis = NULL;
CPushMsgHistory *CPushMsgHistoryManage::m_pIosMsgHis = NULL;
CPushMsgHistory *CPushMsgHistoryManage::m_pWp7MsgHis = NULL;
CPushMsgHistory *CPushMsgHistoryManage::m_pWin8MsgHis = NULL;

CPushMsgHistoryManage::CPushMsgHistoryManage()
{
}

CPushMsgHistoryManage::~CPushMsgHistoryManage()
{
}

// 初始化资源
const int
CPushMsgHistoryManage::CreateRes()
{
	char PathName[MAX_PATH_LEN] = {0};
	// 创建目录
	char *pOutPutDir = "./data/history";
	if (!IsDir(pOutPutDir))
	{
		MkDir(pOutPutDir);
	}
	
	const size_t RecordLen = sizeof(PushMsg);
	const PlatformUserConfig *pUserConfig = GetAndroidUserConfig();
	if (NULL == m_pAndroidMsgHis)
	{
		snprintf(PathName, MAX_PATH_LEN, "%s/%s", pOutPutDir, pUserConfig->MsgHisFileName);
		m_pAndroidMsgHis = new CPushMsgHistory(RecordLen, pUserConfig->MaxEWarningHisNum,
						pUserConfig->MaxInfoHisNum, pUserConfig->MaxNoticeHisNum,
						pUserConfig->MaxUser, PathName);
	}
	if (NULL == m_pAndroidMsgHis)
	{
		return -1;
	}
	
	pUserConfig = GetIosUserConfig();
	if (NULL == m_pIosMsgHis)
	{
		snprintf(PathName, MAX_PATH_LEN, "%s/%s", pOutPutDir, pUserConfig->MsgHisFileName);
		m_pIosMsgHis = new CPushMsgHistory(RecordLen, pUserConfig->MaxEWarningHisNum,
						pUserConfig->MaxInfoHisNum, pUserConfig->MaxNoticeHisNum,
						pUserConfig->MaxUser, PathName);
	}
	if (NULL == m_pIosMsgHis)
	{
		return -2;
	}
	
	pUserConfig = GetWp7UserConfig();
	if (NULL == m_pWp7MsgHis)
	{
		snprintf(PathName, MAX_PATH_LEN, "%s/%s", pOutPutDir, pUserConfig->MsgHisFileName);
		m_pWp7MsgHis = new CPushMsgHistory(RecordLen, pUserConfig->MaxEWarningHisNum,
						pUserConfig->MaxInfoHisNum, pUserConfig->MaxNoticeHisNum,
						pUserConfig->MaxUser, PathName);
	}
	if (NULL == m_pWp7MsgHis)
	{
		return -3;
	}
	
	pUserConfig = GetWin8UserConfig();
	if (NULL == m_pWin8MsgHis)
	{
		snprintf(PathName, MAX_PATH_LEN, "%s/%s", pOutPutDir, pUserConfig->MsgHisFileName);
		m_pWin8MsgHis = new CPushMsgHistory(RecordLen, pUserConfig->MaxEWarningHisNum,
						pUserConfig->MaxInfoHisNum, pUserConfig->MaxNoticeHisNum,
						pUserConfig->MaxUser, PathName);
	}
	if (NULL == m_pWin8MsgHis)
	{
		return -4;
	}
	
	return 0;
}

// 初始化
bool 
CPushMsgHistoryManage::InitialMsgHis()
{
	int iRes = CreateRes();
	if (iRes < 0)
	{
		DEBUG("CreateRes failed[%d]", iRes);
		return false;
	}
	
	iRes = m_pAndroidMsgHis->InitialLoadMsgHistory()
			* m_pIosMsgHis->InitialLoadMsgHistory()
			* m_pWp7MsgHis->InitialLoadMsgHistory()
			* m_pWin8MsgHis->InitialLoadMsgHistory();
	if (0 == iRes)
	{
		DEBUG("InitialLoadMsgHistory failed");
		return false;
	}
	
	return true;
}

// 释放资源
void
CPushMsgHistoryManage::ReleaseMsgHis()
{
	if (NULL != m_pAndroidMsgHis)
	{
		m_pAndroidMsgHis->ReleaseRes();
		delete m_pAndroidMsgHis;
		m_pAndroidMsgHis = NULL;
	}
	
	if (NULL != m_pIosMsgHis)
	{
		m_pIosMsgHis->ReleaseRes();
		delete m_pIosMsgHis;
		m_pIosMsgHis = NULL;
	}
	
	if (NULL != m_pWp7MsgHis)
	{
		m_pWp7MsgHis->ReleaseRes();
		delete m_pWp7MsgHis;
		m_pWp7MsgHis = NULL;
	}
	
	if (NULL != m_pWin8MsgHis)
	{
		m_pWin8MsgHis->ReleaseRes();
		delete m_pWin8MsgHis;
		m_pWin8MsgHis = NULL;
	}
}

// 获取指定平台的历史记录管理
CPushMsgHistory *
CPushMsgHistoryManage::GetPlatFormMsgHis(const int platformcode)
{
	CPushMsgHistory *pCurrentMsgHis = NULL;
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(platformcode);
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			pCurrentMsgHis = m_pWp7MsgHis;
			break;
		
		case PFCC_ANDROID:
			pCurrentMsgHis = m_pAndroidMsgHis;
			break;
			
		case PFCC_IOS:
			pCurrentMsgHis = m_pIosMsgHis;
			break;
			
		case PFCC_WIN8:
			pCurrentMsgHis = m_pWin8MsgHis;
			break;
			
		default:
			break;
	}
	return pCurrentMsgHis;
}

// 添加历史记录
const int 
CPushMsgHistoryManage::PushBackMsgHistory(const int platformcode, const size_t serial, const size_t MsgIndex, 
	const size_t HaveSendMark, const PushMsg &msg)
{
	CPushMsgHistory *pCurrentMsgHis = GetPlatFormMsgHis(platformcode);
	if (NULL == pCurrentMsgHis)
	{
		ERROR("platformcode[%d] not defined.", platformcode);
		return -3;
	}
	
	return pCurrentMsgHis->AddMsgHistory(serial, MsgIndex, HaveSendMark, msg);
}

// 获取指定索引指定指定类型的消息
const int 
CPushMsgHistoryManage::GetPushMsg(const int platformcode, const size_t serial, const int MsgType, 
	const size_t index, pPushMsg &pMsg)
{
	CPushMsgHistory *pCurrentMsgHis = GetPlatFormMsgHis(platformcode);
	if (NULL == pCurrentMsgHis)
	{
		ERROR("platformcode[%d] not defined.", platformcode);
		return -1;
	}
	
	int iRet = pCurrentMsgHis->GetPushMsg(serial, MsgType, index, pMsg);
	
	// 校验
	time_t tv = GetNowTime();
	if (iRet < 0 || pCurrentMsgHis->IsMsgOutofDate(pMsg, tv))
	{
		return -2;
	}
	
	return 0;
}

// 获取信息地雷Crc
const unsigned int
CPushMsgHistoryManage::GetUserHisMsgCrc(const int platformcode, const size_t serial, 
		const int MsgType, const size_t index)
{
	pPushMsg pMsg = NULL;
	CPushMsgHistory *pCurrentMsgHis = GetPlatFormMsgHis(platformcode);
	
	if (NULL == pCurrentMsgHis)
	{
		ERROR("platformcode[%d] not defined.", platformcode);
		return (unsigned int)-1;
	}
	int iRet = pCurrentMsgHis->GetPushMsg(serial, MsgType, index, pMsg);
	if (iRet < 0)
	{
		return (unsigned int)-1;
	}
	
	return pMsg->SettingRecordID;
}

// 清空节点的历史记录信息
bool
CPushMsgHistoryManage::ClearNodeMsgHistory(const int platformcode, const size_t serial)
{
	CPushMsgHistory *pCurrentMsgHis = GetPlatFormMsgHis(platformcode);
	if (NULL == pCurrentMsgHis)
	{
		ERROR("platformcode[%d] not defined.", platformcode);
		return -3;
	}
	
	return pCurrentMsgHis->ClearNodeMsgHistory(serial);
}

// 获取最大历史记录数
const short 
CPushMsgHistoryManage::GetMaxMsgCacheCount(const int platformcode, const int MsgType)
{
	CPushMsgHistory *pCurrentMsgHis = GetPlatFormMsgHis(platformcode);
	if (NULL == pCurrentMsgHis)
	{
		ERROR("platformcode[%d] not defined.", platformcode);
		return 0;
	}
	
	return (short)pCurrentMsgHis->GetMaxMsgCacheCount(MsgType);
}

