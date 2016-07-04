#ifndef _INCLUDE_CONTAINER_CMP_H
#define _INCLUDE_CONTAINER_CMP_H

#include <algorithm> 
#include <functional>
#include <string> 

template<typename T>
class CContainCmp : public std::unary_function <T, bool> 
{ 
public:
	std::string   m_strMark; 
	CContainCmp(const std::string &s) 
		: m_strMark(s)
	{
	} 
	    
	bool operator ()(T*  e)
	{
		if (NULL == e)
		{
			return false;
		}
		return  e->GetMarkID() == m_strMark;
	}
}; 

class CIntPred
{
private:
	const int m_iMark;

public:
	CIntPred(const int mark)
		: m_iMark(mark)
	{
	}

	bool operator ()(int e)
	{
		return  e == m_iMark;
	}
};

struct NodeMarkCmpFun
{
	bool operator ()(const char *str1, const char *str2) const
	{
		return strcmp(str1, str2) < 0;
	}
};

struct MutiKeyNodeMark
{
	char *MainKey;
	unsigned char SubKey;

	MutiKeyNodeMark(const char *sMainKey, const unsigned char cSubKey)
	{
		MainKey = (char*)sMainKey;
		SubKey = cSubKey;
	}

	MutiKeyNodeMark& operator = (const MutiKeyNodeMark &item)
	{
		MainKey = item.MainKey;
		SubKey = item.SubKey;
		return *this;
	}
};

struct MutiKeyNodeMarkCmp
{
	bool operator ()(const MutiKeyNodeMark &Left, const MutiKeyNodeMark &Right) const
	{
		if (NULL == Left.MainKey || NULL == Right.MainKey)
		{
			return false;
		}

		int iRes = strcmp(Left.MainKey, Right.MainKey);
		if (0 == iRes)
		{
			return Left.SubKey < Right.SubKey;
		}
		return iRes < 0;
	}
};

#endif	/* _INCLUDE_CONTAINER_CMP_H */
