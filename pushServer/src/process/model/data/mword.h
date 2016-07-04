#ifndef _MWORD_H_INCLUDE
#define _MWORD_H_INCLUDE

#include  "../../../util/util.h"
#include  "../../../util/common/common_types.h"

struct MWORD
{
	enum
	{
		MAXBASE = 0x1FFFFFFF,
	};

	int m_nBase:30;
	unsigned int m_nMul:2;

	MWORD(){};
	MWORD(int n);
	MWORD(int nBase,DWORD dwMul);
	operator INT64() const { return GetValue();}
	INT64 GetValue() const;
	INT64 GetABSValue() const;		//����ֵ
	MWORD operator-=(const MWORD& d);
	MWORD operator-=(const INT64 d);
	MWORD operator+=(const MWORD& d);
	MWORD operator+=(const INT64 d);
	MWORD operator*=(const MWORD& d);
	MWORD operator*=(const INT64 d);
	MWORD operator/=(const MWORD& d);
	MWORD operator/=(const INT64 d);
	INT64 operator+(const MWORD& d) const;
	INT64 operator-(const MWORD& d) const;
	INT64 operator*(const MWORD& d) const;
	INT64 operator/(const MWORD& d) const;
	INT64 operator+(const int d) const;
	INT64 operator-(const int d) const;
	INT64 operator*(const int d) const;
	INT64 operator/(const int d) const;
	MWORD operator=(const MWORD& d);
	MWORD operator=(const INT64 d);
	BOOL operator==(const MWORD& d) const;
	BOOL operator==(const INT64 d) const;
	BOOL operator==(const int d) const;
	BOOL operator!=(const MWORD& d) const;
	BOOL operator!=(const INT64 d) const;
	BOOL operator!=(const int d) const;
	BOOL operator>(int d) const;
	BOOL operator>(MWORD d) const;
	BOOL operator<(int d) const;
	BOOL operator<(MWORD d) const;
	BOOL IsZero() const {return m_nBase==0;}
	BOOL IsMinus() const {return m_nBase<0;}
	unsigned int GetMul(){return m_nMul;}
	unsigned int GetBase(){return m_nBase;}
	DWORD GetRawData();
	void SetRawData(DWORD dw);
};

#endif //_MWORD_H_INCLUDE
