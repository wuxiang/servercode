#include "HqLinkNode.h"
#include "../../../util/util.h"
#include "../../../util/log/log.h"
#include "../../../util/time/time_util.h"
#include "../../../util/net/socket_util.h"
#include "../../../controller/runtime.h"
#include "../data/BuffServer.h"
#include "../../data_type.h"
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
#include "../../thread/CalOperatingThread.h"
#include "../../thread/ThreadsManage.h"
#include "../../thread/NetThread.h"
#include "../template/NodeThreadMutex.h"
#include "../ErrorMsg/ErrorMsg.h"
#include "../history/PushMsgHistoryManage.h"
#include "../template/OuterSendMark.h"

using namespace std;


HqLinkNode::HqLinkNode(const unsigned int socketFd, const char *ip)
	: CNodeInterface<HqLinkNode>(),
	fd(socketFd),
	state(HQLK_NORMAL),
	m_pReceiveBuf(NULL),
	m_pSendBuf(NULL),
	m_iLatestSendDataPos(0)
{
	StrCopy(IP, ip, MAX_IP_LEN);
	snprintf(m_strKey, MAX_LINK_KEY_LEN, "%u:%s", socketFd, ip);
	UpdateLastestAliveTime();
}

HqLinkNode::HqLinkNode()
	: fd(0),
	state(HQLK_NORMAL),
	m_pReceiveBuf(NULL),
	m_pSendBuf(NULL),
	m_iMarkIndex(0),
	m_iLatestSendDataPos(0)
{
	bzero(IP, MAX_IP_LEN);
}

HqLinkNode::~HqLinkNode()
{
	if (NULL != m_pReceiveBuf)
	{
		delete m_pReceiveBuf;
		m_pReceiveBuf = NULL;
	}

	if (NULL != m_pSendBuf)
	{
		delete m_pSendBuf;
		m_pSendBuf = NULL;
	}
	
	while(!m_SendQueue.empty())
	{
		m_SendQueue.pop();
	}
}

void
HqLinkNode::UpdateLastestAliveTime()
{
	LastestAliveTime = GetNowTime();
}

// 设置链接状态，CalOperatingThread线程调用
void
HqLinkNode::SetState(const enum HqLinkState NewState)
{
	state = NewState;
}

// 重新构造
void
HqLinkNode::ResetNode(const HqLinkNode &item, const int PosFlag)
{
	fd = item.fd;
	StrCopy(IP, item.IP, MAX_IP_LEN);
	state = item.state;
	m_iMarkIndex = item.m_iMarkIndex;
	m_iLatestSendDataPos = item.m_iLatestSendDataPos;

	if (NULL == m_pReceiveBuf)
	{
		m_pReceiveBuf = new LargeMemoryCache(MAX_SINGLE_REQUEST);
	}
	m_pReceiveBuf->ClearAll();

	if (NULL == m_pSendBuf)
	{
		m_pSendBuf = new LargeMemoryCache(MAX_SINGLE_RESPONSE);
	}
	m_pSendBuf->ClearAll();

	snprintf(m_strKey, MAX_LINK_KEY_LEN, "%u:%s", fd, IP);

	// 更新状态
	UpdateLastestAliveTime();
	SetState(HQLK_NORMAL);
	while(!m_SendQueue.empty())
	{
		m_SendQueue.pop();
	}
}

// 清除处理
void
HqLinkNode::Clear()
{
	if (fd > 2)
	{
		DEBUG("Close socket.[fd = %u, IP=%s]", fd, IP);
		int iSockFd = (int)fd;
		Close(iSockFd);
		fd = 0;
	}
	SetState(HQLK_EMPTY);
	while(!m_SendQueue.empty())
	{
		m_SendQueue.pop();
	}
}

// 获取标识ID
const std::string
HqLinkNode::GetMarkID()const
{
	return m_strKey;
}

// 是否已经无效
bool
HqLinkNode::IsDead()
{
	if (HQLK_NORMAL != state)
	{
		return true;
	}

	time_t NowTime = GetNowTime();
	if (DiffTimeSecond(NowTime, LastestAliveTime) >= HQ_LINK_ALIVE_TIME)
	{
		DEBUG("HqLinkNode dead[%s]", IP);
		SetState(HQLK_TIMEOUT);
		return true;
	}

	return false;
}

// 节点接收数据处理
int
HqLinkNode::NodeReceiveProcess()
{
	int iRes = -1;
	
	do
	{
		// 申请内存
		int iBufLen = 0;
		char *pReceiveBuf = m_pReceiveBuf->GetRemainMem(iBufLen);
		if (NULL == pReceiveBuf || iBufLen <= 0)
		{
			DEBUG("Not enough memory.");
			break;
		}
		if (0 != m_pReceiveBuf->GetLatestWarning())
		{
			WARN("m_pReceiveBuf[%d  %s] maybe smaller.", GetFd(), IP);
		}
		
		iRes = recv(GetFd(), pReceiveBuf, iBufLen, 0);
		
		if (iRes <= 0)
		{
			if(!iRes ||(EWOULDBLOCK != errno && EINTR != errno))
			{
				ERROR("recv error [ip=%s errno=%s]", IP, strerror(errno));
				SetState(HQLK_CLOSE);
				return -2;
			}
			break;
		}
		
		m_pReceiveBuf->SetRemainMemStart(iRes);
	
	} while(iRes > 0);
	
	if (m_pReceiveBuf->GetUsedMemoryLen() <= 0)
	{
		return -3;
	}
	else {
		// 更新存活时间
		UpdateLastestAliveTime();
		if (!ProcessRecData(m_pReceiveBuf->GetRawMemPointer(0), m_pReceiveBuf->GetUsedMemoryLen())
			&& HQLK_CLOSE == GetLinkState())
		{
			return -4;
		}
	}
	return iRes;
}

