#include "Wp7PushUserManage.h"
#include "Wp7PushUser.h"
#include "../../../util/util.h"
#include "../../../util/log/log.h"
#include "../../../util/common/common_file.h"
#include "../../../util/common/common_lib.h"
#include "../../../util/time/time_util.h"
#include "../../../util/string/string_util.h"
#include "../../../controller/runtime.h"
#include "../db/DatabaseSnapshot.h"
#include "../ewarning/PlatformEWarnManage.h"
#include "../ewarning/EarlyWarningManage.h"
#include <sys/mman.h>

CWp7PushUserManage::CWp7PushUserManage()
{
}

CWp7PushUserManage::~CWp7PushUserManage()
{
}

// 更新设置新节点
const int 
CWp7PushUserManage::UpdateSetNode(const int HeadIndex, const CWp7PushUser &Node)
{
	CWp7PushUser *pHeadNode = GetIndexNode(HeadIndex);
	if (NULL == pHeadNode)
	{
		return -1;
	}

	// 节点赋值
	memcpy(pHeadNode, &Node, sizeof(CWp7PushUser));
	// 添加节点标识
	AddToNodeMark(pHeadNode->GetUserID(), pHeadNode->GetUserPlatform(), (unsigned int)HeadIndex);
	return 0;
}

// 节点(组)是否有效
bool
CWp7PushUserManage::IsKeepValid(const int HeadIndex)
{
	CWp7PushUser *pHeadNode = GetIndexNode(HeadIndex);
	if (NULL == pHeadNode)
	{
		return false;
	}
	return (!pHeadNode->IsDead());
}

// 执行整理无效节点(组)
void
CWp7PushUserManage::ProcessArrangeInvalidNode()
{
	int iInvalidStart = GetInvalidFirst();
	CWp7PushUser *pHeadNode = NULL;
	while (iInvalidStart >= 0)
	{
		pHeadNode = GetIndexNode(iInvalidStart);
		if (NULL != pHeadNode)
		{
			// 移除标识索引
			RemoveFromNodeMark(pHeadNode->GetUserID(), pHeadNode->GetUserPlatform());
			// 若设置了预警，设置预警记录为无效
			pHeadNode->ClearWarnRecord();
			// 移除计算队列
			pHeadNode->RemoveFromCalcQueue(iInvalidStart);
		}
		iInvalidStart = GetInvalidNext();
	}
}

// 执行初始化配置
const int 
CWp7PushUserManage::InitialEnv()
{
	// 创建目录
	char *pOutPutDir = "./data/pushuser";
	if (!IsDir(pOutPutDir))
	{
		MkDir(pOutPutDir);
	}
	
	const PlatformUserConfig *pUserConfig = GetWp7UserConfig();
	m_uMaxEWarnUser = pUserConfig->MaxUser;
	m_iRecordLen = sizeof(CWp7PushUser);
	snprintf(m_strFileName, MAX_PATH_LEN, "%s/%s", pOutPutDir, pUserConfig->UserFileName);
	m_uHeadOffset = sizeof(CUserBaseInfoHead);
	return 0;
}

