#ifndef _INCLUDE_STOCKCODE_OPERATE_H
#define _INCLUDE_STOCKCODE_OPERATE_H

class CStkCodeOperate
{
public:
	CStkCodeOperate(const unsigned short AlignLen);
	~CStkCodeOperate(void);
	
private:
	// 代码对齐长度
	const unsigned short m_uAlignLen;

public:
	// 添加代码(返回结果 <0:失败，对应的正整数为错误代码 0:成功)
	const int AddCode(const char *StkCode, char *OutBuf, unsigned int &OccupySize,
		const unsigned int MaxBufSize);
	// 删除代码
	const int RemoveCode(const char *StkCode, char *OutBuf, unsigned int &OccupySize,
		const unsigned int MaxBufSize);
	// 清空服务器代码
	const int ClearAllCode(char *OutBuf, unsigned int &OccupySize,
		const unsigned int MaxBufSize);
	// 读取指定索引的市场代码
	const char* ReadIndexCode(const char *SrcBuf, const unsigned int OccupySize,
		const unsigned int CodeIndex, char *OutBuf, const unsigned int BufSize);
	// 查找指定代码在源中的索引位置
	const int FindStkCode(const char *SrcBuf, const unsigned int OccupySize,
		const char *StkCode);
};

#endif	/* _INCLUDE_STOCKCODE_OPERATE_H */
