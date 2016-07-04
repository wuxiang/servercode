#ifndef _INCLUDE_ANDROID_USER_MANAGE_H
#define _INCLUDE_ANDROID_USER_MANAGE_H

#include "../template/StaticNodeManage.h"
#include "../../../util/common/common_types.h"

class CAndroidPushUser;
struct  CUserBaseInfoHead;

class CAndroidPushUserManage : public CStaticNodeManage<CAndroidPushUser>
{
public:
	CAndroidPushUserManage();
	~CAndroidPushUserManage();

private:
	// 设置最大用户数
	size_t m_uMaxEWarnUser;
	// 每个记录的大小
	size_t m_iRecordLen;
	// 记录文件名称
	char m_strFileName[MAX_PATH_LEN];
	// 映射文件地址
	void *m_pMemAddress;
	// 映射文件大小
	off_t m_iMapFileSize;
	// 当前的最新状态(0表示正常状态)
	int m_iLatestStatus;
	// 头部标识的偏移量
	size_t m_uHeadOffset;
	
protected:
	// 更新设置新节点
	const int UpdateSetNode(const int HeadIndex, const CAndroidPushUser &Node);
	// 节点(组)是否有效
	bool IsKeepValid(const int HeadIndex);
	// 执行整理无效节点(组)
	void ProcessArrangeInvalidNode();

private:
	// 执行初始化配置
	const int InitialEnv();
	// 创建内存映射
	int CreateMemmap(bool &);
	// 初始化标识
	void InitialUserBaseInfo();
	// 更新头标识(只能更新大小，文件结构及占位不能更新)
	void UpdateUserBaseInfo();
	// 初始化读取用户
	void InitialLoadUser();

public:
	// 初始化数据读入
	int InitialLoad();
	// 释放分配的资源
	void ReleaseRes();
	// 获取用户基本信息头
	CUserBaseInfoHead* GetUserBaseInfoHead()const;
	// 获取用户
	CAndroidPushUser* GetUser(const unsigned int);
	// 更新占用的用户数属性
	const short UpdateOccupyProperty(const unsigned int);
	// 更新已经占用的预警资源属性
	const short UpdateUseWarnProperty(const unsigned int);
	// 从数据库加载用户
	const int TryReadUserFromDb(const char *, const unsigned char, CAndroidPushUser*);
	// 同步用户基本信息头
	const int SyncUserBaseInfoHeadToDb();
	// 同步用户信息
	const int SyncUserToDb();
	// 返回无效的预警节点段数
	const unsigned int GetEWarnNodeInvalidCount();
};

#endif	/* _INCLUDE_ANDROID_USER_MANAGE_H */
