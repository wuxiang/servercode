#ifndef _INCLUDE_WIN8_PUSH_USER_H
#define _INCLUDE_WIN8_PUSH_USER_H

#include "PushUser.h"

class WpReqTag;

class CWin8PushUser : public CPushUser
{
public:
	CWin8PushUser(void);
	CWin8PushUser(const UserLogInfor*, const unsigned int);
	~CWin8PushUser(void);

// 扩充数据
protected:
	char m_strPushToken[MAX_WIN8_PUSH_TOKEN_LEN + 1];					// 推送令牌

private:
	int ParseReq306(const sub_head *, const UserLogInfor*, 				// 处理306请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq1000(const sub_head *, const UserLogInfor*, 			// 处理1000请求
					void *data, const unsigned int, CBuffWriter*);
	bool AddWin8EWarningMsg(const PushMsg*, WpReqTag*);					// 以Win8的记录格式添加预警记录
	bool AddWin8PublicNoticeMsg(const PushMsg*, WpReqTag*);				// 以Win8的记录格式生成公共消息记录
	bool AddWin8MineMsg(const PushMsg*, WpReqTag*);						// 以Win8的记录格式生成信息地雷记录
	const int SendEWarningMsg(const unsigned int);						// 发送预警消息
	const int SendInfoMineMsg(const unsigned int);						// 发送信息地雷
	const int SendNoticeMsg(const unsigned int);						// 发送公告

	const int ProcessSelfStkInfoMineStatis(const unsigned int);			// 执行自选股信息地雷统计

public:
	int SetUserPushToken(const char*);									// 设置推送令牌
	void GetUserPushToken(char *token, const int MaxLen);				// 获取推送令牌
	const int CheckUserAliveStatus(const time_t);						// 检查用户节点的运行状态(CalOperatingThread定期调用)
	int ProcessUserRequest(const sub_head*, const UserLogInfor*,		// 处理用户请求(sub_head后面除去用户信息的数据)
						void*, const unsigned int, CBuffWriter*);
	int ExecuteStkPriceWarningCal(const unsigned int);					// 执行节点股价预警计算
	int ExecutePublicNoticeScan(const unsigned int);					// 执行公共新闻公告相关扫描
	const int ExecuteSelfStkInfoMine(const unsigned int);				// 执行自选股信息地雷扫描

	int SendOuterMsg(const unsigned int, const unsigned char);			// 发送消息
	int ExecuteSend(const unsigned int, const unsigned char);			// 执行发送
};

#endif	/* _INCLUDE_WIN8_PUSH_USER_H */
