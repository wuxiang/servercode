#ifndef _INCLUDE_DYNA_NODE_MANAGE_H
#define _INCLUDE_DYNA_NODE_MANAGE_H

/***
**	动态管理节点
**	需要保证同时只有一个线程进行写操作
**
***************************************/
#include "ContainerCmp.h"
#include <set>
#include <vector>
#include <string>
#include <map>
#include <utility>

// 节点插入
enum NodeAddResType
{
	// 节点已经满了
	NA_FULL = -2,
	// 发生错误
	NA_ERROR,
	// 新建节点
	NA_NEW,
	// 节点复用
	NA_REUSE
};

template<typename T>
class CDynaNodeManage
{
public:
	CDynaNodeManage(const unsigned int MaxNum)
		:m_iMaxNode(MaxNum)
	{
		m_ltNodeUsed.clear();
		m_vcNodes.clear();
		m_mpMarkNodes.clear();
		m_ltNotNodeUsed.clear();
		
		m_vcNodes.reserve(m_iMaxNode);
		
		for (int i = 0; i < (int)m_iMaxNode; i++)
		{
			m_ltNotNodeUsed.insert(m_ltNotNodeUsed.end(), i);
		}
	}

	virtual ~CDynaNodeManage(void)
	{
		m_ltNodeUsed.clear();
		m_vcNodes.clear();
		m_mpMarkNodes.clear();
		m_ltNotNodeUsed.clear();
	}
	

// 成员变量定义
protected:
	const unsigned int m_iMaxNode;								// 最大节点数目
	std::set<int> m_ltNodeUsed;									// 已经使用的连接节点索引
	std::set<int> m_ltNotNodeUsed;								// 尚未使用的连接节点索引
	std::vector<T*> m_vcNodes;									// 节点存储管理
	std::map<std::string, T*> m_mpMarkNodes;					// 节点关系映射
	typename std::set<int>::const_iterator m_iterSortedEnum;	// 有序排列数据头

// 局部函数定义
private:
	// 升序排列ID
	static int SortID(const void * item1, const void * item2)
	{
		if( NULL == item1 || NULL == item2)
		{
			return -1;
		}
		
		return *((int*)item1) - *((int*)item2);
	}
	
	// 获取尚未使用的最小节点ID
	int FindMinUnUsedID()									
	{
		int iNodeCount = (int)m_ltNotNodeUsed.size();
		if (0 == iNodeCount)
		{
			return -1;
		}
		const std::set<int>::const_iterator ite = m_ltNotNodeUsed.begin();
		return *ite;
	}

	// 添加占用的连接ID
	int AddUsedID(const int ID)								
	{
		m_ltNodeUsed.insert(ID);
		return 0;
	}
	
	// 添加空闲ID
	int AddUnUsedID(const int ID)							
	{
		m_ltNotNodeUsed.insert(ID);
		return 0;
	}

	// 移除占用的连接ID
	void RemoveUsedID(const int ID)							
	{
		typename std::set<int>::iterator iter = 
			m_ltNodeUsed.find(ID);
		
		if (iter != m_ltNodeUsed.end())
		{
			m_ltNodeUsed.erase(iter);
		}
	}

	// 移除空闲ID
	bool RemoveUnUsedID(const int ID)						
	{
		typename std::set<int>::iterator iter = 
			m_ltNotNodeUsed.find(ID);
		
		if (iter != m_ltNotNodeUsed.end())
		{
			m_ltNotNodeUsed.erase(iter);
			return true;
		}

		return false;
	}
	
	// 获取指定索引的Node，不存在创建
	T* GetIndexNode(const unsigned int index) 					
	{
		if (index >= (unsigned int)m_vcNodes.size())
		{
			return new T();
		}
		return m_vcNodes[index];
	}

	// 添加到节点标识管理
	bool AddToNodeMark(T *Node)									
	{
		return (m_mpMarkNodes.insert(std::pair<std::string, T*>(Node->GetMarkID(), Node))).second;
	}

	// 从节点标识管理移除
	void RemoveFromNodeMark(std::string &Key)					
	{
		m_mpMarkNodes.erase(Key);
	}
	
public:
	// 有序list是否有效
	bool IsSortedEnumValid()const								
	{
		return (int)m_mpMarkNodes.size() == (int)m_ltNodeUsed.size();
	}
	
