#include "EarlyWarning.h"
#include "../../../util/log/log.h"
#include "../../../util/common/common_lib.h"
#include "../../../util/common/common_types.h"
#include "../../../util/string/string_util.h"
#include "../../../controller/runtime.h"
#include "../../quota/QuotesSnapshot.h"
#include <math.h>
#include <map>

using namespace std;


CEarlyWarningCondition::CEarlyWarningCondition(const unsigned int value, 
	unsigned short Property, unsigned char cmp)
	: m_uTriggerValue(value),
	m_sProperty(Property),
	m_eCompareType(cmp)
{
}

CEarlyWarningCondition::CEarlyWarningCondition()
	: m_uTriggerValue(0),
	m_sProperty(EWCP_PERIOD_VALIDITY_ALWAYS | EWCP_NO_MATCH_CONDITION | EWCP_INVALID),
	m_eCompareType(GREATER_THAN)
{
}

bool
CEarlyWarningCondition::Construct(const unsigned int value, unsigned short Property,
	unsigned char cmp)
{
	m_uTriggerValue = value;
	SetProperty(Property);
	m_eCompareType = cmp;
	return true;
}

CEarlyWarningCondition::~CEarlyWarningCondition()
{
	
}

// 设置属性，需要取其它属性字段一起设置
void
CEarlyWarningCondition::SetProperty(unsigned short Property)
{
	m_sProperty = Property;
}

CEarlyWarningCondition&
CEarlyWarningCondition::operator = (const CEarlyWarningCondition &item)
{
	m_uTriggerValue = item.m_uTriggerValue;
	m_sProperty = item.m_sProperty;
	m_eCompareType = item.m_eCompareType;
	return *this;
}

// 获取有效期的原始表示
const unsigned char 
CEarlyWarningCondition::GetRawGetPeriodProperty()const
{
	const unsigned short uVal = GetPeriodValidityProperty();
	if (EWCP_PERIOD_VALIDITY_ALWAYS == uVal)
	{
		return 1;
	}
	else if (EWCP_PERIOD_VALIDITY_ONCE == uVal)
	{
		return 0;
	}
	return 2;
}

// 执行比较，并置相应的属性位
bool
CEarlyWarningCondition::ExecuteCompare(const unsigned int value)
{
	bool bResMatch = false;
	if (0xFFFFFFFF == value)
	{
		return false;
	}
	
	switch(m_eCompareType)
	{
	case GREATER_THAN:
		bResMatch = value > m_uTriggerValue;
		break;

	case SMALLER_THAN:
		bResMatch = value < m_uTriggerValue;
		break;

	case EQUAL_TO:
		bResMatch = value == m_uTriggerValue;
		break;

	case GREATER_AND_EQUAL:
		bResMatch = value >= m_uTriggerValue;
		break;

	case SMALLER_AND_EQUAL:
		bResMatch = value <= m_uTriggerValue;
		break;

	default:
		break;
	}

	if (bResMatch)
	{
		unsigned short uRequireResetProperty = EWCP_KEEP_STATE;
		unsigned short uEarlyWarningStatusProperty = EWCP_UNDER_WAITING;
		unsigned short uPeriodValidityProperty = GetPeriodValidityProperty();
		if (EWCP_PERIOD_VALIDITY_ALWAYS == uPeriodValidityProperty)
		{
			uRequireResetProperty = EWCP_RESET_STATE;
			uEarlyWarningStatusProperty = EWCP_UNDER_WAITING;
		}
		else if (EWCP_PERIOD_VALIDITY_ONCE == uPeriodValidityProperty)
		{
			uRequireResetProperty = EWCP_KEEP_STATE;
			uEarlyWarningStatusProperty = EWCP_OUTOFDATE;
		}
		else if (EWCP_PERIOD_VALIDITY_ONEDAY == uPeriodValidityProperty)
		{
			uRequireResetProperty = EWCP_KEEP_STATE;
			uEarlyWarningStatusProperty = EWCP_OUTOFDATE;
		}

		SetProperty(EWCP_HAVE_MATCH_CONDITION | uEarlyWarningStatusProperty | uPeriodValidityProperty
				| uRequireResetProperty);
	}

	return bResMatch;
}