// 处理已经收到的数据
int
HqLinkNode::ProcessRecData(const void *data, const int length)
{
	if (NULL == data || length <= 0)
	{
		m_pReceiveBuf->ClearUsedMem();
		return -1;
	}

	// 数据解析器
	CBuffReader reader((void*)data, length, 0);

	ACC_CMDHEAD *pCmdHead = NULL;
	int iDataLen = 0;
	bool bReserveUnCompleteData = false;
	bool bValidBegin = false;
	while(1)
	{
		bValidBegin = false;
		pCmdHead = reader.ReadPoiner<ACC_CMDHEAD>();
		if (NULL != pCmdHead)
		{
			iDataLen = pCmdHead->m_nLen;
			if (iDataLen > (int)reader.GetUnReadLen())
			{
				reader.SeekPos((int)sizeof(ACC_CMDHEAD) * -1);
				bReserveUnCompleteData = true;
				break;
			}
			else
			{
				bValidBegin = true;

				// 申请响应数据缓存
				int iBufLen = 0;
				char *pBuf = m_pSendBuf->GetRemainMem(iBufLen);
				if (0 != m_pSendBuf->GetLatestWarning())
				{
					// 缓存不足，放弃本次解析
					reader.SeekPos((int)sizeof(ACC_CMDHEAD) * -1);
					bReserveUnCompleteData = true;
					break;
				}
				CBuffWriter writer(pBuf, iBufLen, 0);
				// 预留出头的空间
				writer.SeekPos(sizeof(ACC_CMDHEAD));

				// 获取返回属性定义
				bool bMustSendResult = (0 != (pCmdHead->m_wAttr & ACCATTR_COMINTERFACE_RET));
				if (!ParseRequestData(pCmdHead, reader.GetCurrentStartData(), iDataLen, &writer))
				{
					DEBUG("ParseRequestData found error.");
					if (HQLK_CLOSE == GetLinkState())
					{
						m_pReceiveBuf->ClearUsedMem();
						return -2;
					}
				}

				if (writer.IsFull())
				{
					// 缓存不足，放弃本次解析
					reader.SeekPos(((int)sizeof(ACC_CMDHEAD)) * -1);
					bReserveUnCompleteData = true;

					if (writer.GetNewdataLen() > 0)
					{
						m_pSendBuf->ClearRemainMem(writer.GetNewdataLen());
					}
					break;
				}

				// 置位下次开始位置
				if (writer.GetNewdataLen() > 0)
				{
					if (bMustSendResult
						|| ACCCMD_KEEPALIVE == pCmdHead->m_wCmdType)
					{
						m_pSendBuf->SetRemainMemStart(writer.GetNewdataLen());
					}
					else
					{
						m_pSendBuf->ClearRemainMem(writer.GetNewdataLen());
					}
				}
				reader.SeekPos(iDataLen);
				continue;
			}
		}
		else
		{
			if (!reader.IsEnd())
			{
				bReserveUnCompleteData = true;
			}
			break;
		}
	}

	// 保存未处理完成的数据
	if (bReserveUnCompleteData)
	{
		if (!m_pReceiveBuf->MoveRangeMemToStart(reader.GetCurrentPos(), reader.GetUnReadLen()))
		{
			DEBUG("MoveRangeMemToStart failed");
		}
		m_pReceiveBuf->ClearRemainMem(reader.GetUnReadLen());
	}
	else
	{
		if (reader.IsEnd())
		{
			m_pReceiveBuf->ClearUsedMem();
		}
		else
		{
			FATAL("wrong data has been found[ip=%s, errno=%s]", IP, strerror(errno));
			SetState(HQLK_CLOSE);
			return -12;
		}
	}

	return 0;
}

// 解析请求数据
bool
HqLinkNode::ParseRequestData(const struct ACC_CMDHEAD *accHead, const void *data,
	const int length, CBuffWriter *writer)
{
	switch(accHead->m_wCmdType)
	{
		case ACCCMD_SERVERLOGIN:
			return ProcessServerLogin(data, length, false, writer);

		case ACCCMD_KEEPALIVE:
			return ProcessKeepLive(accHead, false, writer);

		case ACCCMD_REQ_GUID:
			return ParseUserRequestData(accHead, data, length, writer);

		default:
			break;
	}

	return true;
}

// 解析用户请求数据
bool
HqLinkNode::ParseUserRequestData(const struct ACC_CMDHEAD *accHead, const void *data, const int length,
	CBuffWriter *writer)
{
	sub_head *pCmdHead = NULL;
	int iRemainLen = length;
	CBuffReader reader((void*)data, length, 0);
	UserLogInfor user;
	int iRes = 0;
	const char *pErrorMsg = NULL;
	int iTotalUserLogLen = 0;

	while (iRemainLen > 0)
	{
		pCmdHead = reader.ReadPoiner<sub_head>();
		if (NULL == pCmdHead || pCmdHead->sub_length > reader.GetUnReadLen())
		{
			DEBUG("parse user data head cmd error.");
			iRes = -26;
			goto END;
		}

		// 解析用户信息
		int iGot = 0;
		bzero(&user, sizeof(UserLogInfor));
		if (0 != (iGot = ParseUserInfoMark(reader.GetCurrentStartData(), pCmdHead->sub_length, &user, iTotalUserLogLen)))
		{
			DEBUG("parse user information error.[%d]", iGot);
			iRes = -27;
			goto END;
		}
		
		// 初始化或数据备份时放弃用户请求
		if (!ThreadsManage::GetNetThread()->IsHangupRequest())
		{
			if (!ParsePlatformUserRequest(accHead, pCmdHead, &user, reader.GetCurrentStartData() + iTotalUserLogLen,
								pCmdHead->sub_length - iTotalUserLogLen, writer))
			{
				DEBUG("pPushUser->ProcessUserRequest error.");
			}
		}

		if (!reader.SeekPos(pCmdHead->sub_length))
		{
			DEBUG("Data error.");
			iRes = -30;
			goto END;
		}

		iRemainLen -= (sizeof(sub_head) + pCmdHead->sub_length);
	}

	// 只要是要求有返回数据的，空也可以返回
	WriteRespData(accHead, writer);
	return true;

END:
	{
		sub_head subHead = {0};
		if (NULL != pCmdHead)
		{
			memcpy(&subHead, pCmdHead, sizeof(sub_head));
		}
		subHead.sub_type = ST_ERROR;

		int iErroCode = iRes * -1;
		pErrorMsg = GetPushErrorMsg(iErroCode);
		short iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		subHead.sub_length = sizeof(int) + sizeof(short) + iStrLength + length;

		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&iErroCode, sizeof(int));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
		writer->Push_back((void*)data, length);
		WriteRespData(accHead, writer);
		return false;
	}
}

