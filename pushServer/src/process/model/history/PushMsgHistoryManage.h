#ifndef _INCLUDE_PUSHMSG_HISTORY_MANAGE_H
#define _INCLUDE_PUSHMSG_HISTORY_MANAGE_H

#include "PushMsgHistory.h"

class CPushMsgHistoryManage
{
private:
	CPushMsgHistoryManage();
	~CPushMsgHistoryManage();
	
private:
	// Android平台用户历史记录信息
	static CPushMsgHistory *m_pAndroidMsgHis;
	// Ios平台用户历史记录信息
	static CPushMsgHistory *m_pIosMsgHis;
	// Wp7平台用户历史记录信息
	static CPushMsgHistory *m_pWp7MsgHis;
	// Win8平台用户历史记录信息
	static CPushMsgHistory *m_pWin8MsgHis;
	
private:
	// 初始化资源
	static const int CreateRes();
	// 获取指定平台的历史记录管理
	static CPushMsgHistory *GetPlatFormMsgHis(const int);
	
public:
	// 初始化
	static bool InitialMsgHis();
	// 释放资源
	static void ReleaseMsgHis();
	// 添加历史记录
	static const int PushBackMsgHistory(const int, const size_t, const size_t,
			const size_t, const PushMsg&);
	// 获取指定索引指定指定类型的消息
	static const int GetPushMsg(const int, const size_t,
				const int, const size_t, pPushMsg&);
	// 清空节点的历史记录信息
	static bool ClearNodeMsgHistory(const int, const size_t);
	// 获取最大历史记录数
	static const short GetMaxMsgCacheCount(const int, const int);
	// 获取信息Crc
	static const unsigned int GetUserHisMsgCrc(const int, const size_t,
				const int, const size_t);
};

#endif		/* _INCLUDE_PUSHMSG_HISTORY_MANAGE_H */

