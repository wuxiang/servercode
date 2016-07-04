#ifndef _HQ_LINKE_NODE_H
#define _HQ_LINKE_NODE_H

#include "../template/NodeInterface.h"
#include "../../../util/util.h"
#include "../../../util/common/common_types.h"
#include "../../../util/string/string_util.h"
#include "../template/ThreadMutex.h"
#include "../../data_type.h"
#include <string>
#include <queue>

class LargeMemoryCache;
struct UserLogInfor;
class CBuffWriter;
class CPushUser;
class COuterSendMark;

enum HqLinkState {
	HQLK_NORMAL = 0,								// ״̬����
	HQLK_ERROR,										// ���ִ���
	HQLK_TIMEOUT,									// ��ʱδ�
	HQLK_CLOSE,										// ��Ҫ�ر�����
	HQLK_EMPTY,										// ���״̬
};

// �������ӹ���
class HqLinkNode : public CNodeInterface<HqLinkNode>
{
private:
	char  m_strKey[MAX_LINK_KEY_LEN];						// ��ʶKey
	unsigned int fd;										// socket��ʶ
	char IP[MAX_IP_LEN];									// Hq��Ip
	HqLinkState state;										// ����״̬
	time_t LastestAliveTime;								// ���һ�λʱ��
	LargeMemoryCache *m_pReceiveBuf;						// �������ݻ���
	LargeMemoryCache *m_pSendBuf;							// �������ݻ���
	CThreadMutex m_SendMutex;								// �������ͱ���
	unsigned int m_iMarkIndex;								// �ڵ�������ʶ
	int m_iLatestSendDataPos;								// ����ķ�������λ��
	std::queue<COuterSendMark> m_SendQueue;					// �����������͵����ݻ���

private:
	void SetState(const enum HqLinkState);									// ��������״̬
	int ProcessRecData(const void*, const int);								// �����Ѿ��յ�������

	bool ParseRequestData(const struct ACC_CMDHEAD*,						// ������������
					const void*, const int, CBuffWriter*);
	bool ParseUserRequestData(const struct ACC_CMDHEAD*,					// �����û���������
					const void*, const int, CBuffWriter*);
	int ParseUserInfoMark(const void*, const int,							// �����û���Ϣ
					UserLogInfor *, int&);
	
	// ����android�û�����
	bool ParseAndroidUserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
	// ����Ios�û�����
	bool ParseIosUserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
	// ����Wp7�û�����
	bool ParseWp7UserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
					
	// ����Win8�û�����
	bool ParseWin8UserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
	// ����ƽ̨�û�����
	bool ParsePlatformUserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
	
	bool SendData(const void*, const int);									// ��������
                                                            				
	bool ProcessServerLogin(const void*, 									// ����Ӧ�÷�������¼
					const int, bool, CBuffWriter*);		
	bool ProcessKeepLive(const struct ACC_CMDHEAD*,							// ��������������
					bool, CBuffWriter*);			
	
	bool CloseLinkNode();													// �رսڵ�
	int WriteRespData(const struct ACC_CMDHEAD*, 							// �����Ӧ��������
						CBuffWriter*);
	bool MatchFoundUser(const UserLogInfor*, const CPushUser*);				// �Ƿ�ƥ��ָ���û�
	int DistributeUserToCalThread(const unsigned int, const CPushUser*);	// �ַ��û��������߳�
	int ProcessSendMark(COuterSendMark&);									// ������

public:
	HqLinkNode(const unsigned int socketFd, const char *ip);
	HqLinkNode();
	~HqLinkNode();

// �̳нӿ�
public:
	void ResetNode(const HqLinkNode &, const int);			// ���¹���
	void Clear();											// �������
	const std::string GetMarkID()const;						// ��ȡ��ʶID
	bool IsDead();											// �Ƿ��Ѿ���Ч
	unsigned int GetNodeIndex()								// ��ȡ�ڵ�����
	{
		return m_iMarkIndex;
	}
	
	void SetNodeIndex(const unsigned int index)				// ���ýڵ�����
	{
		m_iMarkIndex = index;
	}
	
	void SetMarkID(std::string &key)						// ���ñ�ʶ�ַ���
	{
		StrCopy(m_strKey, key.c_str(), MAX_LINK_KEY_LEN);
	}
	
	void ExternalRelease()
	{
	}

// ���к���
public:	
	const unsigned int GetFd() const						// ��������socket�ؼ���
	{
		return fd;
	}

	const std::string GetIp()const							// �����ϼ�������IP��ַ
	{
		return IP;
	}

	const HqLinkState GetLinkState()const					// ���ؽڵ�״̬
	{
		return state;
	}
	
	void UpdateLastestAliveTime();							// ���½ڵ����»ʱ��
	int NodeReceiveProcess();								// �ڵ�������ݴ���					
	bool SendPushData(const void*, const int);				// ������������
	
	int SendLinkCacheData();								// ���ͻ�������
	bool CanReceive() const;								// �Ƿ���Խ���
	bool CanSend()const;									// �Ƿ���Է�������
	
	int PushToSendQueue(const unsigned char, 				// ���뷢�Ͷ���(0�ɹ� -1ʧ�� 1����)
			const unsigned int, const unsigned char);
	const int ProcessCacheQueue();							// ����������Ϣ�Ļ���
};

#endif  /* _HQ_LINKE_NODE_H */