// 解析android用户请求
bool
HqLinkNode::ParseAndroidUserRequest(const struct ACC_CMDHEAD *accCmd, struct sub_head *pCmdHead,
				const UserLogInfor *pUserInfo,
				const void *data, const int dataLen,
				CBuffWriter *writer)
{
	// 获取用户管理
	CAndroidPushUserManage *pUserManage =
		CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
	CAndroidPushUser *pUser = NULL;
	unsigned int uUserIndex = (unsigned int)-1;
	int iRes = -1;
	const char *pErrorMsg = NULL;
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;

	// 处理用户信息
	if (0 == pCmdHead->sub_extend)
	{
		// 尝试从内存查找
		uUserIndex = pUserManage->FindFromNodeMark(pUserInfo->m_strUserID, pUserInfo->m_cPlatformCode);
		if ((unsigned int)-1 != uUserIndex)
		{
			pUser = pUserManage->GetUser(uUserIndex);
			pCmdHead->sub_extend = uUserIndex;
			
			if (PU_DEAD == pUser->GetActiveProperty())
			{
				if (0 > DistributeUserToCalThread(uUserIndex, pUser))
				{
					iRes = -29;
					goto END;
				}
			}
		}
		else
		{
			// 尝试从库中加载
			CAndroidPushUser user;
			iRes = pUserManage->TryReadUserFromDb(pUserInfo->m_strUserID, pUserInfo->m_cPlatformCode, &user);
			if (0 < iRes)
			{
				iRes = pUserManage->AddNewNode(user);
				if (iRes < 0)
				{
					iRes = -28;
					ERROR("AddNewNode failed.[%d]", pUserManage->GetManageStatus());
					goto END;
				}
				else
				{
					uUserIndex = (unsigned int)iRes;
					pCmdHead->sub_extend = uUserIndex;
					pUser = pUserManage->GetUser(uUserIndex);
					pUserManage->UpdateOccupyProperty(uUserIndex);
					pUser->SetOperDbProperty(PU_DB_NULL);

					// 更新属性
					pUser->SetUserThreadNum(DistributeUserThread(uUserIndex));
					pUser->SetUserHqLinkIndex(GetNodeIndex());
					pUser->SetUserServerCodeNum(GetServerCodeNum());
					
					// 读取预警记录
					if (0 >= pUser->ReloadEWarningFromDb())
					{
						pUser->SetHaveSetPrivateWarningProperty(PU_NO_VALID_EARLY_WARNING_SET);
					}
					
					// 读取自选股
					pUser->TryLoadUserSelfStkList(uUserIndex);

					// 清空原有历史记录
					CPushMsgHistoryManage::ClearNodeMsgHistory(pUserInfo->m_cPlatformCode, uUserIndex);
					pUser->ReloadPushHistoryFromDb(uUserIndex);
					
					// 分发到对应的线程
					if (0 > DistributeUserToCalThread(uUserIndex, pUser))
					{
						iRes = -29;
						goto END;
					}
				}
			}
			else
			{
				if (-1 == iRes)
				{
					// 新用户
					CAndroidPushUser user(pUserInfo, accCmd->m_nExpandInfo);
					iRes = pUserManage->AddNewNode(user);
					if (iRes < 0)
					{
						iRes = -28;
						ERROR("AddNewNode failed.[%d]", pUserManage->GetManageStatus());
						goto END;
					}
					else
					{
						uUserIndex = (unsigned int)iRes;
						pCmdHead->sub_extend = uUserIndex;
						pUser = pUserManage->GetUser(uUserIndex);
						pUserManage->UpdateOccupyProperty(uUserIndex);
						pUser->SetOperDbProperty(PU_DB_ADD);

						// 更新属性
						pUser->UpdateUserRegTime(GetNowTime());
						pUser->SetUserThreadNum(DistributeUserThread(uUserIndex));
						pUser->SetUserHqLinkIndex(GetNodeIndex());
						pUser->SetUserServerCodeNum(GetServerCodeNum());
						
						// 清空原有历史记录
						CPushMsgHistoryManage::ClearNodeMsgHistory(pUserInfo->m_cPlatformCode, uUserIndex);

						// 分发到对应的线程
						if (0 > DistributeUserToCalThread(uUserIndex, pUser))
						{
							iRes = -29;
							goto END;
						}
					}
				}
				else
				{
					// 加载用户失败
					iRes = -34;
					goto END;
				}
			}
		}
	}
	else
	{
		// 已有用户
		uUserIndex = pCmdHead->sub_extend;
		pUser = pUserManage->GetUser(uUserIndex);

		// 校验用户ID
		if (!MatchFoundUser(pUserInfo, pUser))
		{
			pUser = NULL;
		}
		
		if (NULL != pUser && PU_DEAD == pUser->GetActiveProperty())
		{
			if (0 > DistributeUserToCalThread(uUserIndex, pUser))
			{
				iRes = -29;
				goto END;
			}
		}
	}

	if (NULL == pUser)
	{
		DEBUG("can not find matched user[%s %u]. verify again", pUserInfo->m_strUserID, pCmdHead->sub_extend);
		goto ERROR_MATCH;
	}

	// 申请操作权限
	bmuGot = false;
	uObjID = pNodeThreadMutex->GenerateObjID(uUserIndex, pUserInfo->m_cPlatformCode);
	muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
	if (LOCK_REQUIRED == muRes)
	{
		if (pNodeThreadMutex->AskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
		{
			bmuGot = true;
		}
	}

	if (LOCK_SHARED == muRes
		|| (LOCK_REQUIRED == muRes && bmuGot))
	{
		// 更新用户扩展信息
		pUser->UpdatePushUserExpandInfo(accCmd->m_nExpandInfo);
		// 更新节点
		pUser->SetUserHqLinkIndex(GetNodeIndex());
		// 更新版本
		pUser->SetUserLocalVersion(pUserInfo->m_uLocalVersion);

		// 处理请求数据
		iRes = pUser->ProcessUserRequest(pCmdHead, pUserInfo, (void*)data,
						dataLen, writer);
	}
	else
	{
		ERROR("control error.");
	}

	// 提交权限
	if (bmuGot)
	{
		pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
	}

	if (0 != iRes)
	{
		DEBUG("pPushUser->ProcessUserRequest error.[%d]", iRes);
	}

	return true;

ERROR_MATCH:
	{
		sub_head subHead = {0};
		if (NULL != pCmdHead)
		{
			memcpy(&subHead, pCmdHead, sizeof(sub_head));
			subHead.sub_extend = 0;
		}
		subHead.sub_type = ST_REVERIFY;
		char cMark = 0;

		subHead.sub_length = sizeof(char);
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));

		WriteRespData(accCmd, writer);
		return false;
	}

END:
	{
		sub_head subHead = {0};
		if (NULL != pCmdHead)
		{
			memcpy(&subHead, pCmdHead, sizeof(sub_head));
		}
		subHead.sub_type = ST_ERROR;

		int iErroCode = iRes * -1;
		pErrorMsg = GetPushErrorMsg(iErroCode);
		short iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		subHead.sub_length = sizeof(int) + sizeof(short) + iStrLength + dataLen;

		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&iErroCode, sizeof(int));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
		writer->Push_back((void*)data, dataLen);
		WriteRespData(accCmd, writer);
		return false;
	}
}

