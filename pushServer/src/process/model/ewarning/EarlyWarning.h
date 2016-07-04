#ifndef _INCLUDE_EARLY_WARNING_H
#define _INCLUDE_EARLY_WARNING_H

#include "../../config.h"
#include "../../data_type.h"
#include "../../../util/util.h"
#include "../../../util/common/common_types.h"
#include <string>

struct PriceAlarm;

enum EarlyWarningConditionProperty
{
	// 0 - 1位 标识预警条件的有效期
	EWCP_PERIOD_VALIDITY_ONCE =						0x0000,				// 一次有效(只要满足一次，其后置为无效)
	EWCP_PERIOD_VALIDITY_ALWAYS =					0x0001,				// 永久有效(每天满足一次，长期有效)
	EWCP_PERIOD_VALIDITY_ONEDAY =					0x0002,				// 一天有效(一天内，最多满足一次，过后无论是否满足都将无效)

	// 2位 标识是否已经满足一次条件
	EWCP_NO_MATCH_CONDITION =						0x0000,				// 尚未满足过条件
	EWCP_HAVE_MATCH_CONDITION =						0x0004,				// 已经满足条件一次

	// 3 - 4位 标识当前的预警条件的有效性
	EWCP_UNDER_MONITORING =							0x0000,				// 正在监测中
	EWCP_UNDER_WAITING =							0x0008,				// 有效期内已经达成一次，等待下次触发
	EWCP_OUTOFDATE =								0x0010,				// 已经过期失效
	EWCP_INVALID =									0x0018,				// 非法无效设置

	// 5位 标识状态是否需要重置
	EWCP_KEEP_STATE =								0x0000,				// 保持当前的状态
	EWCP_RESET_STATE =								0x0020				// 需要重置状态，以启动下次监测
};

// 符号比较类型
enum CompareTypeEnum
{
	GREATER_THAN = 0,													// >
	SMALLER_THAN,														// <
	EQUAL_TO,															// ==
	GREATER_AND_EQUAL,													// >=
	SMALLER_AND_EQUAL													// <=
};

// 预警类型
enum WarningTypeEnum
{
	PriceGreater = 0,													// 股价高于
	PriceSmaller,														// 股价低于
	IncreaseGreater,													// 涨幅大于
	DecreaseGreater,													// 跌幅大于
	ExchangeGreater,													// 换手率大于
};

#pragma pack(1)
class CEarlyWarningCondition
{
public:
	CEarlyWarningCondition(const unsigned int value, unsigned short Property, unsigned char cmp);
	CEarlyWarningCondition();
	~CEarlyWarningCondition();
	bool Construct(const unsigned int value, unsigned short Property, unsigned char cmp);
	friend class DatabaseSnapshot;

private:
	unsigned int m_uTriggerValue;										// 触发点值
	unsigned short m_sProperty;											// 属性字段
	unsigned char m_eCompareType;										// 比较类型

private:
	void SetProperty(unsigned short Property);							// 设置属性，需要取其它属性字段一起设置

public:
	CEarlyWarningCondition& operator = (const CEarlyWarningCondition &item);

	const unsigned short GetPeriodValidityProperty()const;				// 取有效期属性
	const unsigned short GetMatchConditionyProperty()const;				// 取有匹配属性
	const unsigned short GetEarlyWarningStatusProperty()const;			// 取有效性属性
	const unsigned short GetRequireResetProperty()const;				// 取有是否重置属性
	const unsigned short GetProperty()const;							// 获取属性值
	const unsigned char GetRawGetPeriodProperty()const;					// 获取有效期的原始表示

	const unsigned int GetTriggerValue()const;							// 取触发点值
	const unsigned int GetConditionValue()const;						// 获取在当前条件下的触发值(无效为零 否则为GetTriggerValue)
	bool ExecuteCompare(const unsigned int value);						// 执行比较，并置相应的属性位
	bool CanExecuteCmp()const;											// 是否可以执行触发比较
	void Reset();														// 重置节点
	const unsigned char IsRemainValid()const;							// 是否依然有效
	bool EqualZero();													// 是否为零
};