// 创建内存映射
int
CWp7PushUserManage::CreateMemmap(bool &bCreate)
{
	if (0 != InitialEnv())
	{
		m_iLatestStatus = -8;
		return m_iLatestStatus;
	}
	
	int iFd = -1;
	// 计算最终文件大小
	m_iMapFileSize = (off_t)(m_uHeadOffset + m_iRecordLen * m_uMaxEWarnUser);
			
	if (!IsFileExist(m_strFileName))
	{
		iFd = open(m_strFileName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if (iFd < 0)
		{
			m_iLatestStatus = -1;
			return m_iLatestStatus;
		}

		// 设置文件大小
		if ((off_t)-1 == lseek(iFd, m_iMapFileSize + 1, SEEK_SET))
		{
			CloseFile(&iFd);
			m_iLatestStatus = -2;
			return m_iLatestStatus;
		}
		if (write(iFd, "\0", 1) < 0)
		{
			CloseFile(&iFd);
			m_iLatestStatus = -3;
			return m_iLatestStatus;
		}
		lseek(iFd, 0, SEEK_SET);
		bCreate = true;
	}
	else
	{
		iFd = open(m_strFileName, O_RDWR, S_IRUSR | S_IWUSR);
		if (iFd < 0)
		{
			m_iLatestStatus = -4;
			return m_iLatestStatus;
		}
		
		long lFileLen = GetFileLength(m_strFileName);
		if ((m_iMapFileSize + 2) != lFileLen)
		{
			// 设置文件大小
			if ((m_iMapFileSize + 2) > lFileLen)
			{
				if ((off_t)-1 == lseek(iFd, m_iMapFileSize + 1, SEEK_SET))
				{
					CloseFile(&iFd);
					m_iLatestStatus = -5;
					return m_iLatestStatus;
				}
				if (write(iFd, "\0", 1) < 0)
				{
					CloseFile(&iFd);
					m_iLatestStatus = -6;
					return m_iLatestStatus;
				}
				lseek(iFd, 0, SEEK_SET);
			}
			else 
			{
				if (-1 == truncate(m_strFileName, m_iMapFileSize + 2))
				{
					CloseFile(&iFd);
					m_iLatestStatus = -7;
					return m_iLatestStatus;
				}
			}
		}
		bCreate = false;
	}
	
	// 打开映射
	m_pMemAddress = mmap(NULL, m_iMapFileSize, PROT_WRITE | PROT_READ,
						MAP_SHARED, iFd, 0);
	if (MAP_FAILED == m_pMemAddress)
	{
		CloseFile(&iFd);
		m_iLatestStatus = -7;
		return m_iLatestStatus;
	}

	CloseFile(&iFd);
	m_iLatestStatus = 0;
	return m_iLatestStatus;
}

// 初始化数据读入
int 
CWp7PushUserManage::InitialLoad()
{
	bool bCreate = false;
	if (0 != CreateMemmap(bCreate))
	{
		ERROR("CreateMemmap error [%d]", m_iLatestStatus);
		return 0;
	}
	
	if (bCreate)
	{
		InitialUserBaseInfo();
	}
	else
	{
		UpdateUserBaseInfo();
	}
	
	CUserBaseInfoHead* pBaseInfoHead = GetUserBaseInfoHead();
	int iRes = Construct(((char*)m_pMemAddress + m_uHeadOffset), m_iRecordLen, m_uMaxEWarnUser, 
			NULL == pBaseInfoHead ? 0 : pBaseInfoHead->m_nOccupyNum, 1);
	if (iRes < 0)
	{
		ERROR("Construct error ");
		return 0;
	}
	
	InitialLoadUser();
	
	return 1;
}

// 初始化读取用户
void
CWp7PushUserManage::InitialLoadUser()
{
	CUserBaseInfoHead* pBaseInfoHead = GetUserBaseInfoHead();	
	CWp7PushUser *pHeadNode = NULL;
	unsigned int iCount = 0;
	for (unsigned int i = 0; i < pBaseInfoHead->m_nOccupyNum; i++)
	{
		pHeadNode = GetIndexNode(i);
		if (NULL == pHeadNode || IsEmptyString(pHeadNode->GetUserID()))
		{
			continue;
		}
		
		// 更新属性
		pHeadNode->SetUserThreadNum(DistributeUserThread(i));
		
		// 添加节点标识
		AddToNodeMark(pHeadNode->GetUserID(), pHeadNode->GetUserPlatform(), i);
		// 更新标识
		pHeadNode->UpdateBelongCalcThreadProperty(i);
		iCount++;
	}
	
	TRACE("Load Wp7 User total[%u]", iCount);
}

// 释放分配的资源
void 
CWp7PushUserManage::ReleaseRes()
{
	// 释放映射地址
	if (0 == m_iLatestStatus && MAP_FAILED != m_pMemAddress)
	{
		munmap(m_pMemAddress, m_iMapFileSize);
		m_pMemAddress = MAP_FAILED;
	}
}

// 获取用户基本信息头
CUserBaseInfoHead*
CWp7PushUserManage::GetUserBaseInfoHead()const
{
	if (0 != m_iLatestStatus || MAP_FAILED == m_pMemAddress)
	{
		return NULL;
	}
	return (CUserBaseInfoHead*)((char*)m_pMemAddress + 0);
}

// 获取用户
CWp7PushUser*
CWp7PushUserManage::GetUser(const unsigned int NodeIndex)
{
	if (0 != m_iLatestStatus || MAP_FAILED == m_pMemAddress)
	{
		return NULL;
	}
	
	return GetIndexNode(NodeIndex);
}

// 更新占用的用户数属性
const short
CWp7PushUserManage::UpdateOccupyProperty(const unsigned int NewUserIndex)
{
	CUserBaseInfoHead *pHead = GetUserBaseInfoHead();
	
	if (NULL == pHead)
	{
		return -1;
	}
	
	if (NewUserIndex == pHead->m_nOccupyNum)
	{
		pHead->m_nOccupyNum++;
		return 0;
	}
	else if (NewUserIndex > pHead->m_nOccupyNum)
	{
		ERROR("User Map File Maybe in error state.");
		return -2;
	}
	
	return 1;
}

// 更新已经占用的预警资源属性
const short
CWp7PushUserManage::UpdateUseWarnProperty(const unsigned int NewEWarnIndex)
{
	CUserBaseInfoHead *pHead = GetUserBaseInfoHead();
	
	if (NULL == pHead)
	{
		return -1;
	}
	
	if (NewEWarnIndex == pHead->m_nUseEarlyWarning)
	{
		pHead->m_nUseEarlyWarning++;
		return 0;
	}
	else if (NewEWarnIndex > pHead->m_nUseEarlyWarning)
	{
		ERROR("EWarning Map File Maybe in Error State");
		return -2;
	}
	
	return 1;
}

// 初始化标识head
void
CWp7PushUserManage::InitialUserBaseInfo()
{
	CUserBaseInfoHead *pHead = GetUserBaseInfoHead();
	
	if (NULL == pHead)
	{
		return;
	}
	
	memset(pHead, 0, sizeof(CUserBaseInfoHead));
	pHead->m_tDBSynTime = GetNowTime();
	pHead->m_nSpace = m_uMaxEWarnUser;
	pHead->m_nOccupyNum = 0;
		
	const PlatformUserConfig *pUserConfig = GetWp7UserConfig();
	pHead->m_nHisSpace = pUserConfig->MaxEWarningHisNum +
					pUserConfig->MaxInfoHisNum +
					pUserConfig->MaxNoticeHisNum;
	
	pHead->m_nEarlyWarningSpace = pUserConfig->MaxUserWithEWarn;
	pHead->m_nUseEarlyWarning = 0;
	pHead->m_nEarlyWarningPerUser = pUserConfig->MaxEWarnPerUser;
}

// 更新头标识(只能更新大小，文件结构及占位不能更新)
void
CWp7PushUserManage::UpdateUserBaseInfo()
{
	CUserBaseInfoHead *pHead = GetUserBaseInfoHead();
	if (NULL == pHead)
	{
		return;
	}
	
	const PlatformUserConfig *pUserConfig = GetWp7UserConfig();
	pHead->m_nSpace = m_uMaxEWarnUser;
	pHead->m_nEarlyWarningSpace = pUserConfig->MaxUserWithEWarn;
}

// 从数据库加载用户
const int 
CWp7PushUserManage::TryReadUserFromDb(const char *UserID, const unsigned char PlatformCode,
	CWp7PushUser *PushUser)
{
	int iRet = -1;
	char token[MAX_WP_PUSH_TOKEN_LEN + 1] = {0};
	iRet = DatabaseSnapshot::GetMain()->FindUser(UserID, PlatformCode, PushUser, token, MAX_WP_PUSH_TOKEN_LEN);
	if (0 == iRet)
	{
		return -1;
	}
	else if (0 > iRet)
	{
		return -2;
	}
	
	PushUser->SetUserPushToken(token);
	return iRet;
}

const int 
CWp7PushUserManage::SyncUserBaseInfoHeadToDb()
{
	CUserBaseInfoHead *pBasicHead = GetUserBaseInfoHead();
	return DatabaseSnapshot::GetSub()->InsertUserBasicInfo(pBasicHead, GetServerCodeNum(), PFCC_WP7,
		GetCurrentInvalidCount(), GetEWarnNodeInvalidCount());
}

// 返回无效的预警节点段数
const unsigned int
CWp7PushUserManage::GetEWarnNodeInvalidCount()
{
	CPlatformEWarnManage *pCurrentManage = CEwarningManage::GetPlatFormEWarn(WP7);
	return pCurrentManage->GetCurrentInvalidCount();
}

// 同步用户信息
const int
CWp7PushUserManage::SyncUserToDb()
{
	int uScanIndex = 0;
	int uCount = GetNodeCount();
	if (SNMS_ERROR == GetManageStatus())
	{
		return -1;
	}
	
	CWp7PushUser *pUser = NULL;
	char token[MAX_WP_PUSH_TOKEN_LEN + 1] = {0};

	for (uScanIndex = 0; uScanIndex < uCount; uScanIndex++)
	{
		bzero(token, MAX_WP_PUSH_TOKEN_LEN + 1);
		pUser = GetIndexNode(uScanIndex);
		if (NULL == pUser)
		{
			continue;
		}
		
		// 同步信息
		pUser->GetUserPushToken(token, MAX_WP_PUSH_TOKEN_LEN);
		pUser->SyncLatestToDb(uScanIndex, token);
		// 清除无效的历史记录
		pUser->DeleteOutofDateHisMsg();
	}
	
	GetUserBaseInfoHead()->m_tDBSynTime = GetNowTime();

	return uScanIndex;
}

