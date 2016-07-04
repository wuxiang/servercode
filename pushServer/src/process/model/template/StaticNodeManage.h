#ifndef _INCLUDE_STATIC_NODE_MANAGE_H
#define _INCLUDE_STATIC_NODE_MANAGE_H

#include "ContainerCmp.h"
#include <vector>
#include <string>
#include <map>

// 状态
enum StaticNodeManageState
{
	// 正常状态
	SNMS_NORMAL = 0,
	// 饱和状态
	SNMS_FULL,
	// 移位整理
	SNMS_ARRANGE,
	// 异常状态
	SNMS_ERROR
};

template<typename T>
class CStaticNodeManage
{
public:
	CStaticNodeManage()
		: m_pMemCache(NULL),
		m_uNodeSize(0),
		m_uMaxNodeNum(0),
		m_uCurrentAddPos(0),
		m_uEndPos(0),
		m_uNodeStep(1),
		m_uStatus(SNMS_ERROR),
		m_iAttachedPos(-1)
	{
	}

	virtual ~CStaticNodeManage()
	{
		m_ResultConclude.clear();
		m_mpMarkNodes.clear();
	}

private:
	// 需要管理的节点内存起始地址
	void *m_pMemCache;
	// 单个节点的空间大小
	unsigned short m_uNodeSize;
	// 最大节点数目
	int m_uMaxNodeNum;
	// 当前添加索引
	int m_uCurrentAddPos;
	// 结尾位置
	int m_uEndPos;
	// 节点管理步长
	int m_uNodeStep;
	// 当前节点的状态
	StaticNodeManageState m_uStatus;
	// 管理结果统计
	std::vector<int> m_ResultConclude;
	// 无效节点扫描结果统计
	typename std::vector<int>::iterator m_iterInvalidEnum;
	// 节点关系映射
	std::map<MutiKeyNodeMark, const unsigned int, MutiKeyNodeMarkCmp> m_mpMarkNodes;

protected:
	// 相对于主节点的附属位置
	int m_iAttachedPos;

private:
	// 更新状态
	void UpdateStatus()
	{
		if (m_uCurrentAddPos < m_uMaxNodeNum)
		{
			m_uStatus = SNMS_NORMAL;
		}
		else if (m_uCurrentAddPos == m_uMaxNodeNum)
		{
			if (m_ResultConclude.size() > 0)
			{
				m_uStatus = SNMS_NORMAL;
			}
			else
			{
				m_uStatus = SNMS_FULL;
			}
		}
		else
		{
			m_uStatus = SNMS_ERROR;
		}
	}

	// 获取当前插入新节点位置索引
	const int GetCurrentAddPos()
	{
		size_t uCount = m_ResultConclude.size();
		int iAddPos = -1;
		
		// 有已分配的空闲节点
		if (uCount > 0)
		{
			std::vector<int>::iterator iter = m_ResultConclude.begin();
			iAddPos = *iter;
			m_ResultConclude.erase(iter);
		}
		// 无空闲节点
		else
		{
			iAddPos = m_uEndPos;
		}
		return iAddPos;
	}

	// 更新下一个插入点的位置
	void UpdateNextAddPos(const int PreAddPos)
	{
		if (PreAddPos == m_uEndPos)
		{
			m_uEndPos++;
		}
	}

	// 扫描无效节点
	bool ScanInvalidNode()
	{
		int uScanIndex = 0;
		int uCount = GetNodeCount();
		if (SNMS_ERROR == m_uStatus)
		{
			return false;
		}

		m_ResultConclude.clear();
		for (uScanIndex = 0; uScanIndex < uCount; uScanIndex++)
		{
			if (IsKeepValid(uScanIndex))
			{
				continue;
			}

			m_ResultConclude.push_back(uScanIndex);
		}

		return true;
	}

// 需要实现的接口函数
protected:
	// 更新设置新节点
	virtual const int UpdateSetNode(const int HeadIndex, const T &Node) = 0;
	// 节点(组)是否有效
	virtual bool IsKeepValid(const int HeadIndex) = 0;
	// 执行整理无效节点(组)
	virtual void ProcessArrangeInvalidNode() = 0;

protected:
	// 构建
	const int Construct(void *source, const unsigned short nodesize, const int maxnum,
		const int InitialMemCount, const int Step)
	{
		if (NULL == source)
		{
			m_uStatus = SNMS_ERROR;
			return -1;
		}

		if (nodesize <= 0 || maxnum <= 0)
		{
			m_uStatus = SNMS_ERROR;
			return -2;
		}

		m_pMemCache = source;
		m_uNodeSize = nodesize;
		m_uMaxNodeNum = maxnum;
		m_uNodeStep = Step;
		
		if (InitialMemCount < 0 || InitialMemCount > m_uMaxNodeNum)
		{
			m_uStatus = SNMS_ERROR;
			return -3;
		}

		m_ResultConclude.clear();
		m_ResultConclude.reserve(m_uMaxNodeNum);

		// 插入新节点位置
		m_uEndPos = InitialMemCount;
		m_uCurrentAddPos = m_uEndPos;
		UpdateStatus();

		return 0;
	}

