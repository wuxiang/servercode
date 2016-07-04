#include "StkCodeOperate.h"
#include "../../../util/util.h"

// ת����������С
static const int MaxExchangeBufSize = 1280;
// ��ʽ���ַ���Ĭ�ϳ���
static const short DefaultFormatLen = 128;

CStkCodeOperate::CStkCodeOperate(const unsigned short AlignLen)
	: m_uAlignLen(AlignLen)
{
}

CStkCodeOperate::~CStkCodeOperate(void)
{
}

// ��Ӵ���
const int
CStkCodeOperate::AddCode(const char *StkCode, char *OutBuf, unsigned int &OccupySize, 
						 const unsigned int MaxBufSize)
{
	char cExcBuf[MaxExchangeBufSize] = {0};
	if (NULL == StkCode || NULL == OutBuf)
	{
		return -1;
	}

	char cFormat[DefaultFormatLen] = {0};
	snprintf(cFormat, DefaultFormatLen, "%%%us", m_uAlignLen);

	int iRes = 0;
	char cCode[16] = {0};
	iRes = snprintf(cCode, m_uAlignLen + 1, cFormat, StkCode);
	if ((unsigned int)iRes > m_uAlignLen)
	{
		return -2;
	}

	char *pFind = strstr(OutBuf, cCode);
	if (NULL == pFind)
	{
		if (OccupySize < MaxBufSize)
		{
			iRes = snprintf(cExcBuf, MaxExchangeBufSize, "%s%s", cCode, OutBuf);
			if (iRes > MaxExchangeBufSize)
			{
				return -3;
			}
			OccupySize += m_uAlignLen;
		}
		else if (OccupySize == MaxBufSize)
		{
			iRes = snprintf(cExcBuf, MaxExchangeBufSize, "%s", cCode);
			strncat(cExcBuf, OutBuf, OccupySize - m_uAlignLen);
		}
		else 
		{
			return -4;
		}
	}
	else
	{
		int iRemainLen = OccupySize - strlen(pFind);
		iRes = snprintf(cExcBuf, MaxExchangeBufSize, "%s", cCode);
		if (iRemainLen > 0)
		{
			strncat(cExcBuf, OutBuf, iRemainLen);
		}

		pFind += m_uAlignLen;
		if (NULL != pFind && '\0' != *pFind)
		{
			strcat(cExcBuf, pFind);
		}
	}

	memset(OutBuf, 0, MaxBufSize);
	strncpy(OutBuf, cExcBuf, MaxBufSize);
	return 0;
} 


// ɾ������
const int
CStkCodeOperate::RemoveCode(const char *StkCode, char *OutBuf, unsigned int &OccupySize,
	const unsigned int MaxBufSize)
{
	char cExcBuf[MaxExchangeBufSize] = {0};
	if (NULL == StkCode || NULL == OutBuf)
	{
		return -1;
	}

	char cFormat[DefaultFormatLen] = {0};
	snprintf(cFormat, DefaultFormatLen, "%%%us", m_uAlignLen);

	int iRes = 0;
	char cCode[16] = {0};
	iRes = snprintf(cCode, m_uAlignLen + 1, cFormat, StkCode);
	if (iRes > m_uAlignLen)
	{
		return -2;
	}

	char *pFind = strstr(OutBuf, cCode);
	if (NULL == pFind)
	{
		return -3;
	}

	int iRemainLen = OccupySize - strlen(pFind);
	if (iRemainLen > 0)
	{
		strncat(cExcBuf, OutBuf, iRemainLen);
	}

	pFind += m_uAlignLen;
	if (NULL != pFind && '\0' != *pFind)
	{
		strcat(cExcBuf, pFind);
	}

	OccupySize -= m_uAlignLen;
	memset(OutBuf, 0, MaxBufSize);
	strncpy(OutBuf, cExcBuf, MaxBufSize);
	return 0;
}

// ��շ���������
const int 
CStkCodeOperate::ClearAllCode(char *OutBuf, unsigned int &OccupySize,
	const unsigned int MaxBufSize)
{
	if (NULL == OutBuf)
	{
		return -1;
	}
	memset(OutBuf, 0, MaxBufSize);
	OccupySize = 0;

	return 0;
}

// ��ȡָ���������г�����
const char*
CStkCodeOperate::ReadIndexCode(const char *SrcBuf, const unsigned int OccupySize,
	const unsigned int CodeIndex, char *OutBuf, const unsigned int BufSize)
{
	int iCount = OccupySize / m_uAlignLen;
	if (iCount <= 0 || NULL == SrcBuf)
	{
		return NULL;
	}

	if (NULL == OutBuf || BufSize < m_uAlignLen || BufSize < 1)
	{
		return NULL;
	}
	memset(OutBuf, 0, BufSize);
	strncpy(OutBuf, SrcBuf + CodeIndex * m_uAlignLen, m_uAlignLen);
	return OutBuf;
}

// ����ָ��������Դ�е�����λ��
const int
CStkCodeOperate::FindStkCode(const char *SrcBuf, const unsigned int OccupySize,
	const char *StkCode)
{
	if (NULL == StkCode || NULL == SrcBuf)
	{
		return -1;
	}

	char cFormat[DefaultFormatLen] = {0};
	snprintf(cFormat, DefaultFormatLen, "%%%us", m_uAlignLen);

	int iRes = 0;
	char cCode[16] = {0};
	iRes = snprintf(cCode, m_uAlignLen + 1, cFormat, StkCode);
	if ((unsigned int)iRes > m_uAlignLen)
	{
		return -2;
	}

	char *pFind = strstr(SrcBuf, cCode);
	if (NULL == pFind)
	{
		return -3;
	}
	
	int iRemainLen = OccupySize - strlen(pFind);	
	return iRemainLen / m_uAlignLen;
}

