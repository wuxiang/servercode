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
	HQLK_NORMAL = 0,								// 状态正常
	HQLK_ERROR,										// 出现错误
	HQLK_TIMEOUT,									// 超时未活动
	HQLK_CLOSE,										// 需要关闭连接
	HQLK_EMPTY,										// 清空状态
};

// 行情连接管理
class HqLinkNode : public CNodeInterface<HqLinkNode>
{
private:
	char  m_strKey[MAX_LINK_KEY_LEN];						// 标识Key
	unsigned int fd;										// socket标识
	char IP[MAX_IP_LEN];									// Hq的Ip
	HqLinkState state;										// 链接状态
	time_t LastestAliveTime;								// 最近一次活动时间
	LargeMemoryCache *m_pReceiveBuf;						// 接收数据缓存
	LargeMemoryCache *m_pSendBuf;							// 发送数据缓存
	CThreadMutex m_SendMutex;								// 发送推送保护
	unsigned int m_iMarkIndex;								// 节点索引标识
	int m_iLatestSendDataPos;								// 最近的发送数据位置
	std::queue<COuterSendMark> m_SendQueue;					// 接收主动推送的数据缓存

private:
	void SetState(const enum HqLinkState);									// 设置链接状态
	int ProcessRecData(const void*, const int);								// 处理已经收到的数据

	bool ParseRequestData(const struct ACC_CMDHEAD*,						// 解析请求数据
					const void*, const int, CBuffWriter*);
	bool ParseUserRequestData(const struct ACC_CMDHEAD*,					// 解析用户请求数据
					const void*, const int, CBuffWriter*);
	int ParseUserInfoMark(const void*, const int,							// 解析用户信息
					UserLogInfor *, int&);
	
	// 解析android用户请求
	bool ParseAndroidUserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
	// 解析Ios用户请求
	bool ParseIosUserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
	// 解析Wp7用户请求
	bool ParseWp7UserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
					
	// 解析Win8用户请求
	bool ParseWin8UserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
	// 解析平台用户请求
	bool ParsePlatformUserRequest(const struct ACC_CMDHEAD*,
					struct sub_head*, const UserLogInfor*,
					const void*, const int,
					CBuffWriter*);
	
	bool SendData(const void*, const int);									// 发送数据
                                                            				
	bool ProcessServerLogin(const void*, 									// 处理应用服务器登录
					const int, bool, CBuffWriter*);		
	bool ProcessKeepLive(const struct ACC_CMDHEAD*,							// 处理行情心跳包
					bool, CBuffWriter*);			
	
	bool CloseLinkNode();													// 关闭节点
	int WriteRespData(const struct ACC_CMDHEAD*, 							// 添加响应请求数据
						CBuffWriter*);
	bool MatchFoundUser(const UserLogInfor*, const CPushUser*);				// 是否匹配指定用户
	int DistributeUserToCalThread(const unsigned int, const CPushUser*);	// 分发用户到计算线程
	int ProcessSendMark(COuterSendMark&);									// 处理发送

public:
	HqLinkNode(const unsigned int socketFd, const char *ip);
	HqLinkNode();
	~HqLinkNode();

// 继承接口
public:
	void ResetNode(const HqLinkNode &, const int);			// 重新构造
	void Clear();											// 清除处理
	const std::string GetMarkID()const;						// 获取标识ID
	bool IsDead();											// 是否已经无效
	unsigned int GetNodeIndex()								// 获取节点索引
	{
		return m_iMarkIndex;
	}
	
	void SetNodeIndex(const unsigned int index)				// 设置节点索引
	{
		m_iMarkIndex = index;
	}
	
	void SetMarkID(std::string &key)						// 设置标识字符串
	{
		StrCopy(m_strKey, key.c_str(), MAX_LINK_KEY_LEN);
	}
	
	void ExternalRelease()
	{
	}

// 公有函数
public:	
	const unsigned int GetFd() const						// 返回链接socket关键字
	{
		return fd;
	}

	const std::string GetIp()const							// 返回上级服务器IP地址
	{
		return IP;
	}

	const HqLinkState GetLinkState()const					// 返回节点状态
	{
		return state;
	}
	
	void UpdateLastestAliveTime();							// 更新节点最新活动时间
	int NodeReceiveProcess();								// 节点接收数据处理					
	bool SendPushData(const void*, const int);				// 发送推送数据
	
	int SendLinkCacheData();								// 发送缓存数据
	bool CanReceive() const;								// 是否可以接收
	bool CanSend()const;									// 是否可以发送数据
	
	int PushToSendQueue(const unsigned char, 				// 存入发送队列(0成功 -1失败 1已满)
			const unsigned int, const unsigned char);
	const int ProcessCacheQueue();							// 处理推送消息的缓存
};

#endif  /* _HQ_LINKE_NODE_H */