// 解析Ios用户请求
bool
HqLinkNode::ParseIosUserRequest(const struct ACC_CMDHEAD *accCmd, struct sub_head *pCmdHead,
			const UserLogInfor *pUserInfo,
			const void *data, const int dataLen,
			CBuffWriter *writer)
{
	// 获取用户管理
	CIosPushUserManage *pUserManage =
		CPushUserManage::GetPlatFormUserManage<CIosPushUserManage>(PFCC_IOS);
	CIphonePushUser *pUser = NULL;
	unsigned int uUserIndex = (unsigned int)-1;
	int iRes = -1;
	const char *pErrorMsg = NULL;
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;

	// 处理用户信息
	if (0 == pCmdHead->sub_extend)
	{
		// 尝试从内存查找
		uUserIndex = pUserManage->FindFromNodeMark(pUserInfo->m_strUserID, pUserInfo->m_cPlatformCode);
		if ((unsigned int)-1 != uUserIndex)
		{
			pUser = pUserManage->GetUser(uUserIndex);
			pCmdHead->sub_extend = uUserIndex;
			
			if (PU_DEAD == pUser->GetActiveProperty())
			{
				if (0 > DistributeUserToCalThread(uUserIndex, pUser))
				{
					iRes = -29;
					goto END;
				}
			}
		}
		else
		{
			// 尝试从库中加载
			CIphonePushUser user;
			iRes = pUserManage->TryReadUserFromDb(pUserInfo->m_strUserID, pUserInfo->m_cPlatformCode, &user);
			if (0 < iRes)
			{
				iRes = pUserManage->AddNewNode(user);
				if (iRes < 0)
				{
					iRes = -28;
					ERROR("AddNewNode failed.[%d]", pUserManage->GetManageStatus());
					goto END;
				}
				else
				{
					uUserIndex = (unsigned int)iRes;
					pCmdHead->sub_extend = uUserIndex;
					pUser = pUserManage->GetUser(uUserIndex);
					pUserManage->UpdateOccupyProperty(uUserIndex);
					pUser->SetOperDbProperty(PU_DB_NULL);

					// 更新属性
					pUser->SetUserThreadNum(DistributeUserThread(uUserIndex));
					pUser->SetUserHqLinkIndex(GetNodeIndex());
					pUser->SetUserServerCodeNum(GetServerCodeNum());
					
					// 读取预警记录
					if (0 >= pUser->ReloadEWarningFromDb())
					{
						pUser->SetHaveSetPrivateWarningProperty(PU_NO_VALID_EARLY_WARNING_SET);
					}
					
					// 读取自选股
					pUser->TryLoadUserSelfStkList(uUserIndex);

					// 清空原有历史记录
					CPushMsgHistoryManage::ClearNodeMsgHistory(pUserInfo->m_cPlatformCode, uUserIndex);
					pUser->ReloadPushHistoryFromDb(uUserIndex);
					
					// 分发到对应的线程
					if (0 > DistributeUserToCalThread(uUserIndex, pUser))
					{
						iRes = -29;
						goto END;
					}
				}
			}
			else
			{
				if (-1 == iRes)
				{
					// 新用户
					CIphonePushUser user(pUserInfo, accCmd->m_nExpandInfo);
					iRes = pUserManage->AddNewNode(user);
					if (iRes < 0)
					{
						iRes = -28;
						ERROR("AddNewNode failed.[%d]", pUserManage->GetManageStatus());
						goto END;
					}
					else
					{
						uUserIndex = (unsigned int)iRes;
						pCmdHead->sub_extend = uUserIndex;
						pUser = pUserManage->GetUser(uUserIndex);
						pUserManage->UpdateOccupyProperty(uUserIndex);
						pUser->SetOperDbProperty(PU_DB_ADD);

						// 更新属性
						pUser->UpdateUserRegTime(GetNowTime());
						pUser->SetUserThreadNum(DistributeUserThread(uUserIndex));
						pUser->SetUserHqLinkIndex(GetNodeIndex());
						pUser->SetUserServerCodeNum(GetServerCodeNum());
						
						// 清空原有历史记录
						CPushMsgHistoryManage::ClearNodeMsgHistory(pUserInfo->m_cPlatformCode, uUserIndex);

						// 分发到对应的线程
						if (0 > DistributeUserToCalThread(uUserIndex, pUser))
						{
							iRes = -29;
							goto END;
						}
					}
				}
				else
				{
					// 加载用户失败
					iRes = -34;
					goto END;
				}
			}
		}
	}
	else
	{
		// 已有用户
		uUserIndex = pCmdHead->sub_extend;
		pUser = pUserManage->GetUser(uUserIndex);

		// 校验用户ID
		if (!MatchFoundUser(pUserInfo, pUser))
		{
			pUser = NULL;
		}
		
		if (NULL != pUser && PU_DEAD == pUser->GetActiveProperty())
		{
			if (0 > DistributeUserToCalThread(uUserIndex, pUser))
			{
				iRes = -29;
				goto END;
			}
		}
	}

	if (NULL == pUser)
	{
		DEBUG("can not find matched user[%s %u]. verify again", pUserInfo->m_strUserID, pCmdHead->sub_extend);
		goto ERROR_MATCH;
	}

	// 申请操作权限
	bmuGot = false;
	uObjID = pNodeThreadMutex->GenerateObjID(uUserIndex, pUserInfo->m_cPlatformCode);
	muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
	if (LOCK_REQUIRED == muRes)
	{
		if (pNodeThreadMutex->AskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
		{
			bmuGot = true;
		}
	}

	if (LOCK_SHARED == muRes
		|| (LOCK_REQUIRED == muRes && bmuGot))
	{
		// 更新用户扩展信息
		pUser->UpdatePushUserExpandInfo(accCmd->m_nExpandInfo);
		// 更新节点
		pUser->SetUserHqLinkIndex(GetNodeIndex());
		// 更新版本
		pUser->SetUserLocalVersion(pUserInfo->m_uLocalVersion);

		// 处理请求数据
		iRes = pUser->ProcessUserRequest(pCmdHead, pUserInfo, (void*)data,
						dataLen, writer);
	}
	else
	{
		ERROR("control error.");
	}

	// 提交权限
	if (bmuGot)
	{
		pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
	}

	if (0 != iRes)
	{
		DEBUG("pPushUser->ProcessUserRequest error.[%d]", iRes);
	}

	return true;

ERROR_MATCH:
	{
		sub_head subHead = {0};
		if (NULL != pCmdHead)
		{
			memcpy(&subHead, pCmdHead, sizeof(sub_head));
			subHead.sub_extend = 0;
		}
		subHead.sub_type = ST_REVERIFY;
		char cMark = 0;

		subHead.sub_length = sizeof(char);
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));

		WriteRespData(accCmd, writer);
		return false;
	}

END:
	{
		sub_head subHead = {0};
		if (NULL != pCmdHead)
		{
			memcpy(&subHead, pCmdHead, sizeof(sub_head));
		}
		subHead.sub_type = ST_ERROR;

		int iErroCode = iRes * -1;
		pErrorMsg = GetPushErrorMsg(iErroCode);
		short iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		subHead.sub_length = sizeof(int) + sizeof(short) + iStrLength + dataLen;

		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&iErroCode, sizeof(int));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
		writer->Push_back((void*)data, dataLen);
		WriteRespData(accCmd, writer);
		return false;
	}
}

