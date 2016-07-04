#include "CalOperatingThread.h"
#include "ThreadsManage.h"
#include "UpdateDynaDataThread.h"
#include "../../util/util.h"
#include "../../util/log/log.h"
#include "../../util/net/socket_util.h"
#include "../../util/common/common_env.h"
#include "../../util/common/common_lib.h"
#include "../../util/time/time_util.h"
#include "../../controller/runtime.h"
#include "../model/pushuser/PushUserManage.h"
#include "../model/pushuser/PushUser.h"
#include "../model/pushuser/AndroidPushUser.h"
#include "../model/pushuser/AndroidPushUserManage.h"
#include "../model/pushuser/IphonePushUser.h"
#include "../model/pushuser/IosPushUserManage.h"
#include "../model/pushuser/Wp7PushUser.h"
#include "../model/pushuser/Wp7PushUserManage.h"
#include "../model/pushuser/Win8PushUser.h"
#include "../model/pushuser/Win8PushUserManage.h"
#include "../config.h"
#include "../data_type.h"
#include "../model/outerservpush/AppPushNotice.h"
#include "../model/outerservpush/Push2WP.h"
#include "../model/data/BuffServer.h"
#include "../model/template/NodeThreadMutex.h"
#include "../model/selfstock/SelfSelectStockManage.h"

using namespace std;

CalOperatingThread::CalOperatingThread()
	: m_pSendBuf(new LargeMemoryCache(MAX_SINGLE_RESPONSE)),
	m_pEwarningUserList(new CIndexList()),
	m_pActiveUserList(new CIndexList())
{
	m_pSendBuf->ClearAll();
	m_PublicNoticeFlag.uState = TF_BEGIN;
	m_SelfStkInfoMineStatisFlag.uState = TF_BEGIN;
}

CalOperatingThread::~CalOperatingThread() {

	if (NULL != m_pSendBuf)
	{
		delete m_pSendBuf;
		m_pSendBuf = NULL;
	}

	if (NULL != m_pEwarningUserList)
	{
		delete m_pEwarningUserList;
		m_pEwarningUserList = NULL;
	}

	if (NULL != m_pActiveUserList)
	{
		delete m_pActiveUserList;
		m_pActiveUserList = NULL;
	}
}

// ��ʼ�̻߳���
bool
CalOperatingThread::InitialThreadEnv() {
	ThreadImp::InitialThreadEnv();
	time_t NowTime = GetNowTime();
	m_ExecuteStkWarningTimer = NowTime;
	m_ExecuteStkPrivateNoticTimer = NowTime;
	m_ExecutePublicNoticTimer = NowTime;
	m_ExecuteUserAliveCheckTimer = NowTime;
	m_ExecuteSelfMineStatisTimer = NowTime;

	// ��λ�Ѿ���ʼ��(��������Ҫ���Ͻ��̳�ʼ��)
	// ����
	m_bHasInitialed = true;
	m_iInitialDate = GetIntDate(&NowTime);

	return true;
}

// �����߳�
int
CalOperatingThread::StartThread() {
	pthread_t  tid;
	pthread_attr_t   thread_attr;

	pthread_attr_init(&thread_attr);
	/* ���ö�ջ��С*/
	if(pthread_attr_setstacksize(&thread_attr, 1024*1024) != 0)//1024 K
	{
		FATAL("pthread_attr_setstacksize failed.");
		exit(-1);
		return -1;
	}

	if (pthread_create(&tid, &thread_attr, CalOperatingThread::ThreadProc, this))
	{
		FATAL("Create CalOperatingThread thread error");
		exit(-2);
		return -2;
	}

	SetThreadID(tid);
	pthread_detach(tid);
	return 0;
}

// �̴߳�����
void*
CalOperatingThread::ThreadProc(void *param) {
	CalOperatingThread *pCurrentThread = (CalOperatingThread*)param;
	if (NULL == pCurrentThread) {
		return 0;
	}
	TRACE("CalOperatingThread[%u] Started.", (unsigned int)pthread_self());

	int MatchCount = 0;
	while(1) {

		if (Thread_Terminate == pCurrentThread->GetThreadRunState()) {
			DEBUG("CalOperatingThread[%u] Terminate", (unsigned int)pthread_self());
			break;
		}
		else if (Thread_Running == pCurrentThread->GetThreadRunState()) {
			// ִ�д���
			if (0 == (
				pCurrentThread->ExecuteInitial()
				* pCurrentThread->ProcedureMsgQueue()
				* pCurrentThread->DetectStkLatestResetSignal()
				* pCurrentThread->ProcessStkPriceWarningCal()
				* pCurrentThread->ProcessSelfStkInfoMineScan()
				* pCurrentThread->ProcessPublicNoticeScan(MatchCount)
				* pCurrentThread->ProcessUserAliveCheck()
				))
			{
				continue;
			}
		}
		else if (Thread_Waiting == pCurrentThread->GetThreadRunState()) {

		}
		else if (Thread_Initial == pCurrentThread->GetThreadRunState()) {

		}
		SleepMs(CALC_THREAD_TIME_INTERVAL);
	}
	return 0;
}

