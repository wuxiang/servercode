#ifndef PUBLICNEWS_H
#define PUBLICNEWS_H

#include "PNmsg.h"
#include "../../data_type.h"
#include <string>
#include <map>
#include <vector>

#ifndef NOTICE_LINK_LEN
#define NOTICE_LINK_LEN 32
#endif

class CppODBC;
class CPushUser;

class CPublicNews
{
private:
	CPublicNews(const unsigned int, const char*, const char*, const char*);
	~CPublicNews();
	
private:
	// 缓存最大数量
	const unsigned int m_uMaxCacheNum;
	// 用于调用odbc函数
	CppODBC * m_pCppODBC;
	// 数据缓存
	PNmessage *m_pRecordCache;
	// 节点关系映射(key消息数据库标识ID value头节点位置索引)
	std::map<const unsigned long, unsigned int> m_mpMarkNodes;
	// 当前占用的节点数目索引
	unsigned int m_uOccupyIndex;
	// 当前的数据库最大记录标识ID
	unsigned long m_luCurrentMaxRecID;
	// 当前的状态
	int m_iCurrentStatus;
	// ODBC数据源
	char m_dsn[NOTICE_LINK_LEN];
	// 用户名
	char m_uname[NOTICE_LINK_LEN];
	// 密码
	char m_upass[NOTICE_LINK_LEN];
	// 类指针
	static CPublicNews * m_pPublicNews;
	// 新增的消息表示ID
	std::vector<unsigned long> m_GotNewMsgID;
	
private:
	const unsigned int GetCirculateIndex(const unsigned int);
	// 添加记录
	const int AddNewRecord(const PNmessage &);
	// 获取指定索引的记录指针
	PNmessage* GetIndexPNmessage(const unsigned int);
	// 移除指定索引的记录
	const int RemoveIndexNode(const unsigned int);
	// 更新节点链接
	bool UpdateLinkNode(const unsigned long, const unsigned int Index);
	// 初始化资源
	void InitialRes();
	// 重置请求
	void resetData();
	// 建立数据库连接
	bool DBconnect();
	// 断开数据库连接								
	bool DBdisconnect();
	// 请求数据
	const int RequestMsg(const char *, bool);
	// 读取最近10天的公告
	const int ReadLatest10DaysMsg();
	// 解析平台
	const int ParsePlatformCode(char*, PNmessage*);
	// 匹配公告消息发送属性
	const int MatchNoticeSend(const CPushUser*, const PNmessage*);
	// 设置最新状态
	void SetCurrentStatus(const int);
	// 置位数据库标识为已读取
	const int SendCallBackToDb();
	// 按照UTF8截取字符串长度
	char* LeftUtf8String(const int, const char *,
			char *, const int);
	// 解析版本号
	const int ParseVersionRange(char*, PNmessage*);
	// 读取已经发送的最大数据ID
	const unsigned long GetMaxDbIDMarkByOne();

public:
	static CPublicNews * getPublicNewsPointer();
	static void releasePublicNewsPointer();
	// 初始化请求
	bool InitialData();
	// 增量请求
	const int GetDynaData();
	// 请求最新的消息
	const unsigned long GetLatestMsg(const unsigned long, const CPushUser*, PNmessage&);
	// 获取指定的消息
	bool GetIndexPNmessage(const unsigned long, PNmessage&);
	// 获取当前的状态值
	const int GetCurrentStatus()const;
};

#endif //PUBLICNEWS_H