// 解析Wp7用户请求
bool
HqLinkNode::ParseWp7UserRequest(const struct ACC_CMDHEAD *accCmd, struct sub_head *pCmdHead,
			const UserLogInfor *pUserInfo,
			const void *data, const int dataLen,
			CBuffWriter *writer)
{
	// 获取用户管理
	CWp7PushUserManage *pUserManage =
		CPushUserManage::GetPlatFormUserManage<CWp7PushUserManage>(PFCC_WP7);
	CWp7PushUser *pUser = NULL;
	unsigned int uUserIndex = (unsigned int)-1;
	int iRes = -1;
	const char *pErrorMsg = NULL;
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;

	// 处理用户信息
	if (0 == pCmdHead->sub_extend)
	{
		// 尝试从内存查找
		uUserIndex = pUserManage->FindFromNodeMark(pUserInfo->m_strUserID, pUserInfo->m_cPlatformCode);
		if ((unsigned int)-1 != uUserIndex)
		{
			pUser = pUserManage->GetUser(uUserIndex);
			pCmdHead->sub_extend = uUserIndex;
			
			if (PU_DEAD == pUser->GetActiveProperty())
			{
				if (0 > DistributeUserToCalThread(uUserIndex, pUser))
				{
					iRes = -29;
					goto END;
				}
			}
		}
		else
		{
			CWp7PushUser user;
			iRes = pUserManage->TryReadUserFromDb(pUserInfo->m_strUserID, pUserInfo->m_cPlatformCode, &user);
			if (0 < iRes)
			{
				iRes = pUserManage->AddNewNode(user);
				if (iRes < 0)
				{
					iRes = -28;
					ERROR("AddNewNode failed.[%d]", pUserManage->GetManageStatus());
					goto END;
				}
				else
				{
					uUserIndex = (unsigned int)iRes;
					pCmdHead->sub_extend = uUserIndex;
					pUser = pUserManage->GetUser(uUserIndex);
					pUserManage->UpdateOccupyProperty(uUserIndex);
					pUser->SetOperDbProperty(PU_DB_NULL);

					// 更新属性
					pUser->SetUserThreadNum(DistributeUserThread(uUserIndex));
					pUser->SetUserHqLinkIndex(GetNodeIndex());
					pUser->SetUserServerCodeNum(GetServerCodeNum());
					
					// 读取预警记录
					if (0 >= pUser->ReloadEWarningFromDb())
					{
						pUser->SetHaveSetPrivateWarningProperty(PU_NO_VALID_EARLY_WARNING_SET);
					}
					
					// 读取自选股
					pUser->TryLoadUserSelfStkList(uUserIndex);

					// 清空原有历史记录
					CPushMsgHistoryManage::ClearNodeMsgHistory(pUserInfo->m_cPlatformCode, uUserIndex);
					pUser->ReloadPushHistoryFromDb(uUserIndex);
					
					// 分发到对应的线程
					if (0 > DistributeUserToCalThread(uUserIndex, pUser))
					{
						iRes = -29;
						goto END;
					}
				}
			}
			else
			{
				if (-1 == iRes)
				{
					// 新用户
					CWp7PushUser user(pUserInfo, accCmd->m_nExpandInfo);
					iRes = pUserManage->AddNewNode(user);
					if (iRes < 0)
					{
						iRes = -28;
						ERROR("AddNewNode failed.[%d]", pUserManage->GetManageStatus());
						goto END;
					}
					else
					{
						uUserIndex = (unsigned int)iRes;
						pCmdHead->sub_extend = uUserIndex;
						pUser = pUserManage->GetUser(uUserIndex);
						pUserManage->UpdateOccupyProperty(uUserIndex);
						pUser->SetOperDbProperty(PU_DB_ADD);

						// 更新属性
						pUser->UpdateUserRegTime(GetNowTime());
						pUser->SetUserThreadNum(DistributeUserThread(uUserIndex));
						pUser->SetUserHqLinkIndex(GetNodeIndex());
						pUser->SetUserServerCodeNum(GetServerCodeNum());
						
						// 清空原有历史记录
						CPushMsgHistoryManage::ClearNodeMsgHistory(pUserInfo->m_cPlatformCode, uUserIndex);

						// 分发到对应的线程
						if (0 > DistributeUserToCalThread(uUserIndex, pUser))
						{
							iRes = -29;
							goto END;
						}
					}
				}
				else
				{
					// 加载用户失败
					iRes = -34;
					goto END;
				}
			}
		}
	}
	else
	{
		// 已有用户
		uUserIndex = pCmdHead->sub_extend;
		pUser = pUserManage->GetUser(uUserIndex);

		// 校验用户ID
		if (!MatchFoundUser(pUserInfo, pUser))
		{
			pUser = NULL;
		}
		
		if (NULL != pUser && PU_DEAD == pUser->GetActiveProperty())
		{
			if (0 > DistributeUserToCalThread(uUserIndex, pUser))
			{
				iRes = -29;
				goto END;
			}
		}
	}

	if (NULL == pUser)
	{
		DEBUG("can not find matched user[%s %u]. verify again", pUserInfo->m_strUserID, pCmdHead->sub_extend);
		goto ERROR_MATCH;
	}

	// 申请操作权限
	bmuGot = false;
	uObjID = pNodeThreadMutex->GenerateObjID(uUserIndex, pUserInfo->m_cPlatformCode);
	muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
	if (LOCK_REQUIRED == muRes)
	{
		if (pNodeThreadMutex->AskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
		{
			bmuGot = true;
		}
	}

	if (LOCK_SHARED == muRes
		|| (LOCK_REQUIRED == muRes && bmuGot))
	{
		// 更新用户扩展信息
		pUser->UpdatePushUserExpandInfo(accCmd->m_nExpandInfo);
		// 更新节点
		pUser->SetUserHqLinkIndex(GetNodeIndex());
		// 更新版本
		pUser->SetUserLocalVersion(pUserInfo->m_uLocalVersion);

		// 处理请求数据
		iRes = pUser->ProcessUserRequest(pCmdHead, pUserInfo, (void*)data,
						dataLen, writer);
	}
	else
	{
		ERROR("control error.");
	}

	// 提交权限
	if (bmuGot)
	{
		pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
	}

	if (0 != iRes)
	{
		DEBUG("pPushUser->ProcessUserRequest error.[%d]", iRes);
	}

	return true;

ERROR_MATCH:
	{
		sub_head subHead = {0};
		if (NULL != pCmdHead)
		{
			memcpy(&subHead, pCmdHead, sizeof(sub_head));
			subHead.sub_extend = 0;
		}
		subHead.sub_type = ST_REVERIFY;
		char cMark = 0;

		subHead.sub_length = sizeof(char);
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));

		WriteRespData(accCmd, writer);
		return false;
	}

END:
	{
		sub_head subHead = {0};
		if (NULL != pCmdHead)
		{
			memcpy(&subHead, pCmdHead, sizeof(sub_head));
		}
		subHead.sub_type = ST_ERROR;

		int iErroCode = iRes * -1;
		pErrorMsg = GetPushErrorMsg(iErroCode);
		short iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		subHead.sub_length = sizeof(int) + sizeof(short) + iStrLength + dataLen;

		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&iErroCode, sizeof(int));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
		writer->Push_back((void*)data, dataLen);
		WriteRespData(accCmd, writer);
		return false;
	}
}