// ƽ̨�ɼ�Ԥ��
int
CalOperatingThread::PlatformStkPriceWarningCal(const IndexMap *CurrentUserInfo)
{
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(CurrentUserInfo->PlatformCode);

	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			{
				CWp7PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
				CWp7PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser && PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ExecuteStkPriceWarningCal(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_EWARNING);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_ANDROID:
			{
				CAndroidPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
				CAndroidPushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser && PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ExecuteStkPriceWarningCal(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_EWARNING);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_IOS:
			{
				CIosPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
				CIphonePushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser && PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ExecuteStkPriceWarningCal(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_EWARNING);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;
			
		case PFCC_WIN8:
			{
				CWin8PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
				CWin8PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser && PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ExecuteStkPriceWarningCal(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_EWARNING);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		default:
			break;
	}

	return 0;
}

// ִ�нڵ�ɼ�Ԥ������
int
CalOperatingThread::ProcessStkPriceWarningCal()
{
	if (!m_bHasInitialed)
	{
		return -1;
	}
	
	if (IsHangupRequest())
	{
		return -2;
	}

	time_t NowTime = GetNowTime();
	if (DiffTimeSecond(NowTime, m_ExecuteStkWarningTimer) < GetEWarningRuntime()->ScanInterval
		&& !GetSignal(CSP_EXE_WARN))
	{
		return -3;
	}
	m_ExecuteStkWarningTimer = NowTime;

	IndexMap *pUserMark = m_pEwarningUserList->GetFirst();
	while(NULL != pUserMark)
	{
		PlatformStkPriceWarningCal(pUserMark);
		pUserMark = m_pEwarningUserList->GetNext();
	}

	SetSignal(CSP_EXE_WARN, false);
	return 0;
}

// ƽ̨����
int
CalOperatingThread::PlatformNoticeScan(const IndexMap *CurrentUserInfo)
{
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(CurrentUserInfo->PlatformCode);
	int iRes = -1;
	
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			{
				CWp7PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
				CWp7PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						iRes = pUser->ExecutePublicNoticeScan(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_NOTICE);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_ANDROID:
			{
				CAndroidPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
				CAndroidPushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						iRes = pUser->ExecutePublicNoticeScan(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_NOTICE);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_IOS:
			{
				CIosPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
				CIphonePushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						iRes = pUser->ExecutePublicNoticeScan(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_NOTICE);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;
			
		case PFCC_WIN8:
			{
				CWin8PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
				CWin8PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						iRes = pUser->ExecutePublicNoticeScan(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_NOTICE);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		default:
			break;
	}

	return iRes;
}

// ִ�й������Ź������ɨ��
int
CalOperatingThread::ProcessPublicNoticeScan(int &MatchCount)
{
	if (!m_bHasInitialed)
	{
		return -1;
	}
	
	if (IsHangupRequest())
	{
		return -2;
	}

	time_t NowTime = GetNowTime();
	int iTime = GetIntTime(&NowTime);
	if (iTime < GetNoticeRuntime()->RunStartTime || iTime > GetNoticeRuntime()->RunEndTime)
	{
		return -3;
	}
	
	if (DiffTimeSecond(NowTime, m_ExecutePublicNoticTimer) < GetNoticeRuntime()->ScanInterval
		&& !GetSignal(CSP_EXE_NOTICE))
	{
		return -4;
	}
	m_ExecutePublicNoticTimer = NowTime;
	
	IndexMap *pUserMark = NULL;
	switch(m_PublicNoticeFlag.uState)
	{
		case TF_BEGIN:
		case TF_END:
			{
			pUserMark = m_pActiveUserList->GetFirst();
			}
			break;

		case TF_CONTINUE:
			{
			m_pActiveUserList->RestoreToPos(m_PublicNoticeFlag.UserMark);
			pUserMark = m_pActiveUserList->GetNext();
			}
			break;

		default:
			break;
	}
	
	if (TF_BEGIN == m_PublicNoticeFlag.uState
		|| TF_END == m_PublicNoticeFlag.uState)
	{
		MatchCount = 0;
		NOTE("<<PublicNotice Scan By Cal[%u] begin>>", (unsigned int)m_tThreadID);
	}

	const int iProcessUnit = GetNoticeRuntime()->ProcessUnit;
	int iCount = 0;
	while(NULL != pUserMark)
	{
		if (PlatformNoticeScan(pUserMark) > 0)
		{
			MatchCount++;
		}
		m_PublicNoticeFlag.UserMark.UserMapIndex = pUserMark->UserMapIndex;
		m_PublicNoticeFlag.UserMark.PlatformCode = pUserMark->PlatformCode;

		iCount++;
		if (iCount >= iProcessUnit)
		{
			m_PublicNoticeFlag.uState = TF_CONTINUE;
			return 0;
		}

		pUserMark = m_pActiveUserList->GetNext();
	}

	NOTE("Notice Scan by Cal[%u] Conclude ScanNum=%d MatchCount=%d", 
				(unsigned int)m_tThreadID, m_pActiveUserList->GetCount(), MatchCount);
	m_PublicNoticeFlag.uState = TF_END;
	SetSignal(CSP_EXE_NOTICE, false);
	return 0;
}

// ƽ̨�û����г�ʼ��
int
CalOperatingThread::PlatformUserInitial(const IndexMap *CurrentUserInfo)
{
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(CurrentUserInfo->PlatformCode);

	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			{
				CWp7PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
				CWp7PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						if (PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
						{
							pUser->ResetAllEarlyWarningRecordState();
						}
						
						pUser->ResetSelfSelectStkState(CurrentUserInfo->UserMapIndex);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_ANDROID:
			{
				CAndroidPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
				CAndroidPushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						if (PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
						{
							pUser->ResetAllEarlyWarningRecordState();
						}
						
						pUser->ResetSelfSelectStkState(CurrentUserInfo->UserMapIndex);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_IOS:
			{
				CIosPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
				CIphonePushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						if (PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
						{
							pUser->ResetAllEarlyWarningRecordState();
						}
						
						pUser->ResetSelfSelectStkState(CurrentUserInfo->UserMapIndex);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;
			
		case PFCC_WIN8:
			{
				CWin8PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
				CWin8PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						if (PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
						{
							pUser->ResetAllEarlyWarningRecordState();
						}
						
						pUser->ResetSelfSelectStkState(CurrentUserInfo->UserMapIndex);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		default:
			break;
	}

	return 0;
}

// ִ�г�ʼ��
int
CalOperatingThread::ExecuteInitial()
{
	if (!CanResetNow())
	{
		return -1;
	}
	
	if (IsHangupRequest())
	{
		return -2;
	}

	DEBUG("CalOperatingThread::ExecuteInitial[%u]", (unsigned int)pthread_self());

	CSelfSelectStockManage::ResetConfigMark();
		
	// ���ýڵ�״̬
	IndexMap *pUserMark = m_pActiveUserList->GetFirst();
	while(NULL != pUserMark)
	{
		PlatformUserInitial(pUserMark);
		pUserMark = m_pActiveUserList->GetNext();
	}

	// ��λ�Ѿ���ʼ��
	m_bHasInitialed = true;

	return 0;
}

// ƽ̨�û�״̬���
int
CalOperatingThread::PlatformUserAliveCheck(const IndexMap *CurrentUserInfo, const time_t NowTime)
{
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(CurrentUserInfo->PlatformCode);

	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			{
				CWp7PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
				CWp7PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->CheckUserAliveStatus(NowTime);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_ANDROID:
			{
				CAndroidPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
				CAndroidPushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->CheckUserAliveStatus(NowTime);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_IOS:
			{
				CIosPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
				CIphonePushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->CheckUserAliveStatus(NowTime);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;
			
		case PFCC_WIN8:
			{
				CWin8PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
				CWin8PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->CheckUserAliveStatus(NowTime);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		default:
			break;
	}

	return 0;
}

// ִ�м���û����״̬
int
CalOperatingThread::ProcessUserAliveCheck()
{
	time_t NowTime = GetNowTime();
	if (DiffTimeSecond(NowTime, m_ExecuteUserAliveCheckTimer) < CHECK_USER_ALIVE_TIME)
	{
		return -1;
	}
	m_ExecuteUserAliveCheckTimer = NowTime;
	
	if (IsHangupRequest())
	{
		return -2;
	}

	IndexMap *pUserMark = m_pActiveUserList->GetFirst();
	while(NULL != pUserMark)
	{
		PlatformUserAliveCheck(pUserMark, NowTime);
		pUserMark = m_pActiveUserList->GetNext();
	}

	return 0;
}

// ��ȡ��Ӧ��Դ����
LargeMemoryCache*
CalOperatingThread::GetResponseCache()
{
	return m_pSendBuf;
}

// ƽ̨�û���������������Ϣ����
int
CalOperatingThread::PlatformStkLatestReset(const IndexMap *CurrentUserInfo)
{
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(CurrentUserInfo->PlatformCode);

	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			{
				CWp7PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
				CWp7PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser && PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ResetEarlyWarningRecordLastestStkInfo();
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_ANDROID:
			{
				CAndroidPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
				CAndroidPushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser && PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ResetEarlyWarningRecordLastestStkInfo();
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_IOS:
			{
				CIosPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
				CIphonePushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser && PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ResetEarlyWarningRecordLastestStkInfo();
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;
			
		case PFCC_WIN8:
			{
				CWin8PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
				CWin8PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser && PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ResetEarlyWarningRecordLastestStkInfo();
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		default:
			break;
	}

	return 0;
}

// �������֤ȯ����������Ϣ�ź�
int
CalOperatingThread::DetectStkLatestResetSignal()
{
	// H �ж���Ϊ0λ
	if (!GetSignal(CSP_HQ_INDEX_RESET))
	{
		return -1;
	}
	
	if (IsHangupRequest())
	{
		return -2;
	}

	// ����֤ȯ����������Ϣ
	IndexMap *pUserMark = m_pEwarningUserList->GetFirst();
	while(NULL != pUserMark)
	{
		PlatformStkLatestReset(pUserMark);
		pUserMark = m_pEwarningUserList->GetNext();
	}

	SetSignal(CSP_HQ_INDEX_RESET, false);
	return 0;
}

// ���Ԥ���û�
void
CalOperatingThread::AddEWarningUser(const unsigned int index, const unsigned char cPlatform)
{
	m_pEwarningUserList->AddIndexList(index, cPlatform);
}

// �Ƴ�Ԥ���û�
void
CalOperatingThread::RemoveEWarningUser(const unsigned int index, const unsigned char cPlatform)
{
	m_pEwarningUserList->RemoveIndexList(index, cPlatform);
}

// ��ӻ�Ծ�û�
void
CalOperatingThread::AddActiveUser(const unsigned int index, const unsigned char cPlatform)
{
	m_pActiveUserList->AddIndexList(index, cPlatform);
}

// �Ƴ���Ծ�û�
void
CalOperatingThread::RemoveActiveUser(const unsigned int index, const unsigned char cPlatform)
{
	m_pActiveUserList->RemoveIndexList(index, cPlatform);
}

// ������Ϣ����
int
CalOperatingThread::ProcedureMsgQueue()
{
	int iCount = -1;
	
	while(!IsEmpty())
	{
		iCount++;
		CMsgMark Msg;
		if (-1 != GetFront(Msg))
		{
			switch(Msg.m_Msg)
			{
				case UPD_DYNA_THREAD_REQUEST_RESET:
					{
						DEBUG("Send UPD_DYNA_THREAD_REQUEST_RESET");
						SetSignal(NSP_HANGUP_REQUEST, true);
						ThreadsManage::GetUpdDataThread()->SendMsg(UPD_DYNA_THREAD_CONFIRM_RESET, GetThreadKey(), -1);
					}
					break;
					
				case UPD_HQ_INDEX_RESET:
					{
						SetSignal(CSP_HQ_INDEX_RESET, 1 == Msg.m_wParam);
					}
					break;
					
				case UPD_EXE_WARN:
					{
						SetSignal(CSP_EXE_WARN, 1 == Msg.m_wParam);
					}
					break;
					
				case UPD_EXE_INFOMINE:
					{
						SetSignal(CSP_EXE_INFOMINE, 1 == Msg.m_wParam);
					}
					break;
					
				case UPD_EXE_NOTICE:
					{
						SetSignal(CSP_EXE_NOTICE, 1 == Msg.m_wParam);
					}
					break;
					
				case UPD_EXE_INFOMINE_STATIS:
					{
						SetSignal(CSP_EXE_INFOMINE_STATIS, 1 == Msg.m_wParam);
					}
					break;
					
				case UPD_HUNGUP_THREAD_PROC:
					{
						SetSignal(NSP_HANGUP_REQUEST, 1 == Msg.m_wParam);
					}
					break;
					
				default:
					DEBUG("unused msg found[%u]", Msg.m_Msg);
					break;
			}
			PopFront();
		}
	}
	
	return -1 == iCount ? -1 : 0;
}

// ִ����ѡ����Ϣ����ɨ��
int
CalOperatingThread::ProcessSelfStkInfoMineScan()
{
	if (!m_bHasInitialed)
	{
		return -1;
	}
	
	if (IsHangupRequest())
	{
		return -2;
	}

	time_t NowTime = GetNowTime();
	int iTime = GetIntTime(&NowTime);
	if (iTime < GetInfoMineRuntime()->RunStartTime || iTime > GetInfoMineRuntime()->RunEndTime)
	{
		return -3;
	}
	
	if (DiffTimeSecond(NowTime, m_ExecuteSelfMineStatisTimer) < GetInfoMineRuntime()->ScanInterval
		&& !GetSignal(CSP_EXE_INFOMINE_STATIS)
		&& !GetSignal(CSP_EXE_INFOMINE))
	{
		return -4;
	}
	m_ExecuteSelfMineStatisTimer = NowTime;

	IndexMap *pUserMark = NULL;
	switch(m_SelfStkInfoMineStatisFlag.uState)
	{
		case TF_BEGIN:
		case TF_END:
			{
			pUserMark = m_pActiveUserList->GetFirst();
			}
			break;

		case TF_CONTINUE:
			{
			m_pActiveUserList->RestoreToPos(m_SelfStkInfoMineStatisFlag.UserMark);
			pUserMark = m_pActiveUserList->GetNext();
			}
			break;

		default:
			break;
	}

	const int iProcessUnit = GetInfoMineRuntime()->ProcessUnit;
	int iCount = 0;
	TRACE("<<SelfStkInfoMineStatis Scan By Cal[%u] begin>>", (unsigned int)m_tThreadID);
	while(NULL != pUserMark)
	{
		PlatformSelfStkInfoMineScan(pUserMark);
		m_SelfStkInfoMineStatisFlag.UserMark.UserMapIndex = pUserMark->UserMapIndex;
		m_SelfStkInfoMineStatisFlag.UserMark.PlatformCode = pUserMark->PlatformCode;

		iCount++;
		if (iCount >= iProcessUnit)
		{
			TRACE("SelfStkInfoMineStatis Scan by Cal[%u] Conclude ScanNum=%d", 
				(unsigned int)m_tThreadID, iCount);
			m_SelfStkInfoMineStatisFlag.uState = TF_CONTINUE;
			return 0;
		}

		pUserMark = m_pActiveUserList->GetNext();
	}
	
	TRACE("SelfStkInfoMineStatis Scan by Cal[%u] Conclude ScanNum=%d", 
				(unsigned int)m_tThreadID, iCount);

	m_SelfStkInfoMineStatisFlag.uState = TF_END;
	SetSignal(CSP_EXE_INFOMINE_STATIS, false);
	SetSignal(CSP_EXE_INFOMINE, false);
	return 0;
}

// ƽ̨��ѡ����Ϣ����ɨ��
int
CalOperatingThread::PlatformSelfStkInfoMineScan(const IndexMap *CurrentUserInfo)
{
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(CurrentUserInfo->PlatformCode);
	
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			{
				CWp7PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
				CWp7PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ExecuteSelfStkInfoMine(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_INFOMINE);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_ANDROID:
			{
				CAndroidPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
				CAndroidPushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ExecuteSelfStkInfoMine(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_INFOMINE);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		case PFCC_IOS:
			{
				CIosPushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
				CIphonePushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ExecuteSelfStkInfoMine(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_INFOMINE);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;
			
		case PFCC_WIN8:
			{
				CWin8PushUserManage *pUserManage =
					CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
				CWin8PushUser *pUser = pUserManage->GetUser(CurrentUserInfo->UserMapIndex);

				if (NULL != pUser)
				{
					// �������Ȩ��
					bmuGot = false;
					uObjID = pNodeThreadMutex->GenerateObjID(CurrentUserInfo->UserMapIndex, CurrentUserInfo->PlatformCode);
					muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
					if (LOCK_REQUIRED == muRes)
					{
						if (pNodeThreadMutex->TryAskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
						{
							bmuGot = true;
						}
					}

					if (LOCK_SHARED == muRes
						|| (LOCK_REQUIRED == muRes && bmuGot))
					{
						pUser->ExecuteSelfStkInfoMine(CurrentUserInfo->UserMapIndex);
						pUser->ExecuteSend(CurrentUserInfo->UserMapIndex, PTE_INFOMINE);
					}
					else
					{
						ERROR("control error.");
					}

					// �ύȨ��
					if (bmuGot)
					{
						pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
					}
				}
			}
			break;

		default:
			break;
	}

	return 0;
}