// 是否为零
bool
CEarlyWarningCondition::EqualZero()
{
	return 0 == m_uTriggerValue;
}		

// 是否可以执行触发比较
bool
CEarlyWarningCondition::CanExecuteCmp()const
{
	return (EWCP_UNDER_MONITORING == GetEarlyWarningStatusProperty()
			&& EWCP_NO_MATCH_CONDITION == GetMatchConditionyProperty());
}

// 取有效期属性
const unsigned short 
CEarlyWarningCondition::GetPeriodValidityProperty()const				
{
	return (unsigned short)(m_sProperty & 0x0003);
}

// 取有匹配属性
const unsigned short 
CEarlyWarningCondition::GetMatchConditionyProperty()const				
{
	return (unsigned short)(m_sProperty & 0x0004);
}

// 取有效性属性
const unsigned short 
CEarlyWarningCondition::GetEarlyWarningStatusProperty()const			
{
	return (unsigned short)(m_sProperty & 0x0018);
}

// 取有是否重置属性
const unsigned short 
CEarlyWarningCondition::GetRequireResetProperty()const					
{
	return (unsigned short)(m_sProperty & 0x0020);
}

// 取触发点值
const unsigned int
CEarlyWarningCondition::GetTriggerValue()const									
{
	return m_uTriggerValue;
}

const unsigned int
CEarlyWarningCondition::GetConditionValue()const
{
	const unsigned short uProperty = GetEarlyWarningStatusProperty();
	const unsigned short uPeriodP = GetPeriodValidityProperty();
	if (EWCP_UNDER_MONITORING == uProperty
		|| (uPeriodP == EWCP_PERIOD_VALIDITY_ALWAYS && EWCP_INVALID != uProperty))
	{
		return GetTriggerValue();
	}
	return (unsigned int)-1;
}

// 重置节点
void
CEarlyWarningCondition::Reset()
{
	if (EWCP_RESET_STATE == GetRequireResetProperty())
	{
		SetProperty(GetPeriodValidityProperty() | EWCP_NO_MATCH_CONDITION | EWCP_UNDER_MONITORING | EWCP_KEEP_STATE);
	}
}

// 是否依然有效
const unsigned char 
CEarlyWarningCondition::IsRemainValid()const
{
	const unsigned short uProperty = GetEarlyWarningStatusProperty();
	if (EWCP_UNDER_MONITORING != uProperty)
	{
		return 1;
	}
	return 0;
}

// 获取属性值
const unsigned short 
CEarlyWarningCondition::GetProperty()const
{
	return m_sProperty;
}



CEarlyWarningConditionRecord::CEarlyWarningConditionRecord()
{
	bzero(m_strStkCode, MAX_STKCODE_LEN);
	m_cRecordProperty = 0;
	bzero(m_EarlyWarningCondition, sizeof(CEarlyWarningCondition) * EARLY_WARNING_TYPE_NUM);
	m_nStkIndex = 0xFFFF;
	m_nBeginCmpTime = GetNowTime();
	m_uLatestInfoMineCrc = 0;
}

CEarlyWarningConditionRecord::~CEarlyWarningConditionRecord()
{
}

// 更新比较起点值
void
CEarlyWarningConditionRecord::UpdateBeginCmpTime()
{
	m_nBeginCmpTime = GetNowTime();
}

// 获取比较起点值
const TIME_T32
CEarlyWarningConditionRecord::GetBeginCmpTime()const
{
	return m_nBeginCmpTime;
}

// 设置信息地雷属性
void 
CEarlyWarningConditionRecord::UpdateSetInfoMineProperty(const unsigned char SetFlag)
{
	if (0 != SetFlag)
	{
		BitSet(m_cRecordProperty, 5);
	}
	else
	{
		BitUnSet(m_cRecordProperty, 5);
	}
}

// 获取信息地雷设置属性
const unsigned char 
CEarlyWarningConditionRecord::GetInforMineProper()const
{
	/* 注释信息地雷
	return m_cRecordProperty & WARNINGSET_NEW;*/
	
	return 0;
}