// 解析Win8用户请求
bool
HqLinkNode::ParseWin8UserRequest(const struct ACC_CMDHEAD *accCmd, struct sub_head *pCmdHead,
			const UserLogInfor *pUserInfo,
			const void *data, const int dataLen,
			CBuffWriter *writer)
{
	// 获取用户管理
	CWin8PushUserManage *pUserManage =
		CPushUserManage::GetPlatFormUserManage<CWin8PushUserManage>(PFCC_WIN8);
	CWin8PushUser *pUser = NULL;
	unsigned int uUserIndex = (unsigned int)-1;
	int iRes = -1;
	const char *pErrorMsg = NULL;
	CNodeThreadMutex *pNodeThreadMutex = ThreadsManage::GetNodeThreadMutexManage();
	LockResType muRes = LOCK_FAILED;
	bool bmuGot = false;
	unsigned int uObjID = 0;

	// 处理用户信息
	if (0 == pCmdHead->sub_extend)
	{
		// 尝试从内存查找
		uUserIndex = pUserManage->FindFromNodeMark(pUserInfo->m_strUserID, pUserInfo->m_cPlatformCode);
		if ((unsigned int)-1 != uUserIndex)
		{
			pUser = pUserManage->GetUser(uUserIndex);
			pCmdHead->sub_extend = uUserIndex;
			
			if (PU_DEAD == pUser->GetActiveProperty())
			{
				if (0 > DistributeUserToCalThread(uUserIndex, pUser))
				{
					iRes = -29;
					goto END;
				}
			}
		}
		else
		{
			CWin8PushUser user;
			iRes = pUserManage->TryReadUserFromDb(pUserInfo->m_strUserID, pUserInfo->m_cPlatformCode, &user);
			if (0 < iRes)
			{
				iRes = pUserManage->AddNewNode(user);
				if (iRes < 0)
				{
					iRes = -28;
					ERROR("AddNewNode failed.[%d]", pUserManage->GetManageStatus());
					goto END;
				}
				else
				{
					uUserIndex = (unsigned int)iRes;
					pCmdHead->sub_extend = uUserIndex;
					pUser = pUserManage->GetUser(uUserIndex);
					pUserManage->UpdateOccupyProperty(uUserIndex);
					pUser->SetOperDbProperty(PU_DB_NULL);

					// 更新属性
					pUser->SetUserThreadNum(DistributeUserThread(uUserIndex));
					pUser->SetUserHqLinkIndex(GetNodeIndex());
					pUser->SetUserServerCodeNum(GetServerCodeNum());
					
					// 读取预警记录
					if (0 >= pUser->ReloadEWarningFromDb())
					{
						pUser->SetHaveSetPrivateWarningProperty(PU_NO_VALID_EARLY_WARNING_SET);
					}
					
					// 读取自选股
					pUser->TryLoadUserSelfStkList(uUserIndex);

					// 清空原有历史记录
					CPushMsgHistoryManage::ClearNodeMsgHistory(pUserInfo->m_cPlatformCode, uUserIndex);
					pUser->ReloadPushHistoryFromDb(uUserIndex);
					
					// 分发到对应的线程
					if (0 > DistributeUserToCalThread(uUserIndex, pUser))
					{
						iRes = -29;
						goto END;
					}
				}
			}
			else
			{
				if (-1 == iRes)
				{
					// 新用户
					CWin8PushUser user(pUserInfo, accCmd->m_nExpandInfo);
					iRes = pUserManage->AddNewNode(user);
					if (iRes < 0)
					{
						iRes = -28;
						ERROR("AddNewNode failed.[%d]", pUserManage->GetManageStatus());
						goto END;
					}
					else
					{
						uUserIndex = (unsigned int)iRes;
						pCmdHead->sub_extend = uUserIndex;
						pUser = pUserManage->GetUser(uUserIndex);
						pUserManage->UpdateOccupyProperty(uUserIndex);
						pUser->SetOperDbProperty(PU_DB_ADD);

						// 更新属性
						pUser->UpdateUserRegTime(GetNowTime());
						pUser->SetUserThreadNum(DistributeUserThread(uUserIndex));
						pUser->SetUserHqLinkIndex(GetNodeIndex());
						pUser->SetUserServerCodeNum(GetServerCodeNum());
						
						// 清空原有历史记录
						CPushMsgHistoryManage::ClearNodeMsgHistory(pUserInfo->m_cPlatformCode, uUserIndex);

						// 分发到对应的线程
						if (0 > DistributeUserToCalThread(uUserIndex, pUser))
						{
							iRes = -29;
							goto END;
						}
					}
				}
				else
				{
					// 加载用户失败
					iRes = -34;
					goto END;
				}
			}
		}
	}
	else
	{
		// 已有用户
		uUserIndex = pCmdHead->sub_extend;
		pUser = pUserManage->GetUser(uUserIndex);

		// 校验用户ID
		if (!MatchFoundUser(pUserInfo, pUser))
		{
			pUser = NULL;
		}
		
		if (NULL != pUser && PU_DEAD == pUser->GetActiveProperty())
		{
			if (0 > DistributeUserToCalThread(uUserIndex, pUser))
			{
				iRes = -29;
				goto END;
			}
		}
	}

	if (NULL == pUser)
	{
		DEBUG("can not find matched user[%s %u]. verify again", pUserInfo->m_strUserID, pCmdHead->sub_extend);
		goto ERROR_MATCH;
	}

	// 申请操作权限
	bmuGot = false;
	uObjID = pNodeThreadMutex->GenerateObjID(uUserIndex, pUserInfo->m_cPlatformCode);
	muRes = pNodeThreadMutex->ConfigLock(pUser->GetUserThreadNum(), uObjID);
	if (LOCK_REQUIRED == muRes)
	{
		if (pNodeThreadMutex->AskOperateAuthority(pUser->GetUserThreadNum(), uObjID))
		{
			bmuGot = true;
		}
	}

	if (LOCK_SHARED == muRes
		|| (LOCK_REQUIRED == muRes && bmuGot))
	{
		// 更新用户扩展信息
		pUser->UpdatePushUserExpandInfo(accCmd->m_nExpandInfo);
		// 更新节点
		pUser->SetUserHqLinkIndex(GetNodeIndex());
		// 更新版本
		pUser->SetUserLocalVersion(pUserInfo->m_uLocalVersion);

		// 处理请求数据
		iRes = pUser->ProcessUserRequest(pCmdHead, pUserInfo, (void*)data,
						dataLen, writer);
	}
	else
	{
		ERROR("control error.");
	}

	// 提交权限
	if (bmuGot)
	{
		pNodeThreadMutex->CommitAuthority(pUser->GetUserThreadNum());
	}

	if (0 != iRes)
	{
		DEBUG("pPushUser->ProcessUserRequest error.[%d]", iRes);
	}

	return true;

ERROR_MATCH:
	{
		sub_head subHead = {0};
		if (NULL != pCmdHead)
		{
			memcpy(&subHead, pCmdHead, sizeof(sub_head));
			subHead.sub_extend = 0;
		}
		subHead.sub_type = ST_REVERIFY;
		char cMark = 0;

		subHead.sub_length = sizeof(char);
		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&cMark, sizeof(char));

		WriteRespData(accCmd, writer);
		return false;
	}

END:
	{
		sub_head subHead = {0};
		if (NULL != pCmdHead)
		{
			memcpy(&subHead, pCmdHead, sizeof(sub_head));
		}
		subHead.sub_type = ST_ERROR;

		int iErroCode = iRes * -1;
		pErrorMsg = GetPushErrorMsg(iErroCode);
		short iStrLength = (NULL == pErrorMsg ? 0 : strlen(pErrorMsg));
		subHead.sub_length = sizeof(int) + sizeof(short) + iStrLength + dataLen;

		writer->Push_back(&subHead, sizeof(sub_head));
		writer->Push_back(&iErroCode, sizeof(int));
		writer->Push_back(&iStrLength, sizeof(short));
		if (iStrLength > 0)
		{
			writer->Push_back((char*)pErrorMsg, iStrLength);
		}
		writer->Push_back((void*)data, dataLen);
		WriteRespData(accCmd, writer);
		return false;
	}
}

