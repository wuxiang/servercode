#include "PushUserManage.h"
#include "PushUser.h"
#include "AndroidPushUser.h"
#include "IphonePushUser.h"
#include "Wp7PushUser.h"
#include "AndroidPushUserManage.h"
#include "IosPushUserManage.h"
#include "Wp7PushUserManage.h"
#include "Win8PushUser.h"
#include "Win8PushUserManage.h"
#include "../../../util/log/log.h"
#include "../../../util/time/time_util.h"
#include "../../../util/net/socket_util.h"
#include "../../../util/string/string_util.h"
#include "../../../controller/runtime.h"
#include "../../thread/ThreadsManage.h"
#include "../../thread/CalOperatingThread.h"
#include "../template/NodeThreadMutex.h"
#include <string>

using namespace std;

// 静态成员初始化
CAndroidPushUserManage *CPushUserManage::m_pAndroidUser = NULL;
CIosPushUserManage *CPushUserManage::m_pIosUser = NULL;
CWp7PushUserManage *CPushUserManage::m_pWp7User = NULL;
CWin8PushUserManage *CPushUserManage::m_pWin8User = NULL;

CPushUserManage::CPushUserManage()
{
}


CPushUserManage::~CPushUserManage()
{
}

// 初始化资源
const int
CPushUserManage::CreateRes()
{
	if (NULL == m_pAndroidUser)
	{
		m_pAndroidUser = new CAndroidPushUserManage();
	}
	if (NULL == m_pAndroidUser)
	{
		return -1;
	}
	
	if (NULL == m_pIosUser)
	{
		m_pIosUser = new CIosPushUserManage();
	}
	if (NULL == m_pIosUser)
	{
		return -2;
	}
	
	if (NULL == m_pWp7User)
	{
		m_pWp7User = new CWp7PushUserManage();
	}
	if (NULL == m_pWp7User)
	{
		return -3;
	}
	
	if (NULL == m_pWin8User)
	{
		m_pWin8User = new CWin8PushUserManage();
	}
	if (NULL == m_pWin8User)
	{
		return -4;
	}
	
	return 0;
}

// 初始化
bool 
CPushUserManage::Initial()
{
	int iRes = CreateRes();
	if (iRes < 0)
	{
		DEBUG("CreateRes failed[%d]", iRes);
		return false;
	}
	
	iRes = m_pAndroidUser->InitialLoad()
			* m_pIosUser->InitialLoad()
			* m_pWp7User->InitialLoad()
			* m_pWin8User->InitialLoad();
	if (0 == iRes)
	{
		DEBUG("InitialLoad user failed");
		return false;
	}
	
	return true;
}

// 释放资源
void 
CPushUserManage::Release()
{
	if (NULL != m_pAndroidUser)
	{
		m_pAndroidUser->ReleaseRes();
		delete m_pAndroidUser;
		m_pAndroidUser = NULL;
	}
	
	if (NULL != m_pIosUser)
	{
		m_pIosUser->ReleaseRes();
		delete m_pIosUser;
		m_pIosUser = NULL;
	}
	
	if (NULL != m_pWp7User)
	{
		m_pWp7User->ReleaseRes();
		delete m_pWp7User;
		m_pWp7User = NULL;
	}
	
	if (NULL != m_pWin8User)
	{
		m_pWin8User->ReleaseRes();
		delete m_pWin8User;
		m_pWin8User = NULL;
	}
}

// 整理平台记录
void
CPushUserManage::RearrangeUserNode()
{
	CWp7PushUserManage *pWp7UserManage =
				CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
	pWp7UserManage->ReArrangeInvalidNode();
	
	CAndroidPushUserManage *pAndroidUserManage =
				CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
	pAndroidUserManage->ReArrangeInvalidNode();
	
	CIosPushUserManage *pIosUserManage =
				CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
	pIosUserManage->ReArrangeInvalidNode();
	
	CWin8PushUserManage *pWin8UserManage =
				CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
	pWin8UserManage->ReArrangeInvalidNode();
}

// 同步记录至数据库
int
CPushUserManage::SyncLatestToDb()
{
	int iRet = 0;
	CWp7PushUserManage *pWp7UserManage =
				CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
	iRet = pWp7UserManage->SyncUserToDb()
			* pWp7UserManage->SyncUserBaseInfoHeadToDb();
	
	CAndroidPushUserManage *pAndroidUserManage =
				CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
	iRet = pAndroidUserManage->SyncUserToDb()
			* pAndroidUserManage->SyncUserBaseInfoHeadToDb();
	
	CIosPushUserManage *pIosUserManage =
				CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
	iRet = pIosUserManage->SyncUserToDb()
			* pIosUserManage->SyncUserBaseInfoHeadToDb();
			
	CWin8PushUserManage *pWin8UserManage =
				CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
	iRet = pWin8UserManage->SyncUserToDb()
			* pWin8UserManage->SyncUserBaseInfoHeadToDb();
			
	return iRet;
}