// 获取信息地雷设置的原始属性
const unsigned char
CEarlyWarningConditionRecord::GetInforMineRawProper()const
{
	/* 注释信息地雷
	unsigned char uProper = m_cRecordProperty & WARNINGSET_NEW;
	uProper >>= 5;
	return uProper;*/
	
	return 0;
}

int
CEarlyWarningConditionRecord::Construct(const char* StkCode, const unsigned char SetMine)
{
	StrCopy(m_strStkCode, StkCode, MAX_STKCODE_LEN);
	UpdateSetInfoMineProperty(SetMine);
	m_nBeginCmpTime = GetNowTime();
	
	m_nStkIndex = CQuotesSnapshot::findSecurityIndex(m_strStkCode);
	if (0xFFFF == m_nStkIndex)
	{
		if (CQuotesSnapshot::HasInitialed())
		{
			// 选择的股票不支持
			return -5;
		}
		// 正在初始化
		return -22;
	}
	
	PriceAlarm *pStkLatestInfo = CQuotesSnapshot::GetIndexSecurity(m_nStkIndex, m_strStkCode);
	if (NULL == pStkLatestInfo)
	{
		// 正在初始化
		return -22;
	}
	return pStkLatestInfo->m_cPrecision;
}

bool
CEarlyWarningConditionRecord::ExcuteTypeCmp(const struct PriceAlarm *CurrentValue,
		const unsigned char serial)
{
	if (NULL == CurrentValue)
	{
		return false;
	}

	CEarlyWarningCondition *pCondition = NULL;
	unsigned int uTypeValue = 0;
	switch(serial)
	{
	case PriceGreater:
		pCondition = &m_EarlyWarningCondition[0];
		uTypeValue = CurrentValue->m_Price;
		break;

	case PriceSmaller:
		pCondition = &m_EarlyWarningCondition[1];
		uTypeValue = CurrentValue->m_Price;
		break;

	case IncreaseGreater:
		pCondition = &m_EarlyWarningCondition[2];
		uTypeValue = (CurrentValue->m_Change < 0 ? 0xFFFFFFFF : CurrentValue->m_Change);
		break;

	case DecreaseGreater:
		pCondition = &m_EarlyWarningCondition[3];
		uTypeValue = CurrentValue->m_Change > 0 ? 0xFFFFFFFF : (CurrentValue->m_Change * -1);
		break;

	case ExchangeGreater:
		pCondition = &m_EarlyWarningCondition[4];
		uTypeValue = CurrentValue->m_TurnoverRate;
		break;
	}
	
	if (!CurrentValue->IsValid(GetBeginCmpTime()))
	{
		return false;
	}
	
	if (NULL  != pCondition && pCondition->CanExecuteCmp())
	{
		return pCondition->ExecuteCompare(uTypeValue);
	}

	return false;
}

CEarlyWarningConditionRecord&
CEarlyWarningConditionRecord::operator = (const CEarlyWarningConditionRecord &item)
{
	bzero(m_strStkCode, MAX_STKCODE_LEN);
	StrCopy(m_strStkCode, item.m_strStkCode, MAX_STKCODE_LEN);
	UpdateSetInfoMineProperty(item.GetInforMineProper());
	memcpy(m_EarlyWarningCondition, item.m_EarlyWarningCondition, EARLY_WARNING_TYPE_NUM * sizeof(CEarlyWarningCondition));
	m_nStkIndex = item.m_nStkIndex;
	m_nBeginCmpTime = item.m_nBeginCmpTime;
	m_uLatestInfoMineCrc = item.m_uLatestInfoMineCrc;
	return *this;
}

void
CEarlyWarningConditionRecord::SetCondition(const CEarlyWarningCondition &item, const unsigned char serial)
{
	switch(serial)
	{
	case PriceGreater:
		m_EarlyWarningCondition[0] = item;
		break;

	case PriceSmaller:
		m_EarlyWarningCondition[1] = item;
		break;

	case IncreaseGreater:
		m_EarlyWarningCondition[2] = item;
		break;

	case DecreaseGreater:
		m_EarlyWarningCondition[3] = item;
		break;

	case ExchangeGreater:
		m_EarlyWarningCondition[4] = item;
		break;
	}
}

