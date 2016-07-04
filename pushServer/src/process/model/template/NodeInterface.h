#ifndef _INCLUDE_DYNA_NODE_INTERFACE_H
#define _INCLUDE_DYNA_NODE_INTERFACE_H

#include <string>

template<typename T>
class CNodeInterface
{
public:
	CNodeInterface(){}
	virtual ~CNodeInterface(){}

	virtual void ResetNode(const T &, const int) = 0;
	virtual void Clear() = 0;
	virtual const std::string GetMarkID()const = 0;
	virtual void SetMarkID(std::string &key) = 0;
	virtual bool IsDead() = 0;
	virtual unsigned int GetNodeIndex() = 0;
	virtual void ExternalRelease() = 0;
};

#endif	/* _INCLUDE_DYNA_NODE_INTERFACE_H */

