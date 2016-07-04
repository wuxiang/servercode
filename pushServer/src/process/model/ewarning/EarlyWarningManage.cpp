#include "EarlyWarningManage.h"
#include "EarlyWarning.h"
#include "PlatformEWarnManage.h"
#include "../../../util/log/log.h"
#include "../../../util/time/time_util.h"
#include "../../../controller/runtime.h"
#include "../../../util/common/common_file.h"
#include "../../config.h"
#include "../pushuser/PushUserManage.h"
#include "../pushuser/PushUser.h"
#include "../pushuser/AndroidPushUser.h"
#include "../pushuser/AndroidPushUserManage.h"
#include "../pushuser/IphonePushUser.h"
#include "../pushuser/IosPushUserManage.h"
#include "../pushuser/Wp7PushUser.h"
#include "../pushuser/Wp7PushUserManage.h"
#include "../pushuser/Win8PushUser.h"
#include "../pushuser/Win8PushUserManage.h"


// ��̬��Ա��ʼ��
CPlatformEWarnManage *CEwarningManage::m_pAndroidEWarn = NULL;
CPlatformEWarnManage *CEwarningManage::m_pIosEWarn = NULL;
CPlatformEWarnManage *CEwarningManage::m_pWp7EWarn = NULL;
CPlatformEWarnManage *CEwarningManage::m_pWin8EWarn = NULL;

CEwarningManage::CEwarningManage()
{
}

CEwarningManage::~CEwarningManage()
{
}

// ��ʼ����Դ
const int
CEwarningManage::CreateRes()
{
	char PathName[MAX_PATH_LEN] = {0};
	// ����Ŀ¼
	char *pOutPutDir = "./data/ewarning";
	if (!IsDir(pOutPutDir))
	{
		MkDir(pOutPutDir);
	}
	
	const size_t RecordLen = sizeof(CEarlyWarningConditionRecord);
	const PlatformUserConfig *pUserConfig = GetAndroidUserConfig();
	if (NULL == m_pAndroidEWarn)
	{
		CAndroidPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
		snprintf(PathName, MAX_PATH_LEN, "%s/%s", pOutPutDir, pUserConfig->EWarningFileName);
		m_pAndroidEWarn = new CPlatformEWarnManage(pUserConfig->MaxUserWithEWarn, pUserManage->GetUserBaseInfoHead()->m_nUseEarlyWarning
					, pUserConfig->MaxEWarnPerUser, pUserConfig->DefaultEWarnPerUser
					, RecordLen, PathName);
	}
	if (NULL == m_pAndroidEWarn)
	{
		return -1;
	}
	
	pUserConfig = GetIosUserConfig();
	if (NULL == m_pIosEWarn)
	{
		CIosPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
		snprintf(PathName, MAX_PATH_LEN, "%s/%s", pOutPutDir, pUserConfig->EWarningFileName);
		m_pIosEWarn = new CPlatformEWarnManage(pUserConfig->MaxUserWithEWarn, pUserManage->GetUserBaseInfoHead()->m_nUseEarlyWarning
					, pUserConfig->MaxEWarnPerUser, pUserConfig->DefaultEWarnPerUser
					, RecordLen, PathName);
	}
	if (NULL == m_pIosEWarn)
	{
		return -2;
	}
	
	pUserConfig = GetWp7UserConfig();
	if (NULL == m_pWp7EWarn)
	{
		CWp7PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
		snprintf(PathName, MAX_PATH_LEN, "%s/%s", pOutPutDir, pUserConfig->EWarningFileName);
		m_pWp7EWarn = new CPlatformEWarnManage(pUserConfig->MaxUserWithEWarn, pUserManage->GetUserBaseInfoHead()->m_nUseEarlyWarning
					, pUserConfig->MaxEWarnPerUser, pUserConfig->DefaultEWarnPerUser
					, RecordLen, PathName);
	}
	if (NULL == m_pWp7EWarn)
	{
		return -3;
	}
	
	pUserConfig = GetWin8UserConfig();
	if (NULL == m_pWin8EWarn)
	{
		CWin8PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
		snprintf(PathName, MAX_PATH_LEN, "%s/%s", pOutPutDir, pUserConfig->EWarningFileName);
		m_pWin8EWarn = new CPlatformEWarnManage(pUserConfig->MaxUserWithEWarn, pUserManage->GetUserBaseInfoHead()->m_nUseEarlyWarning
					, pUserConfig->MaxEWarnPerUser, pUserConfig->DefaultEWarnPerUser
					, RecordLen, PathName);
	}
	if (NULL == m_pWin8EWarn)
	{
		return -4;
	}
	
	return 0;
}