// 获取预警记录
CEarlyWarningCondition* 
CEarlyWarningConditionRecord::GetCondition(const unsigned char serial)
{
	CEarlyWarningCondition *pRes = NULL;
	switch(serial)
	{
	case PriceGreater:
		pRes = &m_EarlyWarningCondition[0];
		break;

	case PriceSmaller:
		pRes = &m_EarlyWarningCondition[1];
		break;

	case IncreaseGreater:
		pRes = &m_EarlyWarningCondition[2];
		break;

	case DecreaseGreater:
		pRes = &m_EarlyWarningCondition[3];
		break;

	case ExchangeGreater:
		pRes = &m_EarlyWarningCondition[4];
		break;
	}
	return pRes;
}

// 执行全部条件匹配(按照序列返回是否满足条件，1标识满足 0标识不匹配)
int
CEarlyWarningConditionRecord::ExcuteCmp(struct PriceAlarm *CurrentValue)
{
	int iValue = 0;
	PriceAlarm *pStkLatestInfo = NULL;
	// 查找当前最新
	if (0xFFFF == m_nStkIndex)
	{
		m_nStkIndex = CQuotesSnapshot::findSecurityIndex(m_strStkCode);
	}
	
	if (0xFFFF == m_nStkIndex)
	{
		return iValue;
	}
	pStkLatestInfo = CQuotesSnapshot::GetIndexSecurity(m_nStkIndex, m_strStkCode);
	if (NULL == pStkLatestInfo)
	{
		return iValue;
	}
	*CurrentValue = *pStkLatestInfo;
	
	bool bRes = ExcuteTypeCmp(CurrentValue, PriceGreater);
	if (bRes)
	{
		BitSet(iValue, 0);
	}

	bRes = ExcuteTypeCmp(CurrentValue, PriceSmaller);
	if (bRes)
	{
		BitSet(iValue, 1);
	}

	bRes = ExcuteTypeCmp(CurrentValue, IncreaseGreater);
	if (bRes)
	{
		BitSet(iValue, 2);
	}

	bRes = ExcuteTypeCmp(CurrentValue, DecreaseGreater);
	if (bRes)
	{
		BitSet(iValue, 3);
	}

	bRes = ExcuteTypeCmp(CurrentValue, ExchangeGreater);
	if (bRes)
	{
		BitSet(iValue, 4);
	}

	return iValue;
}

// 获取每条预警记录的成员数目
const int
CEarlyWarningConditionRecord::GetRecordMemCount()
{
	return EARLY_WARNING_TYPE_NUM;
}

// 获取相应位对应的预警类型
enum WarningTypeEnum
CEarlyWarningConditionRecord::GetWarningTypeByBit(const int BitValue)
{
	switch(BitValue)
	{
		case 0:
			return PriceGreater;

		case 1:
			return PriceSmaller;

		case 2:
			return IncreaseGreater;

		case 3:
			return DecreaseGreater;

		case 4:
			return ExchangeGreater;

		default:
			break;
	}

	return PriceGreater;
}

// 返回相应为对应的预警记录的有效状态
unsigned char 
CEarlyWarningConditionRecord::GetWarningValidationByBit(const int BitValue)
{
	switch(BitValue)
	{
		case 0:
			return (unsigned char)m_EarlyWarningCondition[0].IsRemainValid();

		case 1:
			return (unsigned char)m_EarlyWarningCondition[1].IsRemainValid();

		case 2:
			return (unsigned char)m_EarlyWarningCondition[2].IsRemainValid();

		case 3:
			return (unsigned char)m_EarlyWarningCondition[3].IsRemainValid();

		case 4:
			return (unsigned char)m_EarlyWarningCondition[4].IsRemainValid();

		default:
			break;
	}
	return 1;
}

// 重置节点状态
int 
CEarlyWarningConditionRecord::ResetRecordState()
{
	UpdateBeginCmpTime();
	for (int i = 0 ; i < EARLY_WARNING_TYPE_NUM; i++)
	{
		m_EarlyWarningCondition[i].Reset();
	}
	
	// 等待重新初始化
	ResetStkLatestInfo();
	
	return 0;
}