// 解析平台用户请求
bool
HqLinkNode::ParsePlatformUserRequest(const struct ACC_CMDHEAD *accCmd, struct sub_head *pCmdHead,
			const UserLogInfor *pUserInfo,
			const void *data, const int dataLen,
			CBuffWriter *writer)
{
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(pUserInfo->m_cPlatformCode);
	
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_WP7:
			return ParseWp7UserRequest(accCmd, pCmdHead, pUserInfo,
				data, dataLen, writer);

		case PFCC_ANDROID:
			return ParseAndroidUserRequest(accCmd, pCmdHead, pUserInfo,
				data, dataLen, writer);

		case PFCC_IOS:
			return ParseIosUserRequest(accCmd, pCmdHead, pUserInfo,
				data, dataLen, writer);
				
		case PFCC_WIN8:
			return ParseWin8UserRequest(accCmd, pCmdHead, pUserInfo,
				data, dataLen, writer);
		default:
			break;
	}

	return false;
}

// 解析用户信息
int
HqLinkNode::ParseUserInfoMark(const void *data, const int length, UserLogInfor *pUserInfor, int &TotalLen)
{
	TotalLen = 0;
	if (NULL == data || NULL == pUserInfor)
	{
		return -1;
	}

	short iStrLength = 0;
	CBuffReader reader((void*)data, length, 0);

	// 取用户名
	iStrLength = reader.ReadShort();
	TotalLen += sizeof(short);
	if (iStrLength < 0 || iStrLength > (short)reader.GetUnReadLen()
		|| iStrLength > MAX_USER_NAME_LEN)
	{
		DEBUG("Data error.");
		return -2;
	}
	reader.ReadString(iStrLength, pUserInfor->m_strUserName, MAX_USER_NAME_LEN);
	TotalLen += iStrLength;

	// 取用户密码
	iStrLength = reader.ReadShort();
	TotalLen += sizeof(short);
	if (iStrLength < 0 || iStrLength > (short)reader.GetUnReadLen()
		|| iStrLength > MAX_USER_PWD_LEN)
	{
		DEBUG("Data error.");
		return -3;
	}
	reader.ReadString(iStrLength, pUserInfor->m_strUserPwd, MAX_USER_PWD_LEN);
	TotalLen += iStrLength;

	// 取手机号码
	iStrLength = reader.ReadShort();
	TotalLen += sizeof(short);
	if (iStrLength < 0 || iStrLength > (short)reader.GetUnReadLen()
		|| iStrLength > MAX_TEL_NUMBER_LEN)
	{
		DEBUG("Data error.");
		return -4;
	}
	reader.ReadString(iStrLength, pUserInfor->m_strTelNum, MAX_TEL_NUMBER_LEN);
	TotalLen += iStrLength;

	// 取手机ID
	iStrLength = reader.ReadShort();
	TotalLen += sizeof(short);
	if (iStrLength < 0 || iStrLength > (short)reader.GetUnReadLen()
		|| iStrLength > MAX_TEL_ID_LEN)
	{
		DEBUG("Data error.");
		return -5;
	}
	reader.ReadString(iStrLength, pUserInfor->m_strTelID, MAX_TEL_ID_LEN);
	TotalLen += iStrLength;

	// 取客户端标识
	iStrLength = reader.ReadShort();
	TotalLen += sizeof(short);
	if (iStrLength <= 0 || iStrLength > (short)reader.GetUnReadLen()
		|| iStrLength > MAX_USER_ID_LEN)
	{
		DEBUG("Data error.");
		return -6;
	}
	reader.ReadString(iStrLength, pUserInfor->m_strUserID, MAX_USER_ID_LEN);
	TotalLen += iStrLength;
	if (IsEmptyString(pUserInfor->m_strUserID))
	{
		DEBUG("Data error.");
		return -6;
	}

	// 平台
	if (sizeof(char) > reader.GetUnReadLen())
	{
		DEBUG("Data error.");
		return -7;
	}
	pUserInfor->m_cPlatformCode = reader.ReadByte();
	TotalLen += sizeof(char);

	// 版本
	iStrLength = reader.ReadShort();
	TotalLen += sizeof(short);
	if (iStrLength < 0 || iStrLength > (short)reader.GetUnReadLen()
		|| iStrLength > MAX_LOCAL_VERSION)
	{
		DEBUG("Data error.");
		return -8;
	}
	char strLocalVersion[MAX_LOCAL_VERSION + 1] = {0};
	reader.ReadString(iStrLength, strLocalVersion, MAX_LOCAL_VERSION);
	pUserInfor->m_uLocalVersion = (unsigned short)(round(atof(strLocalVersion) * 100));
	TotalLen += iStrLength;

	return 0;
}

// 发送数据
bool
HqLinkNode::SendData(const void *data, const int length)
{
	int iRes = Send(GetFd(), data, length);
	if (0 == iRes)
	{
		DEBUG("[IP=%s fd = %d]Send error.[%s]", IP, GetFd(), strerror(errno));
		SetState(HQLK_CLOSE);
		return false;
	}
	else if (-1 == iRes)
	{
		DEBUG("[IP=%s fd = %d]Send error.[%s]", IP, GetFd(), strerror(errno));
		return false;
	}

	return true;
}

// 处理应用服务器登录
bool
HqLinkNode::ProcessServerLogin(const void *data, const int length, bool RequestRes,
	CBuffWriter *writer)
{
	CBuffReader reader((void*)data, length, 0);
	ACC_SERVERLOGIN *pLogin = reader.ReadPoiner<ACC_SERVERLOGIN>();
	bool bAcceptConnect = true;
	if (NULL == pLogin)
	{
		DEBUG("can not parse ACC_SERVERLOGIN");
		bAcceptConnect = false;
	}
	else
	{
		if (0 != strncmp(pLogin->m_cValid, GetAppLogInValid(), AppLogInValidLen))
		{
			bAcceptConnect = false;
		}
	}

	// 如果验证出错的话，就关闭连接
	if (!bAcceptConnect)
	{
		ERROR("ProcessServerLogin failed.[(%s)-(%s)] close [%d]", pLogin->m_cValid, GetAppLogInValid(), GetFd());
		SetState(HQLK_CLOSE);
	}
	return bAcceptConnect;
}

// 处理行情心跳包
bool
HqLinkNode::ProcessKeepLive(const struct ACC_CMDHEAD *accHead, bool RequestRes, CBuffWriter *writer)
{
	writer->SeekToBegin();
	writer->Push_back((void*)accHead, sizeof(ACC_CMDHEAD));
	writer->SeekToEnd();

	return true;
}

