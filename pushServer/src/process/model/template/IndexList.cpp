#include "IndexList.h"

using namespace std;

CIndexList::CIndexList()
{
	ClearList();
	m_iterSortedEnum = m_IndexList.end();
}

CIndexList::~CIndexList()
{
	ClearList();
}

// 清空列表
void 
CIndexList::ClearList()
{
	m_IndexList.clear();
}

// 添加
bool 
CIndexList::AddIndexList(const unsigned int index, const unsigned char cPlatform)
{
	IndexMap item(index, cPlatform);
	return (m_IndexList.insert(item)).second;
}

// 移除
bool 
CIndexList::RemoveIndexList(const unsigned int index, const unsigned char cPlatform)
{
	IndexMap item(index, cPlatform);
	set<IndexMap, IndexCmpFun>::iterator pIter = m_IndexList.find(item);
	if (m_IndexList.end() == pIter)
	{
		return false;
	}
	
	m_IndexList.erase(pIter);
	return true;
}

// 查找
bool
CIndexList::FindIndexList(const unsigned int index, const unsigned char cPlatform)
{
	IndexMap item(index, cPlatform);
	set<IndexMap, IndexCmpFun>::iterator pIter = m_IndexList.find(item);
	if (m_IndexList.end() == pIter)
	{
		return false;
	}
	
	return true;
}

// 获取第一个元素
IndexMap* 
CIndexList::GetFirst()
{
	m_iterSortedEnum = m_IndexList.begin();
	if (m_iterSortedEnum == m_IndexList.end())
	{
		return NULL;
	}
	
	return (IndexMap*)(&(*m_iterSortedEnum));
}

// 获取接下来的元素
IndexMap* 
CIndexList::GetNext()
{
	if ((int)m_IndexList.size() < 1 || 
		m_iterSortedEnum == m_IndexList.end())
	{
		return NULL;
	}
	
	m_iterSortedEnum++;
	if (m_iterSortedEnum == m_IndexList.end())
	{
		return NULL;
	}
	return (IndexMap*)(&(*m_iterSortedEnum));
}

// 获取个数
int
CIndexList::GetCount()
{
	return m_IndexList.size();
}

// 恢复到指定位置
bool
CIndexList::RestoreToPos(IndexMap &item)
{
	set<IndexMap, IndexCmpFun>::const_iterator pIter = m_IndexList.find(item);
	if (m_IndexList.end() == pIter)
	{
		return false;
	}
	
	m_iterSortedEnum = pIter;
	return true;
}