//m_cRecordProperty字段取值
enum
{
	// 6 - 7位 信息同步到库的操作方式
	WARNINGSET_INSERTDB =	0x80,	// 表示该信息要insert到DB中
	WARNINGSET_UPDATEDB =	0x40,	// 表示该信息要update到DB中
	WARNINGSET_DELDB	=	0xc0,	// 表示该信息从DB中delete(!!!除了表示操作方式外，同时也表示该记录是否有效，因此，同步完毕后不能置空)
	WARNINGSET_NULL = 0x00,			// 置空

	// 5位 标识是否设置了接收该股票信息地雷
	WARNINGSET_NEW =	0x20,

	// 4位 表示该组信息是否分配使用(接下来一组是否已经被占用, 只设置于首节点位)
	WARNINGSET_USE =	0x10,

	// 0 - 3位 标识已经设置的预警记录个数(只设置于首节点位)
	WARNINGSET_NUM =	0x0F,
};

class CEarlyWarningConditionRecord
{
public:
	CEarlyWarningConditionRecord();
	int Construct(const char*, const unsigned char);
	~CEarlyWarningConditionRecord();
	friend class DatabaseSnapshot;

private:
	char m_strStkCode[MAX_STKCODE_LEN];										// 股票代码
	unsigned char m_cRecordProperty;										// 记录的属性字段
	// 序列为：股价大于 股价低于 涨幅大于 涨幅低于 日换手率大于
	CEarlyWarningCondition m_EarlyWarningCondition[EARLY_WARNING_TYPE_NUM];	// 价格预警记录成员
	unsigned short m_nStkIndex;												// 当前股票代码索引, 如果是0xFFFF表示该股票没有找到匹配
	TIME_T32 m_nBeginCmpTime;												// 有效比较起点时间
	unsigned int m_uLatestInfoMineCrc;										// 信息地雷最近Crc

private:
	// 执行比较
	bool ExcuteTypeCmp(const struct PriceAlarm *, const unsigned char);
	// 设置信息地雷属性
	void UpdateSetInfoMineProperty(const unsigned char SetFlag);
	// 更新比较起点值
	void UpdateBeginCmpTime();
	// 获取比较起点值
	const TIME_T32 GetBeginCmpTime()const;

public:
	CEarlyWarningConditionRecord& operator = (const CEarlyWarningConditionRecord &item);
	// 设置预警记录
	void SetCondition(const CEarlyWarningCondition&, const unsigned char);
	// 获取预警记录
	CEarlyWarningCondition* GetCondition(const unsigned char);
	// 执行全部条件匹配(按照序列返回是否满足条件，1标识满足 0标识不匹配)
	int ExcuteCmp(struct PriceAlarm *);
	// 获取每条预警记录的成员数目
	static const int GetRecordMemCount();
	// 获取相应位对应的预警类型
	enum WarningTypeEnum GetWarningTypeByBit(const int);
	// 返回相应为对应的预警记录的有效状态
	unsigned char GetWarningValidationByBit(const int);
	// 重置节点状态
	int ResetRecordState();
	// 获取证券代码
	const char* GetStkCode()const;
	// 获取证券名称
	const char* GetStkName()const;
	// 更新有效的记录
	void UpdateValidRecord(const CEarlyWarningConditionRecord*);
	// 获取浮点型小数位精度
	const unsigned char GetFloatPrecision();
	// 重置节点对应最新动态数据指针
	void ResetStkLatestInfo();

	// 获取信息地雷设置属性
	const unsigned char GetInforMineProper()const;
	// 获取信息地雷设置的原始属性
	const unsigned char GetInforMineRawProper()const;
	// 设置预警记录同步到库的操作方式
	void UpdateSetOperToDb(const unsigned char);
	// 获取预警记录同步到库的操作方式
	const unsigned char GetOperToDb()const;
	// 设置预警记录组占用标志
	void UpdateSetEGroupUseFlag(const unsigned char);
	// 获取预警记录组占用标志
	const unsigned char GetEGroupUseFlag()const;
	// 设置预警记录个数
	void UpdateSetEWarnCount(const unsigned char);
	// 获取预警记录个数
	const unsigned char GetEWarnCount()const;
	// 获取信息地雷最近Crc
	const unsigned int GetLatestInfoMineCrc()const;
	// 设置信息地雷最近Crc
	void UpdateSetLatestInfoMineCrc(const unsigned int);
};

#pragma pack()

#endif	/* _INCLUDE_EARLY_WARNING_H */