// 发送请求的响应数据
int
HqLinkNode::WriteRespData(const struct ACC_CMDHEAD *accHead, CBuffWriter *writer)
{
	writer->SeekToBegin();

	// common head
	ACC_CMDHEAD CommonHead;
	CommonHead.m_wCmdType = accHead->m_wCmdType;
	CommonHead.m_wAttr = 0;
	CommonHead.m_nLen = writer->GetNewdataLen();
	CommonHead.m_nExpandInfo = accHead->m_nExpandInfo;

	writer->Push_set(&CommonHead, sizeof(ACC_CMDHEAD));
	writer->SeekToEnd();

	return m_pSendBuf->GetUsedMemoryLen();
}

// 是否匹配指定用户
bool
HqLinkNode::MatchFoundUser(const UserLogInfor *pUserInfor, const CPushUser *pUser)
{
	if (NULL == pUserInfor || NULL == pUser)
	{
		return false;
	}
	
	// 校验用户ID
	if ((0 != StrNoCaseCmp(pUserInfor->m_strUserID, pUser->GetUserID(), MAX_USER_ID_LEN)
			|| pUserInfor->m_cPlatformCode != pUser->m_cPlatformCode))
	{
		return false;
	}
	
	return true;
}

// 分发用户到计算线程
int
HqLinkNode::DistributeUserToCalThread(const unsigned int UserMapID, const CPushUser *pUser)
{
	short iTreadNum = DistributeUserThread(UserMapID);
	CalOperatingThread *pCurrentThread = ThreadsManage::GetIndexCalcThread(iTreadNum);
	if (NULL == pCurrentThread)
	{
		ERROR("Distribute User[%u] error", UserMapID);
		return -29;
	}

	if (PU_VALID_EARLY_WARNING_SET == pUser->GetHaveSetPrivateWarningProperty())
	{
		pCurrentThread->AddEWarningUser(UserMapID, pUser->GetUserPlatform());
	}
	pCurrentThread->AddActiveUser(UserMapID, pUser->GetUserPlatform());
	return 0;
}

// 发送推送数据
bool
HqLinkNode::SendPushData(const void *data, const int length)
{
	int iBufLen = 0;
	char *pReceiveBuf = m_pSendBuf->GetRemainMem(iBufLen);
	if (NULL == pReceiveBuf || iBufLen <= 0 || iBufLen < length)
	{
		DEBUG("Not enough memory.");
		return false;
	}
	CBuffWriter writer(pReceiveBuf, iBufLen, 0);
	if (!writer.Push_back((void*)data, length))
	{
		DEBUG("Push_back failed.");
		return false;
	}
	m_pSendBuf->SetRemainMemStart(writer.GetNewdataLen());
	return true;
}

// 发送缓存数据
int
HqLinkNode::SendLinkCacheData()
{
	if (NULL == m_pSendBuf)
	{
		return -1;
	}
	if (m_pSendBuf->GetUsedMemoryLen() <= 0)
	{
		return -2;
	}

	int iSendLen = 0;
	int iDataLen = 0;
	char *pData = m_pSendBuf->GetPosData(m_iLatestSendDataPos, iDataLen);
	if (NULL == pData || 0 >= iDataLen)
	{
		return -3;
	}

	if ((iSendLen = send(GetFd(), pData, iDataLen, 0)) <= 0)
	{
		if (EWOULDBLOCK != errno && EINTR != errno)
		{
			//连接需要关闭
			DEBUG("send error found [ip=%s errno=%s]", IP, strerror(errno));
			SetState(HQLK_CLOSE);
			m_iLatestSendDataPos = 0;
			m_pSendBuf->ClearUsedMem();
			return 0;
		}
		else
		{
			return -4;
		}
	}

	// 判断发送结果
	if (iSendLen < iDataLen)
	{
		DEBUG("Send Part[m_iLatestSendDataPos=%d iSendLen=%d iDataLen=%d]", 
			m_iLatestSendDataPos, iSendLen, iDataLen);
		m_iLatestSendDataPos += iSendLen;
	}
	else
	{
		m_iLatestSendDataPos = 0;
		m_pSendBuf->ClearUsedMem();
	}
	return iSendLen;
}

// 是否可以接收
bool
HqLinkNode::CanReceive() const
{
	if (HQLK_NORMAL != state)
	{
		return false;
	}

	if (m_iLatestSendDataPos > 0 && 0 != m_pSendBuf->GetLatestWarning())
	{
		return false;
	}

	if (0 != m_pReceiveBuf->GetLatestWarning())
	{
		return false;
	}

	return true;
}

// 是否可以发送数据
bool
HqLinkNode::CanSend()const
{
	if (HQLK_NORMAL != state)
	{
		return false;
	}

	if (m_pSendBuf->GetUsedMemoryLen() <= 0)
	{
		return false;
	}

	return true;
}

// 存入发送队列(0成功 -1失败 1已满)
int
HqLinkNode::PushToSendQueue(const unsigned char PlatFormCode, 
	const unsigned int UserMapID, const unsigned char MsgType)
{
	const size_t uMaxCacheCount = 1500000;
	
	if (m_SendMutex.try_lock())
	{
		if (m_SendQueue.size() >= uMaxCacheCount)
		{
			// Full
			goto CACHE_FULL;
		}
		
		COuterSendMark temMark(PlatFormCode, UserMapID, MsgType);
		m_SendQueue.push(temMark);
		
		m_SendMutex.unlock();
		return 0;
	}
	
	return -1;
	
CACHE_FULL:
	m_SendMutex.unlock();
	return 1;
}

// 处理推送消息的缓存
const int 
HqLinkNode::ProcessCacheQueue()
{
	if (m_SendQueue.empty())
	{
		return -1;
	}
	
	const unsigned int uMaxPerCycle = 20;
	unsigned int uExeCount = 0;
	if (m_SendMutex.lock())
	{
		if (m_SendQueue.empty())
		{
			m_SendMutex.unlock();
			return -1;
		}
		
		while(!m_SendQueue.empty())
		{
			COuterSendMark temMark = m_SendQueue.front();
			m_SendQueue.pop();
			
			if (ProcessSendMark(temMark) < 0)
			{
				uExeCount++;
			}
			if (uExeCount >= uMaxPerCycle)
			{
				break;
			}
		}
		
		m_SendMutex.unlock();
		return 0;
	}
	
	return -2;
}

// 处理发送
int
HqLinkNode::ProcessSendMark(COuterSendMark &item)
{
	const unsigned char cPlatFormCode = item.GetPlatFormCode();
	const unsigned int uUserMapId = item.GetUserMapID();
	unsigned short uPlatFormClassifiedCode = GetPlatformClassifiedMap(cPlatFormCode);
	
	switch(uPlatFormClassifiedCode)
	{
		case PFCC_ANDROID:
			{
				CAndroidPushUserManage *pUserManage =
						CPushUserManage::GetPlatFormUserManage<CAndroidPushUserManage>(PFCC_ANDROID);
				CAndroidPushUser *pUser = pUserManage->GetUser(uUserMapId);
				if (NULL == pUser || pUser->IsDead())
				{
					DEBUG("Invalid user.");
					return -2;
				}
				
				return pUser->SendOuterMsg(uUserMapId, item.GetMsgType());
			}
			break;
			
		default:
			ERROR("PlatFormClassifiedCode error:%d", uPlatFormClassifiedCode);
			break;
	}
	
	return -3;
}

