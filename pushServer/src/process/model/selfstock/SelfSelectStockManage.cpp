#include "SelfSelectStockManage.h"
#include "SelfSelectStock.h"
#include "../../../util/util.h"
#include "../../../util/log/log.h"
#include "../../../util/common/common_lib.h"
#include "../../../util/time/time_util.h"
#include "../../../controller/runtime.h"
#include "../../../util/common/common_file.h"
#include "../../config.h"
#include "../../data_type.h"

CSelfSelectStock* CSelfSelectStockManage::m_pAndroidSelfStkManage = NULL;
CSelfSelectStock* CSelfSelectStockManage::m_pIosSelfStkManage = NULL;
CSelfSelectStock* CSelfSelectStockManage::m_pWp7SelfStkManage = NULL;
CSelfSelectStock* CSelfSelectStockManage::m_pWin8SelfStkManage = NULL;

CSelfSelectStockManage::CSelfSelectStockManage()
{
}

CSelfSelectStockManage::~CSelfSelectStockManage()
{
}

// 初始化资源
const int
CSelfSelectStockManage::CreateRes()
{
	char PathName[MAX_PATH_LEN] = {0};
	// 创建目录
	char *pOutPutDir = "./data/selfselectstk";
	if (!IsDir(pOutPutDir))
	{
		MkDir(pOutPutDir);
	}
	
	SelfSelectStockInfomineConfigT *pSelfStkConfig = GetSelfSelectStockInfomineConfig();
	
	const PlatformUserConfig *pUserConfig = GetAndroidUserConfig();
	if (NULL == m_pAndroidSelfStkManage)
	{
		snprintf(PathName, MAX_PATH_LEN, "%s/%u_%s", pOutPutDir, PFCC_ANDROID, pSelfStkConfig->FileName);
		m_pAndroidSelfStkManage = new CSelfSelectStock(pUserConfig->MaxUser, pSelfStkConfig->CacheCount, PathName);
	}
	if (NULL == m_pAndroidSelfStkManage)
	{
		return -1;
	}
	
	pUserConfig = GetIosUserConfig();
	if (NULL == m_pIosSelfStkManage)
	{
		snprintf(PathName, MAX_PATH_LEN, "%s/%u_%s", pOutPutDir, PFCC_IOS, pSelfStkConfig->FileName);
		m_pIosSelfStkManage = new CSelfSelectStock(pUserConfig->MaxUser, pSelfStkConfig->CacheCount, PathName);
	}
	if (NULL == m_pIosSelfStkManage)
	{
		return -2;
	}
	
	pUserConfig = GetWp7UserConfig();
	if (NULL == m_pWp7SelfStkManage)
	{
		snprintf(PathName, MAX_PATH_LEN, "%s/%u_%s", pOutPutDir, PFCC_WP7, pSelfStkConfig->FileName);
		m_pWp7SelfStkManage = new CSelfSelectStock(pUserConfig->MaxUser, pSelfStkConfig->CacheCount, PathName);
	}
	if (NULL == m_pWp7SelfStkManage)
	{
		return -3;
	}
	
	pUserConfig = GetWin8UserConfig();
	if (NULL == m_pWin8SelfStkManage)
	{
		snprintf(PathName, MAX_PATH_LEN, "%s/%u_%s", pOutPutDir, PFCC_WIN8, pSelfStkConfig->FileName);
		m_pWin8SelfStkManage = new CSelfSelectStock(pUserConfig->MaxUser, pSelfStkConfig->CacheCount, PathName);
	}
	if (NULL == m_pWin8SelfStkManage)
	{
		return -4;
	}
	
	return 0;
}

// 初始化
bool
CSelfSelectStockManage::InitialSelfStk()
{
	int iRes = CreateRes();
	if (iRes < 0)
	{
		DEBUG("CreateRes failed[%d]", iRes);
		return false;
	}
	
	iRes = m_pAndroidSelfStkManage->InitialLoad()
			* m_pIosSelfStkManage->InitialLoad()
			* m_pWp7SelfStkManage->InitialLoad()
			* m_pWin8SelfStkManage->InitialLoad();
	if (0 == iRes)
	{
		DEBUG("InitialLoad failed");
		return false;
	}
	
	return true;
}

// 释放资源
void
CSelfSelectStockManage::ReleaseSelfStk()
{
	if (NULL != m_pAndroidSelfStkManage)
	{
		m_pAndroidSelfStkManage->ReleaseRes();
		delete m_pAndroidSelfStkManage;
		m_pAndroidSelfStkManage = NULL;
	}
	
	if (NULL != m_pIosSelfStkManage)
	{
		m_pIosSelfStkManage->ReleaseRes();
		delete m_pIosSelfStkManage;
		m_pIosSelfStkManage = NULL;
	}
	
	if (NULL != m_pWp7SelfStkManage)
	{
		m_pWp7SelfStkManage->ReleaseRes();
		delete m_pWp7SelfStkManage;
		m_pWp7SelfStkManage = NULL;
	}
	
	if (NULL != m_pWin8SelfStkManage)
	{
		m_pWin8SelfStkManage->ReleaseRes();
		delete m_pWin8SelfStkManage;
		m_pWin8SelfStkManage = NULL;
	}
}

// 获取指定平台的记录管理
CSelfSelectStock*
CSelfSelectStockManage::GetPlatformManage(const int platformcode)
{
	CSelfSelectStock *pCurrentEWarn = NULL;
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(platformcode);
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			pCurrentEWarn = m_pWp7SelfStkManage;
			break;
		
		case PFCC_ANDROID:
			pCurrentEWarn = m_pAndroidSelfStkManage;
			break;
			
		case PFCC_IOS:
			pCurrentEWarn = m_pIosSelfStkManage;
			break;
			
		case PFCC_WIN8:
			pCurrentEWarn = m_pWin8SelfStkManage;
			break;
			
		default:
			break;
	}
	return pCurrentEWarn;
}

// 重置
void
CSelfSelectStockManage::ResetConfigMark()
{
	SelfSelectStockInfomineConfigT *pSelfSelectStockInfomineConfig = GetSelfSelectStockInfomineConfig();
	pSelfSelectStockInfomineConfig->LastestUpdStatistics = GetNowTime();
	pSelfSelectStockInfomineConfig->InfoMineStatisTimes = 0;
}

