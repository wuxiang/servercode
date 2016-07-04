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
	// 0 - 1λ ��ʶԤ����������Ч��
	EWCP_PERIOD_VALIDITY_ONCE =						0x0000,				// һ����Ч(ֻҪ����һ�Σ������Ϊ��Ч)
	EWCP_PERIOD_VALIDITY_ALWAYS =					0x0001,				// ������Ч(ÿ������һ�Σ�������Ч)
	EWCP_PERIOD_VALIDITY_ONEDAY =					0x0002,				// һ����Ч(һ���ڣ��������һ�Σ����������Ƿ����㶼����Ч)

	// 2λ ��ʶ�Ƿ��Ѿ�����һ������
	EWCP_NO_MATCH_CONDITION =						0x0000,				// ��δ���������
	EWCP_HAVE_MATCH_CONDITION =						0x0004,				// �Ѿ���������һ��

	// 3 - 4λ ��ʶ��ǰ��Ԥ����������Ч��
	EWCP_UNDER_MONITORING =							0x0000,				// ���ڼ����
	EWCP_UNDER_WAITING =							0x0008,				// ��Ч�����Ѿ����һ�Σ��ȴ��´δ���
	EWCP_OUTOFDATE =								0x0010,				// �Ѿ�����ʧЧ
	EWCP_INVALID =									0x0018,				// �Ƿ���Ч����

	// 5λ ��ʶ״̬�Ƿ���Ҫ����
	EWCP_KEEP_STATE =								0x0000,				// ���ֵ�ǰ��״̬
	EWCP_RESET_STATE =								0x0020				// ��Ҫ����״̬���������´μ��
};

// ���űȽ�����
enum CompareTypeEnum
{
	GREATER_THAN = 0,													// >
	SMALLER_THAN,														// <
	EQUAL_TO,															// ==
	GREATER_AND_EQUAL,													// >=
	SMALLER_AND_EQUAL													// <=
};

// Ԥ������
enum WarningTypeEnum
{
	PriceGreater = 0,													// �ɼ۸���
	PriceSmaller,														// �ɼ۵���
	IncreaseGreater,													// �Ƿ�����
	DecreaseGreater,													// ��������
	ExchangeGreater,													// �����ʴ���
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
	unsigned int m_uTriggerValue;										// ������ֵ
	unsigned short m_sProperty;											// �����ֶ�
	unsigned char m_eCompareType;										// �Ƚ�����

private:
	void SetProperty(unsigned short Property);							// �������ԣ���Ҫȡ���������ֶ�һ������

public:
	CEarlyWarningCondition& operator = (const CEarlyWarningCondition &item);

	const unsigned short GetPeriodValidityProperty()const;				// ȡ��Ч������
	const unsigned short GetMatchConditionyProperty()const;				// ȡ��ƥ������
	const unsigned short GetEarlyWarningStatusProperty()const;			// ȡ��Ч������
	const unsigned short GetRequireResetProperty()const;				// ȡ���Ƿ���������
	const unsigned short GetProperty()const;							// ��ȡ����ֵ
	const unsigned char GetRawGetPeriodProperty()const;					// ��ȡ��Ч�ڵ�ԭʼ��ʾ

	const unsigned int GetTriggerValue()const;							// ȡ������ֵ
	const unsigned int GetConditionValue()const;						// ��ȡ�ڵ�ǰ�����µĴ���ֵ(��ЧΪ�� ����ΪGetTriggerValue)
	bool ExecuteCompare(const unsigned int value);						// ִ�бȽϣ�������Ӧ������λ
	bool CanExecuteCmp()const;											// �Ƿ����ִ�д����Ƚ�
	void Reset();														// ���ýڵ�
	const unsigned char IsRemainValid()const;							// �Ƿ���Ȼ��Ч
	bool EqualZero();													// �Ƿ�Ϊ��
};

//m_cRecordProperty�ֶ�ȡֵ
enum
{
	// 6 - 7λ ��Ϣͬ������Ĳ�����ʽ
	WARNINGSET_INSERTDB =	0x80,	// ��ʾ����ϢҪinsert��DB��
	WARNINGSET_UPDATEDB =	0x40,	// ��ʾ����ϢҪupdate��DB��
	WARNINGSET_DELDB	=	0xc0,	// ��ʾ����Ϣ��DB��delete(!!!���˱�ʾ������ʽ�⣬ͬʱҲ��ʾ�ü�¼�Ƿ���Ч����ˣ�ͬ����Ϻ����ÿ�)
	WARNINGSET_NULL = 0x00,			// �ÿ�

	// 5λ ��ʶ�Ƿ������˽��ոù�Ʊ��Ϣ����
	WARNINGSET_NEW =	0x20,

	// 4λ ��ʾ������Ϣ�Ƿ����ʹ��(������һ���Ƿ��Ѿ���ռ��, ֻ�������׽ڵ�λ)
	WARNINGSET_USE =	0x10,

	// 0 - 3λ ��ʶ�Ѿ����õ�Ԥ����¼����(ֻ�������׽ڵ�λ)
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
	char m_strStkCode[MAX_STKCODE_LEN];										// ��Ʊ����
	unsigned char m_cRecordProperty;										// ��¼�������ֶ�
	// ����Ϊ���ɼ۴��� �ɼ۵��� �Ƿ����� �Ƿ����� �ջ����ʴ���
	CEarlyWarningCondition m_EarlyWarningCondition[EARLY_WARNING_TYPE_NUM];	// �۸�Ԥ����¼��Ա
	unsigned short m_nStkIndex;												// ��ǰ��Ʊ��������, �����0xFFFF��ʾ�ù�Ʊû���ҵ�ƥ��
	TIME_T32 m_nBeginCmpTime;												// ��Ч�Ƚ����ʱ��
	unsigned int m_uLatestInfoMineCrc;										// ��Ϣ�������Crc

private:
	// ִ�бȽ�
	bool ExcuteTypeCmp(const struct PriceAlarm *, const unsigned char);
	// ������Ϣ��������
	void UpdateSetInfoMineProperty(const unsigned char SetFlag);
	// ���±Ƚ����ֵ
	void UpdateBeginCmpTime();
	// ��ȡ�Ƚ����ֵ
	const TIME_T32 GetBeginCmpTime()const;

public:
	CEarlyWarningConditionRecord& operator = (const CEarlyWarningConditionRecord &item);
	// ����Ԥ����¼
	void SetCondition(const CEarlyWarningCondition&, const unsigned char);
	// ��ȡԤ����¼
	CEarlyWarningCondition* GetCondition(const unsigned char);
	// ִ��ȫ������ƥ��(�������з����Ƿ�����������1��ʶ���� 0��ʶ��ƥ��)
	int ExcuteCmp(struct PriceAlarm *);
	// ��ȡÿ��Ԥ����¼�ĳ�Ա��Ŀ
	static const int GetRecordMemCount();
	// ��ȡ��Ӧλ��Ӧ��Ԥ������
	enum WarningTypeEnum GetWarningTypeByBit(const int);
	// ������ӦΪ��Ӧ��Ԥ����¼����Ч״̬
	unsigned char GetWarningValidationByBit(const int);
	// ���ýڵ�״̬
	int ResetRecordState();
	// ��ȡ֤ȯ����
	const char* GetStkCode()const;
	// ��ȡ֤ȯ����
	const char* GetStkName()const;
	// ������Ч�ļ�¼
	void UpdateValidRecord(const CEarlyWarningConditionRecord*);
	// ��ȡ������С��λ����
	const unsigned char GetFloatPrecision();
	// ���ýڵ��Ӧ���¶�̬����ָ��
	void ResetStkLatestInfo();

	// ��ȡ��Ϣ������������
	const unsigned char GetInforMineProper()const;
	// ��ȡ��Ϣ�������õ�ԭʼ����
	const unsigned char GetInforMineRawProper()const;
	// ����Ԥ����¼ͬ������Ĳ�����ʽ
	void UpdateSetOperToDb(const unsigned char);
	// ��ȡԤ����¼ͬ������Ĳ�����ʽ
	const unsigned char GetOperToDb()const;
	// ����Ԥ����¼��ռ�ñ�־
	void UpdateSetEGroupUseFlag(const unsigned char);
	// ��ȡԤ����¼��ռ�ñ�־
	const unsigned char GetEGroupUseFlag()const;
	// ����Ԥ����¼����
	void UpdateSetEWarnCount(const unsigned char);
	// ��ȡԤ����¼����
	const unsigned char GetEWarnCount()const;
	// ��ȡ��Ϣ�������Crc
	const unsigned int GetLatestInfoMineCrc()const;
	// ������Ϣ�������Crc
	void UpdateSetLatestInfoMineCrc(const unsigned int);
};

#pragma pack()

#endif	/* _INCLUDE_EARLY_WARNING_H */

