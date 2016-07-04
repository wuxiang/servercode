#ifndef _INCLUDE_STOCKCODE_OPERATE_H
#define _INCLUDE_STOCKCODE_OPERATE_H

class CStkCodeOperate
{
public:
	CStkCodeOperate(const unsigned short AlignLen);
	~CStkCodeOperate(void);
	
private:
	// ������볤��
	const unsigned short m_uAlignLen;

public:
	// ��Ӵ���(���ؽ�� <0:ʧ�ܣ���Ӧ��������Ϊ������� 0:�ɹ�)
	const int AddCode(const char *StkCode, char *OutBuf, unsigned int &OccupySize,
		const unsigned int MaxBufSize);
	// ɾ������
	const int RemoveCode(const char *StkCode, char *OutBuf, unsigned int &OccupySize,
		const unsigned int MaxBufSize);
	// ��շ���������
	const int ClearAllCode(char *OutBuf, unsigned int &OccupySize,
		const unsigned int MaxBufSize);
	// ��ȡָ���������г�����
	const char* ReadIndexCode(const char *SrcBuf, const unsigned int OccupySize,
		const unsigned int CodeIndex, char *OutBuf, const unsigned int BufSize);
	// ����ָ��������Դ�е�����λ��
	const int FindStkCode(const char *SrcBuf, const unsigned int OccupySize,
		const char *StkCode);
};

#endif	/* _INCLUDE_STOCKCODE_OPERATE_H */
