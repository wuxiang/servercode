#ifndef _INCLUDE_PLATFORM_EWARNING_MANAGE_H
#define _INCLUDE_PLATFORM_EWARNING_MANAGE_H

#include "../template/StaticNodeManage.h"
#include "../../../util/common/common_types.h"

class CEarlyWarningConditionRecord;

class CPlatformEWarnManage : public CStaticNodeManage<CEarlyWarningConditionRecord>
{
public:
	CPlatformEWarnManage(const size_t, const size_t, const BYTE, const BYTE, 
			const size_t, const char*);
	~CPlatformEWarnManage();

private:
	// 设置预警的最大用户数
	const size_t m_uMaxEWarnUser;
	// 当前占用的节点数
	const size_t m_uOccupyCount;
	// 每个用户设置预警记录的最大数
	const BYTE m_uMaxEWarnPerUser;
	// 每个用户设置预警的默认个
	const BYTE m_uDefaultEWarnPerUser;
	// 每个记录的大小
	const size_t m_iRecordLen;
	// 预警记录文件名称
	char m_strFileName[MAX_PATH_LEN];
	// 映射文件地址
	void *m_pMemAddress;
	// 映射文件大小
	off_t m_iMapFileSize;
	// 当前的最新状态(0表示正常状态)
	int m_iLatestStatus;
	// 每个用户历史的分段偏移量
	size_t m_uPerSegOffset;
	
protected:
	// 更新设置新节点
	const int UpdateSetNode(const int HeadIndex, const CEarlyWarningConditionRecord &Node);
	// 节点(组)是否有效
	bool IsKeepValid(const int HeadIndex);
	// 执行整理无效节点(组)
	void ProcessArrangeInvalidNode();

private:
	// 创建内存映射
	int CreateMemmap();

public:
	// 获取每个用户的最大预警数
	const BYTE GetMaxEWarnPerUser()const;
	// 获取每个用户的默认的预警数
	const BYTE GetDefaultEWarnPerUser()const;
	// 初始化数据读入
	int InitialLoad();
	// 释放分配的资源
	void ReleaseRes();
	// 获取当前设置的有效预警的数目
	const short GetEarlyWarningNum(const size_t);
	// 获取指定的预警记录设置
	CEarlyWarningConditionRecord* GetConditionRecord(const size_t, const unsigned int);
	// 获取指定代码的预警记录
	CEarlyWarningConditionRecord* GetStkConditionRecord(const size_t, const size_t, const char*);
	// 清空一组记录
	void ClearGroupRecord(const size_t);
};

#endif	/* _INCLUDE_PLATFORM_EWARNING_MANAGE_H */
