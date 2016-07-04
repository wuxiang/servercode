#ifndef _OUTER_SEND_MARK_H
#define _OUTER_SEND_MARK_H

class COuterSendMark
{
private:
	// 平台标识
	unsigned char m_cPlatFormCode;
	// 用户标识
	unsigned int m_uUserMapID;
	// 消息种类
	unsigned char m_cMsgType;

public:
	COuterSendMark(const unsigned char PlatFormCode, const unsigned int UserMapID, 
			unsigned char cMsgType)
		: m_cPlatFormCode(PlatFormCode),
		m_uUserMapID(UserMapID),
		m_cMsgType(cMsgType)
	{
				
	}
	
	~COuterSendMark()
	{
	}
	
	COuterSendMark& operator = (const COuterSendMark& item)
	{
		m_cPlatFormCode = item.m_cPlatFormCode;
		m_uUserMapID = item.m_uUserMapID;
		m_cMsgType = item.m_cMsgType;
		return *this;
	}

	const unsigned char GetPlatFormCode()const
	{
		return m_cPlatFormCode;
	}
	
	const unsigned int GetUserMapID()const
	{
		return m_uUserMapID;
	}
	
	const unsigned char GetMsgType()const
	{
		return m_cMsgType;
	}
};

#endif  /* _OUTER_SEND_MARK_H */
