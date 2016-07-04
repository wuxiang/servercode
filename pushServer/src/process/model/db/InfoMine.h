#ifndef INFOMINE_H
#define INFOMINE_H

#include "../template/ContainerCmp.h"
#include "../../data_type.h"
#include <string>
#include <map>

#ifndef INFOMINE_MAXTITLELEN
#define INFOMINE_MAXTITLELEN 190
#endif

#ifndef INFOMINE_LINK_LEN
#define INFOMINE_LINK_LEN 32
#endif

struct IMmessage
{
	// 数据库的ID
	unsigned long MsgID;
	// 证券代码
	char strStockCode[MAX_STKCODE_LEN + 1];
	// 标题(不超过190字节)
	char strTitle[INFOMINE_MAXTITLELEN + 1];
	// 新闻时间
	time_t ttNewsTime;
	// 前一个同类索引
	unsigned int Previous;
	// 下一个同类索引
	unsigned int Next;

	IMmessage()
	{
		MsgID = (unsigned long)-1;
		memset(strStockCode, 0, MAX_STKCODE_LEN);
		memset(strTitle, 0, INFOMINE_MAXTITLELEN);
		ttNewsTime = 0;
		Previous = (unsigned int)-1;
		Next = (unsigned int)-1;
	}

	IMmessage& operator = (const IMmessage &item)
	{
		MsgID = item.MsgID;
		strncpy(strStockCode, item.strStockCode, MAX_STKCODE_LEN);
		strncpy(strTitle, item.strTitle, INFOMINE_MAXTITLELEN);
		ttNewsTime = item.ttNewsTime;
		Previous = (unsigned int)-1;
		Next = (unsigned int)-1;
		return *this;
	}
};

class CppODBC;
struct InfoMineStatisTag;

class CInfoMine
{
private:
	CInfoMine(const unsigned int, const char*, const char*, const char*);
	~CInfoMine();
	
private:
	// 缓存最大数量
	const unsigned int m_uMaxCacheNum;
	// 用于调用odbc函数
	CppODBC * m_pCppODBC;
	// 数据缓存
	IMmessage *m_pRecordCache;
	// 节点关系映射(key证券代码 value头节点位置索引)
	std::map<const char*, unsigned int, NodeMarkCmpFun> m_mpMarkNodes;
	// 信息地雷个数的统计值(key证券代码 value该证券对应的信息地雷数目)
	std::map<std::string, InfoMineStatisTag> m_mpInfoMineStatics;
	// 需要实时发送的信息地雷列表
	std::map<unsigned long, unsigned int> m_mpInfoMineRT;
	// 当前占用的节点数目索引
	unsigned int m_uOccupyIndex;
	// 当前的数据库最大记录标识ID
	unsigned long m_luCurrentMaxRecID;
	// 是否开始进行实时推送消息统计
	bool m_bStartRTStatis;
	// 当前的状态
	int m_iCurrentStatus;
	// ODBC数据源
	char m_dsn[INFOMINE_LINK_LEN];
	// 用户名
	char m_uname[INFOMINE_LINK_LEN];
	// 密码
	char m_upass[INFOMINE_LINK_LEN];
	// 类指针
	static CInfoMine * m_pInfoMine;
	
private:
	const unsigned int GetCirculateIndex(const unsigned int);
	// 添加记录
	const int AddNewRecord(const IMmessage &, unsigned int &);
	// 获取指定证券的头位置索引
	const unsigned int GetStkInfoMineHeadIndex(const char*);
	// 获取指定证券的尾位置索引
	const unsigned int GetStkInfoMineTailIndex(const unsigned int);
	// 查找指定MsgID的信息位置索引
	const unsigned int GetIMmessageIndexByMsgID(const unsigned int, const unsigned long);
	// 查找比指定ID大的信息位置索引
	const unsigned int GetIMmessageIndexLargerThanMsgID(const unsigned int, const unsigned long);
	// 获取指定索引的记录指针
	IMmessage* GetIndexIMmessage(const unsigned int);
	// 移除指定索引的记录
	const int RemoveIndexNode(const unsigned int);
	// 更新节点链接
	bool UpdateLinkNode(const char *stkCode, const unsigned int Index);
	// 初始化资源
	void InitialRes();
	// 重置请求
	void resetData();
	// 建立数据库连接
	bool DBconnect();
	// 断开数据库连接								
	bool DBdisconnect();
	// 读取当天的信息地雷
	int CompanyNews_Query_CurDate();
	// 初始读取有效的信息地雷
	int ReadValidInfoMine();
	// 添加实时发送的信息地雷索引
	bool AddRTInfoMine(const unsigned int, const IMmessage &);
	// 设置最新状态
	void SetCurrentStatus(const int);
	
public:
	static CInfoMine * getInfoMinePointer();
	static void releaseInfoMinePointer();
	// 初始化请求
	bool InitialData();
	// 增量请求
	const int GetDynaData();
	// 请求最新的消息
	const unsigned long GetLatestMsg(const char*, const unsigned long, IMmessage&);
	// 获取指定的消息
	bool GetIndexIMmessage(const char*, const unsigned long, IMmessage&);
	// 执行信息地雷的个数统计
	const int ExecuteInfoMineStatis(const int);
	// 获取证券的信息地雷个数
	const unsigned int GetStockInfoMineCount(const char*);
	// 信息地雷统计是否执行成功
	bool IsInfoMineStatisSuccess();
	// 设置是否开始实时发送信息地雷
	void SetStartRTInfoMineFlag(bool);
	// 请求最新的消息
	const int GetLatestMsg(const unsigned long, IMmessage&);
	// 获取当前的状态值
	const int GetCurrentStatus()const;
};

#endif //INFOMINE_H
