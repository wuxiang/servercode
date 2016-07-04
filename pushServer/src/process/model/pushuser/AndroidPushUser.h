#ifndef _INCLUDE_ANDROID_PUSH_USER_H
#define _INCLUDE_ANDROID_PUSH_USER_H

#include "PushUser.h"
typedef struct ReqLatestMsgTag;
typedef struct ResLatestMsgTag;

class CAndroidPushUser : public CPushUser
{
public:
	CAndroidPushUser(void);
	CAndroidPushUser(const UserLogInfor*, const unsigned int);
	~CAndroidPushUser(void);

protected:
	int ParseReq314(const sub_head *, const UserLogInfor*, 			// 处理314请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq317(const sub_head *, const UserLogInfor*, 			// 处理317请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq1000(const sub_head *, const UserLogInfor*, 		// 处理1000请求
					void *data, const unsigned int, CBuffWriter*);

private:
	bool AddAndroidEWarningMsg(const unsigned int,					// 以android的记录格式生成预警记录
				const unsigned int, const PushMsg*, CBuffWriter*);
	bool AddAndroidMineMsg(const unsigned int,						// 以android的记录格式生成信息地雷记录
				const unsigned int, const PushMsg*, CBuffWriter*);
	bool AddAndroidPublicNoticeMsg(const unsigned int,				// 以android的记录格式生成公共消息记录
				const unsigned int, const PushMsg*, CBuffWriter*);
	void GenerateSendRecord(LargeMemoryCache*, CBuffWriter*);		// 形成发送数据
	int GetLastestPublicNotice(const sub_head *,	 				// 请求最新的公告(非预警的android用户)
				const unsigned int, CBuffWriter *);
	int PushbackNoticeMsg(const PushMsg*, const unsigned int,		// 存储公告消息
				CBuffWriter*);

	LargeMemoryCache* GetThreadResponeRes();						// 获取线程回复资源
	const int SendEWarningMsg(const unsigned int);					// 发送预警消息
	const int SendInfoMineMsg(const unsigned int);					// 发送信息地雷
	const int SendNoticeMsg(const unsigned int);					// 发送公告

	const int ProcessSelfStkInfoMineStatis(const unsigned int);		// 执行自选股信息地雷统计
	
	int RequestLatestNoticeForUnWarningUser(const ReqLatestMsgTag&,	// 非预警用户请求最新的公告消息
				const sub_head*, CBuffWriter *, unsigned int&);
	int RequestLatestInfoMineForUnWarningUser(const ReqLatestMsgTag&,// 非预警用户请求最新的信息地雷
				const sub_head*, CBuffWriter *, unsigned int&);

public:
	const int CheckUserAliveStatus(const time_t);					// 检查用户节点的运行状态(CalOperatingThread定期调用)
	int ProcessUserRequest(const sub_head*, const UserLogInfor*,	// 处理用户请求(sub_head后面除去用户信息的数据)
						void*, const unsigned int, CBuffWriter*);
	int ExecuteStkPriceWarningCal(const unsigned int);				// 执行节点股价预警计算
	int ExecuteStkInfoMineScan(const unsigned int);					// 执行预警记录信息地雷扫描
	int ExecutePublicNoticeScan(const unsigned int);				// 执行公共新闻公告相关扫描
	int ExecuteSend(const unsigned int, const unsigned char);		// 执行发送
	int SendOuterMsg(const unsigned int, const unsigned char);		// 发送消息
	const int ExecuteSelfStkInfoMine(const unsigned int);			// 执行自选股信息地雷扫描

};

#endif	/* _INCLUDE_ANDROID_PUSH_USER_H */