// 重置节点对应最新动态数据指针
void
CEarlyWarningConditionRecord::ResetStkLatestInfo()
{
	m_nStkIndex = 0xFFFF;
}
	
// 获取证券代码
const char*
CEarlyWarningConditionRecord::GetStkCode()const
{
	return m_strStkCode;
}

// 获取证券名称
const char* 
CEarlyWarningConditionRecord::GetStkName()const
{
	if (0xFFFF == m_nStkIndex)
	{
		return NULL;
	}
	
	PriceAlarm *pStkLatestInfo = CQuotesSnapshot::GetIndexSecurity(m_nStkIndex, m_strStkCode);
	if (NULL == pStkLatestInfo)
	{
		return NULL;
	}
	return pStkLatestInfo->m_strName;
}

// 更新有效的记录
void
CEarlyWarningConditionRecord::UpdateValidRecord(const CEarlyWarningConditionRecord *record)
{
	if (NULL != record)
	{
		bzero(m_strStkCode, MAX_STKCODE_LEN);
		StrCopy(m_strStkCode, record->m_strStkCode, MAX_STKCODE_LEN);
		UpdateSetInfoMineProperty(record->GetInforMineProper());
		m_nStkIndex = record->m_nStkIndex;
		m_nBeginCmpTime = record->m_nBeginCmpTime;
		memcpy(m_EarlyWarningCondition, record->m_EarlyWarningCondition, EARLY_WARNING_TYPE_NUM * sizeof(CEarlyWarningCondition));
	}
}

// 获取浮点型小数位精度
const unsigned char
CEarlyWarningConditionRecord::GetFloatPrecision()
{
	if (0xFFFF == m_nStkIndex)
	{
		return 2;
	}
	
	PriceAlarm *pStkLatestInfo = CQuotesSnapshot::GetIndexSecurity(m_nStkIndex, m_strStkCode);
	if (NULL == pStkLatestInfo)
	{
		return 2;
	}
	return pStkLatestInfo->m_cPrecision;
}

// 设置预警记录同步到库的操作方式
void 
CEarlyWarningConditionRecord::UpdateSetOperToDb(const unsigned char Property)
{
	unsigned char cProperty = m_cRecordProperty & 0x3F;
	unsigned char uCurrentProperty = GetOperToDb();
	
	if (WARNINGSET_INSERTDB == uCurrentProperty)
	{
		if (WARNINGSET_DELDB == Property || WARNINGSET_NULL == Property)
		{
			m_cRecordProperty = Property | cProperty;
		}
		else
		{
			m_cRecordProperty = uCurrentProperty | cProperty;
		}
	}
	else
	{
		m_cRecordProperty = Property | cProperty;
	}
}

// 获取预警记录同步到库的操作方式
const unsigned char 
CEarlyWarningConditionRecord::GetOperToDb()const
{
	return m_cRecordProperty & 0xC0;
}

// 设置预警记录组占用标志
void 
CEarlyWarningConditionRecord::UpdateSetEGroupUseFlag(const unsigned char Property)
{
	unsigned char cProperty = m_cRecordProperty & 0xEF;
	m_cRecordProperty = Property | cProperty;
}

// 获取预警记录组占用标志
const unsigned char 
CEarlyWarningConditionRecord::GetEGroupUseFlag()const
{
	return m_cRecordProperty & WARNINGSET_USE;
}

// 设置预警记录个数
void 
CEarlyWarningConditionRecord::UpdateSetEWarnCount(const unsigned char Property)
{
	unsigned char cProperty = m_cRecordProperty & 0xF0;
	m_cRecordProperty = Property | cProperty;
}

// 获取预警记录个数
const unsigned char 
CEarlyWarningConditionRecord::GetEWarnCount()const
{
	return m_cRecordProperty & 0x0F;
}

// 获取信息地雷最近Crc
const unsigned int
CEarlyWarningConditionRecord::GetLatestInfoMineCrc()const
{
	return m_uLatestInfoMineCrc;
}

// 设置信息地雷最近Crc
void
CEarlyWarningConditionRecord::UpdateSetLatestInfoMineCrc(const unsigned int Crc)
{
	m_uLatestInfoMineCrc = Crc;
}