// 打印最新的用户信息占用情况
void
CPushUserManage::PrintLatesUserInfo()
{
	CUserBaseInfoHead *pHead = NULL;
	CWp7PushUserManage *pWp7UserManage =
				CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
	NOTE("***** Show Platform Statistics *****");
	pHead = pWp7UserManage->GetUserBaseInfoHead();
	NOTE("***** Wp7 : MaxUser=%u InvalUser=%u MaxEWarn=%u InvaEWarn=%u *****", 
		pHead->m_nOccupyNum, pWp7UserManage->GetCurrentInvalidCount(), 
		pHead->m_nUseEarlyWarning, pWp7UserManage->GetEWarnNodeInvalidCount());

	CAndroidPushUserManage *pAndroidUserManage =
				CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);	
	pHead = pAndroidUserManage->GetUserBaseInfoHead();
	NOTE("***** Android : MaxUser=%u InvalUser=%u MaxEWarn=%u InvaEWarn=%u *****", 
		pHead->m_nOccupyNum, pAndroidUserManage->GetCurrentInvalidCount(), 
		pHead->m_nUseEarlyWarning, pAndroidUserManage->GetEWarnNodeInvalidCount());
	
	CIosPushUserManage *pIosUserManage =
				CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);				
	pHead = pIosUserManage->GetUserBaseInfoHead();
	NOTE("***** Ios : MaxUser=%u InvalUser=%u MaxEWarn=%u InvaEWarn=%u *****", 
		pHead->m_nOccupyNum, pIosUserManage->GetCurrentInvalidCount(), 
		pHead->m_nUseEarlyWarning, pIosUserManage->GetEWarnNodeInvalidCount());
		
	CWin8PushUserManage *pWin8UserManage =
				CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
	pHead = pWin8UserManage->GetUserBaseInfoHead();
	NOTE("***** Win8 : MaxUser=%u InvalUser=%u MaxEWarn=%u InvaEWarn=%u *****", 
		pHead->m_nOccupyNum, pWin8UserManage->GetCurrentInvalidCount(), 
		pHead->m_nUseEarlyWarning, pWin8UserManage->GetEWarnNodeInvalidCount());

	NOTE("***** End *****");
}

// 置位活跃内存用户死亡
int
CPushUserManage::SetMemActiveUserDead(const char *UserID, const unsigned char Platform)
{
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(Platform);
	unsigned int uUserIndex = (unsigned int)-1;
	CPushUser *pUser = NULL;
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			{
				CWp7PushUserManage *pWp7UserManage = 
					CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
				if (NULL != pWp7UserManage)
				{
					uUserIndex = pWp7UserManage->FindFromNodeMark(UserID, Platform);
				}
				
				if ((unsigned int)-1 != uUserIndex)
				{
					pUser = pWp7UserManage->GetUser(uUserIndex);
				}
			}
			break;
		
		case PFCC_ANDROID:
			{
				CAndroidPushUserManage *pAndroidUserManage =
					CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
				if (NULL != pAndroidUserManage)
				{
					uUserIndex = pAndroidUserManage->FindFromNodeMark(UserID, Platform);
				}
				
				if ((unsigned int)-1 != uUserIndex)
				{
					pUser = pAndroidUserManage->GetUser(uUserIndex);
				}
			}
			break;
			
		case PFCC_IOS:
			{
				CIosPushUserManage *pIosUserManage =
					CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
				if (NULL != pIosUserManage)
				{
					uUserIndex = pIosUserManage->FindFromNodeMark(UserID, Platform);
				}
				
				if ((unsigned int)-1 != uUserIndex)
				{
					pUser = pIosUserManage->GetUser(uUserIndex);
				}
			}
			break;
			
		case PFCC_WIN8:
			{
				CWin8PushUserManage *pWin8UserManage =
					CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
				if (NULL != pWin8UserManage)
				{
					uUserIndex = pWin8UserManage->FindFromNodeMark(UserID, Platform);
				}
				
				if ((unsigned int)-1 != uUserIndex)
				{
					pUser = pWin8UserManage->GetUser(uUserIndex);
				}
			}
			break;
			
		default:
			break;
	}
	
	if ((unsigned int)-1 == uUserIndex)
	{
		return -1;
	}
	
	if (NULL == pUser)
	{
		return -2;
	}
	
	pUser->SetUserActiveProperty(PU_DEAD);
	return 0;
}

