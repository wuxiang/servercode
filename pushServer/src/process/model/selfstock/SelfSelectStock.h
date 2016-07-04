#ifndef _INCLUDE_SELF_SELECT_STOCK_H
#define _INCLUDE_SELF_SELECT_STOCK_H

#include "../../../util/util.h"
#include "../../../util/common/common_types.h"

class CStkCodeOperate;
struct SelfSelectRecHead;
struct SelfSelectRecBody;

class CSelfSelectStock
{
public:
	CSelfSelectStock(const size_t, const BYTE, const char*);
	~CSelfSelectStock();

private:
	// 设置的最大用户数
	const size_t m_uMaxUser;
	// 每个用户设置记录的最大数
	const BYTE m_uMaxRecPerUser;
	// 每个记录的大小
	const size_t m_iRecordLen;
	// 记录头的大小
	const size_t m_iHeadLen;
	// 记录文件名称
	char m_strFileName[MAX_PATH_LEN + 1];
	// 映射文件地址
	void *m_pMemAddress;
	// 映射文件大小
	off_t m_iMapFileSize;
	// 当前的最新状态(0表示正常状态)
	int m_iLatestStatus;
	// 每个用户的分段偏移量
	size_t m_uPerSegOffset;
	// 代码管理
	CStkCodeOperate *m_pStkCodeOperate;
	
private:
	// 创建内存映射
	int CreateMemmap();
	// 获取指定索引的头
	SelfSelectRecHead* GetIndexHead(const size_t);
	// 获取记录本体
	void* GetIndexBody(const size_t);
	
public:
	// 初始化数据读入
	int InitialLoad();
	// 释放分配的资源
	void ReleaseRes();
	// 添加代码
	const int AddSelfCode(const size_t, const char*);
	// 删除代码
	const int RemoveSelfCode(const size_t, const char*);
	// 清空代码
	const int ClearAllSelfCode(const size_t);
	// 读取指定索引的代码
	const char* ReadSelfIndexCode(const size_t, const size_t, char *, const unsigned int);
	// 读取指定节点的记录数量
	const int GetSelfCodeCount(const size_t);
	// 设置指定节点的记录数量
	void SetSelfCodeCount(const size_t, const unsigned char);
	// 是否设置了接收信息地雷标记
	bool IsRecvMarkSet(const size_t);
	// 设置接收标识
	void SetRecvMark(const size_t, bool);
	// 设置变更标识
	void SetModifiedMark(const size_t, bool);
	// 记录是否存在变更
	bool IsModified(const size_t);
	// 设置最近一次发送统计信息的次数
	void SetStaticsCount(const size_t, const unsigned short);
	// 获取最近一次发送统计信息的次数
	const int GetStaticsCount(const size_t);
	// 设置已经发送统计信息地雷的次数(不会大于发送统计的最大区间段)
	void SetHaveSendTimes(const size_t, const unsigned short);
	// 获取已经发送统计信息地雷的次数
	const int GetHaveSendTimes(const size_t);
	// 设置是否收到已读取回应
	void SetHaveReadMark(const size_t, bool);	
	// 是否收到已读取回应
	bool HaveReadMark(const size_t);
	// 重置
	void Reset(const size_t);
	// 获取全部代码串
	const char* GetTotalSelfStkCode(const size_t);
	// 设置代码串
	void SetTotalSelfStkCode(const size_t, const char*);
	// 设置最近发送的信息地雷ID
	void SetLatestInfoMineCrc(const size_t, const unsigned int);	
	// 获取最近发送的信息地雷ID
	const unsigned int GetLatestInfoMineCrc(const size_t);
	// 获取证券代码在整体中的索引
	const int GetStkCodeIndex(const size_t, const char*);
	// 设置信息地雷总数
	void SetInfomineTotalCount(const size_t, const short);
	// 获取信息地雷总数
	const unsigned char GetInfomineTotalCount(const size_t);
};


#endif		/* _INCLUDE_SELF_SELECT_STOCK_H */