	// 获取扫描无效节点结果首
	const int GetInvalidFirst()
	{
		m_iterInvalidEnum = m_ResultConclude.begin();
		if (m_iterInvalidEnum == m_ResultConclude.end())
		{
			return -1;
		}
		return (*m_iterInvalidEnum);
	}

	// 获取扫描无效节点结果Next
	const int GetInvalidNext()
	{
		if (m_ResultConclude.size() < 1 || 
			m_iterInvalidEnum == m_ResultConclude.end())
		{
			return -1;
		}

		m_iterInvalidEnum++;
		if (m_iterInvalidEnum == m_ResultConclude.end())
		{
			return -1;
		}
		return (*m_iterInvalidEnum);
	}
	
	// 添加到节点标识管理
	bool AddToNodeMark(const char *NodeKey, const unsigned char SubKey, const unsigned int NodeIndex)
	{
		return (m_mpMarkNodes.insert(std::pair<MutiKeyNodeMark, const unsigned int>(MutiKeyNodeMark(NodeKey, SubKey), NodeIndex))).second;
	}

	// 从节点标识管理移除
	void RemoveFromNodeMark(const char *NodeKey, const unsigned char SubKey)					
	{
		m_mpMarkNodes.erase(MutiKeyNodeMark(NodeKey, SubKey));
	}

public:
	// 获取当前占用节点总数
	const int GetNodeCount()const
	{
		return m_uEndPos;
	}

	// 获取管理当前的状态
	enum StaticNodeManageState GetManageStatus()const
	{
		return m_uStatus;
	}

	// 添加节点(返回负值添加失败)
	int AddNewNode(const T &Node, const int AddPos = -1)
	{
		int iRes = 0;
		m_iAttachedPos = -1;
		if (SNMS_NORMAL != m_uStatus)
		{
			return -1;
		}

		// 获取位置
		m_uCurrentAddPos = (-1 == AddPos ? GetCurrentAddPos() : AddPos);
		UpdateStatus();

		if (SNMS_NORMAL != m_uStatus)
		{
			return -2;
		}

		// 更新节点
		iRes = UpdateSetNode(m_uCurrentAddPos, Node);

		// 更新以一次节点位置
		UpdateNextAddPos(m_uCurrentAddPos);

		return iRes == 0 ? m_uCurrentAddPos : iRes;
	}

	// 获取索引节点
	T* GetIndexNode(const size_t NodeIndex)
	{
		if (SNMS_ERROR == m_uStatus)
		{
			return (T*)NULL;
		}

		if (NodeIndex < 0 || NodeIndex >= (size_t)m_uMaxNodeNum)
		{
			return (T*)NULL;
		}

		size_t uCalcNodeIndex = NodeIndex * m_uNodeStep * m_uNodeSize;
		return  (T*)(((char*)m_pMemCache) + uCalcNodeIndex);
	}

	// 整理无效节点
	void ReArrangeInvalidNode()
	{
		if (SNMS_ERROR == m_uStatus)
		{
			return;
		}

		m_uStatus = SNMS_ARRANGE;
		ScanInvalidNode();
		ProcessArrangeInvalidNode();

		UpdateStatus();
	}

	// 清空无效节点列表
	void ClearInvalidNodeList()
	{
		m_ResultConclude.clear();
	}
	
	// 获取相对于主节点的附属位置
	const int GetAttachedPos()const
	{
		return m_iAttachedPos;
	}
	
	// 从节点标识管理查找
	const unsigned int FindFromNodeMark(const char *NodeKey, const unsigned char SubKey)
	{
		if (m_mpMarkNodes.size() < 1)
		{
			return (unsigned int)-1;
		}

		typename std::map<MutiKeyNodeMark, const unsigned int, MutiKeyNodeMarkCmp>::const_iterator findInterator = m_mpMarkNodes.find(MutiKeyNodeMark(NodeKey, SubKey));
		if (findInterator != m_mpMarkNodes.end())
		{
			return findInterator->second;
		}
		return (unsigned int)-1;
	}
	
	// 返回整理后空闲的节点数
	const unsigned int GetCurrentInvalidCount()const
	{
		return m_ResultConclude.size();
	}
};

#endif	/* _INCLUDE_STATIC_NODE_MANAGE_H */
