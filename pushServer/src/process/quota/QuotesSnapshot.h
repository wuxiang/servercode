#ifndef _INCLUDE_QUOTESSNAPSHOT_H
#define _INCLUDE_QUOTESSNAPSHOT_H

#include "../../util/util.h"
#include "../../util/log/log.h"
#include "../../util/common/common_types.h"
#include "../data_type.h"
#include <map>
#include <string>

struct ACC_STK_STATIC;
struct ACC_STK_DYNA;
struct MarketClassified;
struct MarketValueMap;


#pragma pack(1)
// 结果数据
typedef struct PriceAlarm {
    char    m_strSymbol[MAX_STKCODE_LEN + 1];    				//股票代码
    char    m_strName[MAX_STKNAME + 1];      					//股票名称
    unsigned int m_Price;           	 						//最新价
    int m_Change;           									//涨跌幅
    unsigned int m_TurnoverRate;     							//换手率
    time_t  m_time;             								//成交时间
    unsigned char	m_cPrecision;								//小数位精度
    
public:
	// 最新价不为零 并且成交时间大于触点时间定义为有效
	bool IsValid(const time_t trigertime)const
	{
		if (0 == m_time || m_time <= trigertime)
		{
			#ifndef _IGNORE_EXCHANGE_TIME
			return false;
			#endif
		}
		if (m_Price <= 0)
		{
			return false;
		}
		return true;
	}
	
	PriceAlarm& operator = (const PriceAlarm &item)
	{
		strncpy(m_strSymbol, item.m_strSymbol, MAX_STKCODE_LEN);
		strncpy(m_strName, item.m_strName, MAX_STKNAME);
		m_Price = item.m_Price;
		m_Change = item.m_Change;
		m_TurnoverRate = item.m_TurnoverRate;
		m_time = item.m_time;
		m_cPrecision = item.m_cPrecision;
		return *this;
	}
};

// 运行状态
enum QuotesSnapshotStatusEnum
{
	QS_INITIALIZING = 0,										// 正在进行初始化
	QS_NORAML,													// 正常运行
	QS_ERROR,													// 异常状态
	QS_EXCHANGE_SER,											// 切换数据服务器
	QS_NO_DEFINE												// 未定义状态
};

#pragma pack()

class CQuotesSnapshot
{
private:
    CQuotesSnapshot();
    ~CQuotesSnapshot();

private:
    static int m_sockfd;												// socket 标识
    static enum QuotesSnapshotStatusEnum m_nStatus;						// 运行状态
    static enum NetStatus m_nNetStatus;									// 网络运行状态
    static bool m_bInitialed;											// 是否已经成功初始化
    static struct ACC_STK_STATIC *m_pStkStatic;							// 静态数据区
    static struct ACC_STK_DYNA *m_pStkDyna;								// 动态数据区
    static struct PriceAlarm* m_pPriceAlarmFirst;						// 结果数据区1
    static struct PriceAlarm* m_pPriceAlarmSecond;						// 结果数据区2
    static struct PriceAlarm *m_pCurrentSearchDyna;						// 当前用于查询的动态数据区指针
    static struct PriceAlarm *m_pCurrentCalcDyna;						// 当前用于计算的动态数据区指针
    static std::map<std::string, MarketClassified> m_mpMarketClassf;	// 市场分类列表
    static struct MarketValueMap *m_pMarketValueMap;					// 市场值映射
    
private:
	static const int GetMaxRecordNum();									// 获取最大总记录数
	static int SocketConnect(bool);										// 连接服务器
	static void CloseConnect();											// 关闭当前的连接
	static int RequestStaicData();										// 请求静态数据
	static int RequestMarketStaticData(MarketClassified *, const int);	// 请求市场分类的静态数据
	static int RequestDynaData();										// 请求动态数据
	static int RequestMarketDynaData(MarketClassified *, const int);	// 请求市场分类的动态数据
	static int CalcRequireData();										// 计算需要的参数
	static void ExchageNameToUtf8();									// 将证券名称转换为UTF8
	static bool HasMarketStaticDataUpdate(MarketClassified*);			// 判断指定市场的静态数据是否有更新
	static void AddMarkMapValue(const int,								// 添加市场值映射
			const struct MarketValueMap*);
	static void SortMarketIndexMapValue(const int,						// 排序市场值映射
			const int);
	static int FindMarketIndexMapValue(const int,						// 查找市场值映射
			const int, const MarketValueMap*);
	static int ExchangeMarketCode(const char*);							// 转换市场代码
	static bool ParseSupportMarket();									// 解析支持的市场
	static MarketClassified* FindMarketClassified(const char*);			// 查找市场分类
	static char* ParseMarketCode(const int);							// 解析市场代码
	
public:
	static void InitialRes();											// 初始化资源
	static void ReleaseRes();											// 释放资源
	static bool InitialData();											// 数据初始化
	static bool UpdateDynaData();										// 更新动态数据
	static const unsigned short findSecurityIndex(const char*);			// 查找证券在相应市场中的索引
	static PriceAlarm* GetIndexSecurity(const int, const char*);		// 获取指定的动态信息
	static bool HasInitialed();											// 是否已经初始化
	static bool HasStaticDateUpdate();									// 判断静态数据是否有更新
	static ACC_STK_STATIC* GetIndexStatic(const int, const char*);		// 获取指定的静态信息
};

#endif // _INCLUDE_QUOTESSNAPSHOT_H
