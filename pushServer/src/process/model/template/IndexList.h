#ifndef _INCLUDE_INDEX_LIST_H
#define _INCLUDE_INDEX_LIST_H

#include <set>

struct IndexMap
{
	unsigned int UserMapIndex;
	unsigned char PlatformCode;
	
	IndexMap()
	{
	}

	IndexMap(const unsigned int Index, const unsigned char code)
		: UserMapIndex(Index),
		PlatformCode(code)
	{
	}

	IndexMap& operator = (const IndexMap &item)
	{
		UserMapIndex = item.UserMapIndex;
		PlatformCode = item.PlatformCode;
		return *this;
	}
};

struct IndexCmpFun
{
	bool operator ()(const IndexMap &left, const IndexMap &right) const
	{
		if (left.UserMapIndex == right.UserMapIndex)
		{
			return left.PlatformCode < right.PlatformCode;
		}
		return left.UserMapIndex < right.UserMapIndex;
	}
};

class CIndexList
{
public:
	CIndexList();
	~CIndexList();
	
private:
	std::set<IndexMap, IndexCmpFun> m_IndexList;
	std::set<IndexMap, IndexCmpFun>::const_iterator m_iterSortedEnum;
	
public:
	// 清空列表
	void ClearList();
	// 添加
	bool AddIndexList(const unsigned int, const unsigned char);
	// 移除
	bool RemoveIndexList(const unsigned int, const unsigned char);
	// 查找
	bool FindIndexList(const unsigned int, const unsigned char);
	// 获取第一个元素
	IndexMap* GetFirst();
	// 获取接下来的元素
	IndexMap* GetNext();
	// 获取个数
	int GetCount();
	// 恢复到指定位置
	bool RestoreToPos(IndexMap &);
};

#endif		/* _INCLUDE_INDEX_LIST_H */

