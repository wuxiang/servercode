#ifndef _INCLUDE_DYNA_NODE_MANAGE_H
#define _INCLUDE_DYNA_NODE_MANAGE_H

/***
**	��̬����ڵ�
**	��Ҫ��֤ͬʱֻ��һ���߳̽���д����
**
***************************************/
#include "ContainerCmp.h"
#include <set>
#include <vector>
#include <string>
#include <map>
#include <utility>

// �ڵ����
enum NodeAddResType
{
	// �ڵ��Ѿ�����
	NA_FULL = -2,
	// ��������
	NA_ERROR,
	// �½��ڵ�
	NA_NEW,
	// �ڵ㸴��
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
	

// ��Ա��������
protected:
	const unsigned int m_iMaxNode;								// ���ڵ���Ŀ
	std::set<int> m_ltNodeUsed;									// �Ѿ�ʹ�õ����ӽڵ�����
	std::set<int> m_ltNotNodeUsed;								// ��δʹ�õ����ӽڵ�����
	std::vector<T*> m_vcNodes;									// �ڵ�洢����
	std::map<std::string, T*> m_mpMarkNodes;					// �ڵ��ϵӳ��
	typename std::set<int>::const_iterator m_iterSortedEnum;	// ������������ͷ

// �ֲ���������
private:
	// ��������ID
	static int SortID(const void * item1, const void * item2)
	{
		if( NULL == item1 || NULL == item2)
		{
			return -1;
		}
		
		return *((int*)item1) - *((int*)item2);
	}
	
	// ��ȡ��δʹ�õ���С�ڵ�ID
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

	// ���ռ�õ�����ID
	int AddUsedID(const int ID)								
	{
		m_ltNodeUsed.insert(ID);
		return 0;
	}
	
	// ��ӿ���ID
	int AddUnUsedID(const int ID)							
	{
		m_ltNotNodeUsed.insert(ID);
		return 0;
	}

	// �Ƴ�ռ�õ�����ID
	void RemoveUsedID(const int ID)							
	{
		typename std::set<int>::iterator iter = 
			m_ltNodeUsed.find(ID);
		
		if (iter != m_ltNodeUsed.end())
		{
			m_ltNodeUsed.erase(iter);
		}
	}

	// �Ƴ�����ID
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
	
	// ��ȡָ��������Node�������ڴ���
	T* GetIndexNode(const unsigned int index) 					
	{
		if (index >= (unsigned int)m_vcNodes.size())
		{
			return new T();
		}
		return m_vcNodes[index];
	}

	// ��ӵ��ڵ��ʶ����
	bool AddToNodeMark(T *Node)									
	{
		return (m_mpMarkNodes.insert(std::pair<std::string, T*>(Node->GetMarkID(), Node))).second;
	}

	// �ӽڵ��ʶ�����Ƴ�
	void RemoveFromNodeMark(std::string &Key)					
	{
		m_mpMarkNodes.erase(Key);
	}
	
public:
	// ����list�Ƿ���Ч
	bool IsSortedEnumValid()const								
	{
		return (int)m_mpMarkNodes.size() == (int)m_ltNodeUsed.size();
	}
	
	// ��������list�ף������ȵ���
	T* GetFirst()												
	{
		m_iterSortedEnum = m_ltNodeUsed.begin();
		if (m_iterSortedEnum == m_ltNodeUsed.end())
		{
			return (T*)NULL;
		}
		return GetUsedNode(*m_iterSortedEnum);
	}
	
	// ��������list��һ��, �����ȵ���GetFirst
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
	// ��ȡָ������
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

	// ��ȡʹ�õĽڵ���
	const int GetUsedNodeNum()const				
	{
		return m_ltNodeUsed.size();
	}

	// ���Ҳ���ڵ����ͼ�����
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
			// �ڵ���δ����
			iRes = NA_NEW;
		}
		else
		{
			// �Ѿ�����Ľڵ㸴��
			iRes = NA_REUSE;
		}

		NodeIndex = iFindID;
		return iRes;
	}

	// ����µĽڵ�
	const int AddNewNode(T &item, const int NodeIndex)						
	{
		T *pNode = GetUsedNode(NodeIndex);
		if (NULL == pNode)
		{
			// �ڵ���δ����
			pNode = GetIndexNode(NodeIndex);
			pNode->ResetNode(item, NodeIndex);
			m_vcNodes.push_back(pNode);
		}
		else 
		{
			// �Ѿ�����Ľڵ㸴��
			pNode->ResetNode(item, NodeIndex);
		}

		// �ڵ�����
		AddToNodeMark(pNode);
		AddUsedID(NodeIndex);
		RemoveUnUsedID(NodeIndex);
		
		return NodeIndex;
	}

	// �Ƴ���Ч�Ľڵ�
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

	// �ӽڵ��ʶ�������
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
	
	// �ͷ���Դ
	void ReleaseResource()										
	{
		m_ltNodeUsed.clear();

		// �ͷŶ�̬�����T�ڵ�
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