	// 返回有序list首，必须先调用
	T* GetFirst()												
	{
		m_iterSortedEnum = m_ltNodeUsed.begin();
		if (m_iterSortedEnum == m_ltNodeUsed.end())
		{
			return (T*)NULL;
		}
		return GetUsedNode(*m_iterSortedEnum);
	}
	
	// 返回有序list下一个, 必须先调用GetFirst
	T* GetNext()												
	{
		if ((int)m_ltNodeUsed.size() < 1 || 
			m_iterSortedEnum == m_ltNodeUsed.end())
		{
			return (T*)NULL;
		}
		
		m_iterSortedEnum++;
		if (m_iterSortedEnum == m_ltNodeUsed.end())
		{
			return (T*)NULL;
		}
		return GetUsedNode(*m_iterSortedEnum);
	}

public:
	// 获取指定链接
	T* GetUsedNode(const unsigned int index)					
	{
		int iCount = (int)m_vcNodes.size();
		
		if (iCount < 1 || iCount <= (int)index
			|| (int)index < 0)
		{
			return NULL;
		}

		return m_vcNodes[index];
	}

	// 获取使用的节点数
	const int GetUsedNodeNum()const				
	{
		return m_ltNodeUsed.size();
	}

	// 查找插入节点类型及索引
	NodeAddResType ConfigNewNodeRes(int &NodeIndex)
	{
		NodeAddResType iRes = NA_FULL;
		int iFindID = FindMinUnUsedID();
		if (iFindID < 0 || m_vcNodes.size() >= m_iMaxNode)
		{
			iRes = NA_FULL;
			NodeIndex = -1;
			return iRes;
		}

		T *pNode = NULL;
		pNode = GetUsedNode(iFindID);
		if (NULL == pNode)
		{
			// 节点尚未分配
			iRes = NA_NEW;
		}
		else
		{
			// 已经分配的节点复用
			iRes = NA_REUSE;
		}

		NodeIndex = iFindID;
		return iRes;
	}

	// 添加新的节点
	const int AddNewNode(T &item, const int NodeIndex)						
	{
		T *pNode = GetUsedNode(NodeIndex);
		if (NULL == pNode)
		{
			// 节点尚未分配
			pNode = GetIndexNode(NodeIndex);
			pNode->ResetNode(item, NodeIndex);
			m_vcNodes.push_back(pNode);
		}
		else 
		{
			// 已经分配的节点复用
			pNode->ResetNode(item, NodeIndex);
		}

		// 节点设置
		AddToNodeMark(pNode);
		AddUsedID(NodeIndex);
		RemoveUnUsedID(NodeIndex);
		
		return NodeIndex;
	}

	// 移除无效的节点
	void RemoveInvalidNode(const unsigned int index)
	{
		T *pLinkNode = GetUsedNode(index);
		if (NULL != pLinkNode)
		{
			std::string NodeKey = pLinkNode->GetMarkID();
			RemoveFromNodeMark(NodeKey);
			pLinkNode->Clear();
			RemoveUsedID(index);
			AddUnUsedID(index);
		}
	}

	// 从节点标识管理查找
	T* FindFromNodeMark(std::string &Key)
	{
		if (m_mpMarkNodes.size() < 1)
		{
			return NULL;
		}

		typename std::map<std::string, T*>::const_iterator findInterator = m_mpMarkNodes.find(Key);
		if (findInterator != m_mpMarkNodes.end())
		{
			return findInterator->second;
		}
		return NULL;
	}
	
	// 释放资源
	void ReleaseResource()										
	{
		m_ltNodeUsed.clear();

		// 释放动态分配的T节点
		for (unsigned int i = 0; i < (unsigned int)m_vcNodes.size(); i++)
		{
			if (NULL != m_vcNodes[i])
			{
				m_vcNodes[i]->ExternalRelease();
				delete m_vcNodes[i];
				m_vcNodes[i] = NULL;
			}
		}
		
		m_vcNodes.clear();
		m_mpMarkNodes.clear();
	}
};

#endif	/* _INCLUDE_DYNA_NODE_MANAGE_H */

