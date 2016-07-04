#ifndef _INCLUDE_PUSHMSG_HISTORY_H
#define _INCLUDE_PUSHMSG_HISTORY_H

#include "../../../util/util.h"
#include "../../data_type.h"
#include "RespMsg.h"

class CPushMsgHistory
{
public:
	CPushMsgHistory(const size_t, const size_t, const size_t,
		const size_t, const size_t, const char*);
	~CPushMsgHistory();
	
private:
	const size_t m_iRecordLen;								// 每个记录的大小
	const size_t m_iMaxRemainEWarningCount;					// 每个用户保留的最大预警记录数
	const size_t m_iMaxRemainInfomineCount;					// 每个用户保留的最大信息地雷数
	const size_t m_iMaxRemainNoticeCount;					// 每个用户保留的最大公告消息数
	const size_t m_iMaxSegNum;								// 映射区域最大分段数(每一个用户都有固定的地方存储历史记录)
	char m_strFileName[MAX_PATH_LEN];						// 记录文件名
	void *m_pMemAddress;									// 映射文件地址
	off_t m_iMapFileSize;									// 映射文件大小
	int m_iLatestStatus;									// 当前的最新状态(0表示正常状态)
	size_t m_uPerSegOffset;									// 每个用户历史的分段偏移量
	
private:
	int CreateMemmap();										// 创建内存映射	
	PushMsg* GetPushMsgHead(const size_t, const int,		// 获取消息记录的首地址
				size_t&);
	size_t GetCirculateIndex(const size_t, const size_t);	// 获取循环缓冲区的索引
	time_t GetHisMsgValidTime(const int);					// 获取消息的有效期
	
public:
	int InitialLoadMsgHistory();							// 初始化历史记录数据
	void ReleaseRes();										// 释放分配的资源
	const int AddMsgHistory(const size_t, const size_t, 	// 添加历史记录
				const size_t, const PushMsg&);
	const int GetPushMsg(const size_t,						// 获取指定索引指定指定类型的消息
				const int, const size_t, pPushMsg&);
	bool ClearNodeMsgHistory(const size_t);					// 清空节点的历史记录信息
	const size_t GetMaxMsgCacheCount(const int);			// 获取最大历史记录数
	bool IsMsgOutofDate(pPushMsg, const time_t);			// 消息是否已经过期
};


#endif		/* _INCLUDE_PUSHMSG_HISTORY_H */