// ��ȡָ��ƽ̨��Ԥ����¼����
CPlatformEWarnManage *
CEwarningManage::GetPlatFormEWarn(const int platformcode)
{
	CPlatformEWarnManage *pCurrentEWarn = NULL;
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(platformcode);
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			pCurrentEWarn = m_pWp7EWarn;
			break;
		
		case PFCC_ANDROID:
			pCurrentEWarn = m_pAndroidEWarn;
			break;
			
		case PFCC_IOS:
			pCurrentEWarn = m_pIosEWarn;
			break;
			
		case PFCC_WIN8:
			pCurrentEWarn = m_pWin8EWarn;
			break;
			
		default:
			break;
	}
	return pCurrentEWarn;
}

// ��ʼ��
bool 
CEwarningManage::InitialEWarn()
{
	int iRes = CreateRes();
	if (iRes < 0)
	{
		DEBUG("CreateRes failed[%d]", iRes);
		return false;
	}
	
	iRes = m_pAndroidEWarn->InitialLoad()
			* m_pIosEWarn->InitialLoad()
			* m_pWp7EWarn->InitialLoad()
			* m_pWin8EWarn->InitialLoad();
	if (0 == iRes)
	{
		DEBUG("InitialLoad ewarning failed");
		return false;
	}
	
	return true;
}

// �ͷ���Դ
void 
CEwarningManage::ReleaseEWarn()
{
	if (NULL != m_pAndroidEWarn)
	{
		m_pAndroidEWarn->ReleaseRes();
		delete m_pAndroidEWarn;
		m_pAndroidEWarn = NULL;
	}
	
	if (NULL != m_pIosEWarn)
	{
		m_pIosEWarn->ReleaseRes();
		delete m_pIosEWarn;
		m_pIosEWarn = NULL;
	}
	
	if (NULL != m_pWp7EWarn)
	{
		m_pWp7EWarn->ReleaseRes();
		delete m_pWp7EWarn;
		m_pWp7EWarn = NULL;
	}
	
	if (NULL != m_pWin8EWarn)
	{
		m_pWin8EWarn->ReleaseRes();
		delete m_pWin8EWarn;
		m_pWin8EWarn = NULL;
	}
}

// ����Ԥ����¼ռ�ñ�ʶ
int
CEwarningManage::UpdateSetEwarnOccupy(const unsigned char platformcode, const unsigned int NewEWarnIndex)
{
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(platformcode);
	
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			{
				CWp7PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
				return pUserManage->UpdateUseWarnProperty(NewEWarnIndex);
			}
		
		case PFCC_ANDROID:
			{
				CAndroidPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
				return pUserManage->UpdateUseWarnProperty(NewEWarnIndex);
			}
			
		case PFCC_IOS:
			{
				CIosPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
				return pUserManage->UpdateUseWarnProperty(NewEWarnIndex);
			}
			
		case PFCC_WIN8:
			{
				CWin8PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
				return pUserManage->UpdateUseWarnProperty(NewEWarnIndex);
			}
			
		default:
			break;
	}
	
	return -3;
}

// ������Ч��¼
void
CEwarningManage::RearrangeNode()
{
	m_pWp7EWarn->ReArrangeInvalidNode();
	m_pAndroidEWarn->ReArrangeInvalidNode();
	m_pIosEWarn->ReArrangeInvalidNode();
	m_pWin8EWarn->ReArrangeInvalidNode();
}

