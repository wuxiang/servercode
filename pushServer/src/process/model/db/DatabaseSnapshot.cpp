#include "DatabaseSnapshot.h"
#include "CppODBC.h"
#include "../../../util/util.h"
#include "../../../util/log/log.h"
#include "../../../util/string/string_util.h"
#include "../../../controller/runtime.h"
#include "../../config.h"
#include "../pushuser/PushUser.h"
#include "../history/RespMsg.h"
#include "../ewarning/EarlyWarning.h"
#include <iostream>
#include <algorithm>

using namespace std;

static const char sPushUserDbName[] = "user_v0200";
static const char sPushUserBasicDbName[] = "user_basic_info_v0200";
static const char sPushUserEWarnDbName[] = "ewarning_v0200";
static const char sPushHisDbName[] = "pushmsghis_v0200";

DatabaseSnapshot *DatabaseSnapshot::m_sMainPointer = NULL;
DatabaseSnapshot *DatabaseSnapshot::m_sSubPointer = NULL;

DatabaseSnapshot::DatabaseSnapshot(const char*dsn, 
	const char*name, const char*pwd)
	: m_pCppODBC(new CppODBC())
{
	StrCopy(m_dsn, dsn, SHOT_LINK_LEN);
	StrCopy(m_uname, name, SHOT_LINK_LEN);
	StrCopy(m_upass, pwd, SHOT_LINK_LEN);
	
	InitialRes();
}

DatabaseSnapshot::~DatabaseSnapshot()
{
	if (NULL != m_pCppODBC)
	{
		m_pCppODBC->Close();
		delete m_pCppODBC;
		m_pCppODBC = NULL;
	}
}

// 初始化资源
void
DatabaseSnapshot::InitialRes()
{
	if (NULL != m_pCppODBC)
	{
	    m_pCppODBC->Initialize();
	    if(!m_pCppODBC->Open())
	    {
	    	ERROR("Failure - Open PublicNews EnvirHandle ODBC \n");
	    }
	}
}

bool
DatabaseSnapshot::DBconnect()
{	
	if (NULL != m_pCppODBC)
	{
		if (m_pCppODBC->Connect(m_dsn, m_uname, m_upass))
		{
			m_pCppODBC->SQLQuery("SET names utf8;");
			return true;
		}
	}
	
	ERROR("Connect data base error[%s-%s-%s]", m_dsn, m_uname, m_upass);

    return false;
}

// 断开数据库连接
bool
DatabaseSnapshot::DBdisconnect()
{
	if (NULL != m_pCppODBC)
	{
		return m_pCppODBC->DisConnect();
	}
	
    return false;
}

// 执行sql语句
const int 
DatabaseSnapshot::SqlExeOnly(const char *sql)
{
	int iRet = 0;
	iRet = m_pCppODBC->SQLExec(sql);
	if( ( iRet <= 0 ) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
	{
		DBdisconnect();
		if (!DBconnect())
		{
			return -1;
		}
		
		iRet = m_pCppODBC->SQLExec(sql);
	}
	
	if( ( iRet <= 0 ) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
	{
		return -2;
	}
	
	return iRet;
}

// 执行sql查询语句
const int 
DatabaseSnapshot::SqlExeQuery(const char *sql)
{
	int iRet = 0;
	iRet = m_pCppODBC->SQLQuery(sql);
	if( ( iRet <= 0 ) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
	{
		DBdisconnect();
		if (!DBconnect())
		{
			return -1;
		}
		
		iRet = m_pCppODBC->SQLQuery(sql);
	}
	
	if( ( iRet <= 0 ) && ( (unsigned int)-1 == m_pCppODBC->GetError() ) )
	{
		return -2;
	}
	
	return iRet;
}

// 添加新用户记录
const int 
DatabaseSnapshot::InsertNewUser(const CPushUser *PushUser, const char *PushToken)
{
	const unsigned short MaxBufLen = 2048;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"INSERT INTO %s (UserID, UserName, UserPwd, LocalVersion, PlatformCode, \
		TelNum, PushServId, UserProperty, RegDate, LastestActiveTime, \
		RecordSetMarkID, PushWarningTotal, PushWarningNum, PushWarningDBNum, RebackWarningNum, \
		InfoMineTotal, InfoMineNum, InfoMineDBNum, \
		PublicNoticeTotal, PublicNoticeNum, PublicNoticeDBNum, PushToken) \
		VALUE( '%s', '%s', '%s', %d, %u, \
		'%s', %d, %u, from_unixtime(%lu), now(), \
		%u, %u, %u, %u, %u, \
		%u, %u, %u, \
		%u, %u, %u, '%s' );",
		sPushUserDbName,
		PushUser->m_strUserID, PushUser->m_strUserName, PushUser->m_strUserPwd, PushUser->m_uLocalVersion, PushUser->m_cPlatformCode,
		PushUser->m_strTelNum, PushUser->m_nPushServId, PushUser->m_iUserProperty, (unsigned long)PushUser->m_uRegTime, 
		PushUser->m_uRecordSetMarkID, PushUser->m_iPushWarningTotal, PushUser->m_iPushWarningNum, PushUser->m_iPushWarningDBNum, PushUser->m_iRebackWarningNum,
		PushUser->m_iInfoMineTotal, PushUser->m_iInfoMineNum, PushUser->m_iInfoMineDBNum,
		PushUser->m_iPublicNoticeTotal, PushUser->m_iPublicNoticeNum, PushUser->m_iPublicNoticeDBNum, PushToken);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("InsertNewUser faild[%s]", sql);
	}
	
	return iReturn;
}

// 更新用户
const int 
DatabaseSnapshot::UpdateUser(const CPushUser *PushUser, const char *PushToken)
{
	const unsigned short MaxBufLen = 2048;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"UPDATE %s SET UserName = '%s', UserPwd = '%s', LocalVersion = %d, \
		TelNum = '%s', PushServId = %d, UserProperty = %u, RegDate = from_unixtime(%lu), \
		RecordSetMarkID = %u, PushWarningTotal = %u, PushWarningNum = %u, PushWarningDBNum = %u, RebackWarningNum = %u, \
		InfoMineTotal = %u, InfoMineNum = %u, InfoMineDBNum = %u, \
		PublicNoticeTotal = %u, PublicNoticeNum = %u, PublicNoticeDBNum = %u, PushToken = '%s' \
		where PlatformCode = %u and UserID = '%s';",
		sPushUserDbName,
		PushUser->m_strUserName, PushUser->m_strUserPwd, PushUser->m_uLocalVersion,
		PushUser->m_strTelNum, PushUser->m_nPushServId, PushUser->m_iUserProperty, (unsigned long)PushUser->m_uRegTime, 
		PushUser->m_uRecordSetMarkID, PushUser->m_iPushWarningTotal, PushUser->m_iPushWarningNum, PushUser->m_iPushWarningDBNum, PushUser->m_iRebackWarningNum,
		PushUser->m_iInfoMineTotal, PushUser->m_iInfoMineNum, PushUser->m_iInfoMineDBNum,
		PushUser->m_iPublicNoticeTotal, PushUser->m_iPublicNoticeNum, PushUser->m_iPublicNoticeDBNum, PushToken,
		PushUser->m_cPlatformCode, PushUser->m_strUserID);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("UpdateUser faild[%s]", sql);
	}
	
	return iReturn;
}

// 删除用户
const int 
DatabaseSnapshot::DeleteUser(const CPushUser *PushUser)
{
	const unsigned short MaxBufLen = 128;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"delete from %s WHERE UserID = '%s' and PlatformCode = %u;",
		sPushUserDbName, PushUser->m_strUserID, PushUser->m_cPlatformCode);
		
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("DeleteUser faild[%s]", sql);
	}
	
	return iReturn;
}

// 查找指定ID的用户
const int
DatabaseSnapshot::FindUser(const char *UserID, const unsigned char PlatformCode,
	CPushUser *PushUser, char *token, const unsigned int BufLen)
{
	const unsigned short MaxBufLen = 2048;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"SELECT UserName, UserPwd, LocalVersion, \
		TelNum, PushServId, UserProperty, UNIX_TIMESTAMP(RegDate), UNIX_TIMESTAMP(LastestActiveTime), \
		PushWarningTotal, PushWarningNum, PushWarningDBNum, RebackWarningNum, \
		InfoMineTotal, InfoMineNum, InfoMineDBNum, \
		PublicNoticeTotal, PublicNoticeNum, PublicNoticeDBNum, PushToken \
		From %s where PlatformCode = %u and UserID = '%s';",
		sPushUserDbName, PlatformCode, UserID);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeQuery(sql);
	if (iReturn < 0)
	{
		DEBUG("FindUser faild[%s]", sql);
	}
	else if (iReturn > 0)
	{
		if (iReturn > 1)
		{
			WARN("Record not unique[%s:%u]", UserID, PlatformCode);
		}
		int column = 0;
		
		StrCopy(PushUser->m_strUserID, UserID, MAX_USER_ID_LEN);
		PushUser->m_cPlatformCode = PlatformCode;
		StrCopy(PushUser->m_strUserName, m_pCppODBC->GetStrValue(column++), MAX_USER_NAME_LEN);
		StrCopy(PushUser->m_strUserPwd, m_pCppODBC->GetStrValue(column++), MAX_USER_PWD_LEN);
		PushUser->m_uLocalVersion = (unsigned short)(atoi(m_pCppODBC->GetStrValue(column++)));
		
		StrCopy(PushUser->m_strTelNum, m_pCppODBC->GetStrValue(column++), MAX_TEL_ID_LEN);
		PushUser->m_nPushServId = m_pCppODBC->GetIntValue(column++);
		PushUser->m_iUserProperty = m_pCppODBC->GetIntValue(column++);
		PushUser->m_uRegTime = m_pCppODBC->GetTimetValue(column++);
		PushUser->m_tLastestAliveTime = m_pCppODBC->GetTimetValue(column++);
		
		// 重新加载重新分配
		PushUser->m_uRecordSetMarkID = (unsigned int)-1;
		PushUser->m_iPushWarningTotal = m_pCppODBC->GetUIntValue(column++);    
		PushUser->m_iPushWarningNum = m_pCppODBC->GetUIntValue(column++);		
		PushUser->m_iPushWarningDBNum = m_pCppODBC->GetUIntValue(column++);    
		PushUser->m_iRebackWarningNum = m_pCppODBC->GetUIntValue(column++);          
		
		PushUser->m_iInfoMineTotal = m_pCppODBC->GetUIntValue(column++);
		PushUser->m_iInfoMineNum = m_pCppODBC->GetUIntValue(column++);	 
		PushUser->m_iInfoMineDBNum = m_pCppODBC->GetUIntValue(column++);
		
		PushUser->m_iPublicNoticeTotal = m_pCppODBC->GetUIntValue(column++);
		PushUser->m_iPublicNoticeNum = m_pCppODBC->GetUIntValue(column++);	 
		PushUser->m_iPublicNoticeDBNum = m_pCppODBC->GetUIntValue(column++);
		if (NULL != token)
		{
			StrCopy(token, m_pCppODBC->GetStrValue(column++), BufLen);
		}
	}
	
	return iReturn;
}

// 添加用户基本信息
const int
DatabaseSnapshot::InsertUserBasicInfo(const CUserBaseInfoHead *BasicInfo, 
	const unsigned short ServerID, const unsigned char PlatformCode,
	const unsigned int InvalidUserCount, const unsigned int InvalidWarnCount)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"INSERT INTO %s (PushServId, PlatformCode, DBSynTime, MaxUserSpace, OccupyNum, \
		HisSpace, EarlyWarningSpace, UseEarlyWarning, EarlyWarningPerUser, LastestActiveTime) \
		VALUE( %u, %u, from_unixtime(%lu), %u, %u, \
		%u, %u, %u, %u, now());",
		sPushUserBasicDbName,
		ServerID, PlatformCode, (unsigned long)BasicInfo->m_tDBSynTime, BasicInfo->m_nSpace, BasicInfo->m_nOccupyNum, 
		BasicInfo->m_nHisSpace, BasicInfo->m_nEarlyWarningSpace, BasicInfo->m_nUseEarlyWarning, BasicInfo->m_nEarlyWarningPerUser);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("InsertUserBasicInfo faild[%s]", sql);
	}
	
	return iReturn;
}

// 查找用户基本信息
const int
DatabaseSnapshot::FindUserBasicInfo(const unsigned short ServerID, const unsigned char PlatformCode, 
	CUserBaseInfoHead &BasicInfo)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"SELECT UNIX_TIMESTAMP(DBSynTime), MaxUserSpace, OccupyNum, HisSpace, \
		EarlyWarningSpace, UseEarlyWarning, EarlyWarningPerUser \
		From %s where PushServId = %u PlatformCode = %u order by MapID desc limit 1;",
		sPushUserBasicDbName, ServerID, PlatformCode);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeQuery(sql);
	if (iReturn < 0)
	{
		DEBUG("FindUserBasicInfo faild[%s]", sql);
	}
	else if (iReturn > 0)
	{
		int column = 0;
		
		BasicInfo.m_tDBSynTime = m_pCppODBC->GetTimetValue(column++);
		BasicInfo.m_nSpace = m_pCppODBC->GetUIntValue(column++);
		BasicInfo.m_nOccupyNum = m_pCppODBC->GetUIntValue(column++);
		BasicInfo.m_nHisSpace = m_pCppODBC->GetUIntValue(column++);
		BasicInfo.m_nEarlyWarningSpace = m_pCppODBC->GetUIntValue(column++);
		BasicInfo.m_nUseEarlyWarning = m_pCppODBC->GetUIntValue(column++);
		BasicInfo.m_nEarlyWarningPerUser = m_pCppODBC->GetUIntValue(column++);
	}
	
	return iReturn;
}

// 添加用户的预警记录
const int
DatabaseSnapshot::InsertUserEWarning(const char *UserID, const unsigned char PlatformCode,
	const unsigned int RecordID, CEarlyWarningConditionRecord *Record)
{
	const unsigned short MaxBufLen = 2048;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"INSERT INTO %s (UserID, PlatformCode, UniqueID, RecordProperty, StkIndex, StockCode, \
		PriceGreaterTrigger, PriceGreaterProperty, PriceSmallerTrigger, PriceSmallerProperty, IncreaseGreaterTrigger, \
		IncreaseGreaterProperty, DecreaseGreaterTrigger, DecreaseGreaterProperty, ExchangeGreaterTrigger, ExchangeGreaterProperty, \
		LatestInfoMineCrc, TriggerTime, LastestActiveTime) \
		VALUE( '%s', %u, %u, %u, %u, '%s', \
		%u, %u, %u, %u, %u, \
		%u, %u, %u, %u, %u, \
		%u, from_unixtime(%lu), now());",
		sPushUserEWarnDbName,
		UserID, PlatformCode, RecordID, Record->m_cRecordProperty, Record->m_nStkIndex, Record->GetStkCode(),
		Record->GetCondition(PriceGreater)->GetTriggerValue(), Record->GetCondition(PriceGreater)->GetProperty(),
		Record->GetCondition(PriceSmaller)->GetTriggerValue(), Record->GetCondition(PriceSmaller)->GetProperty(),
		Record->GetCondition(IncreaseGreater)->GetTriggerValue(), Record->GetCondition(IncreaseGreater)->GetProperty(),
		Record->GetCondition(DecreaseGreater)->GetTriggerValue(), Record->GetCondition(DecreaseGreater)->GetProperty(),
		Record->GetCondition(ExchangeGreater)->GetTriggerValue(), Record->GetCondition(ExchangeGreater)->GetProperty(),
		Record->GetLatestInfoMineCrc(), (unsigned long)Record->GetBeginCmpTime()
		);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("InsertUserEWarning faild[%s]", sql);
	}
	
	return iReturn;
}

// 更新用户的预警记录
const int 
DatabaseSnapshot::UpdateUserEWarning(const char *UserID, const unsigned char PlatformCode,
	const unsigned int RecordID, CEarlyWarningConditionRecord *Record)
{
	const unsigned short MaxBufLen = 2048;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"UPDATE %s SET RecordProperty = %u, StkIndex = %u, StockCode = '%s', \
		PriceGreaterTrigger = %u, PriceGreaterProperty = %u, PriceSmallerTrigger = %u, PriceSmallerProperty = %u, IncreaseGreaterTrigger = %u, \
		IncreaseGreaterProperty = %u, DecreaseGreaterTrigger = %u, DecreaseGreaterProperty = %u, ExchangeGreaterTrigger = %u, ExchangeGreaterProperty = %u, \
		LatestInfoMineCrc = %u, TriggerTime = from_unixtime(%lu) \
		where PlatformCode  = %u and UserID = '%s' and UniqueID = %u;",
		sPushUserEWarnDbName,
		Record->m_cRecordProperty, Record->m_nStkIndex, Record->GetStkCode(),
		Record->GetCondition(PriceGreater)->GetTriggerValue(), Record->GetCondition(PriceGreater)->GetProperty(),
		Record->GetCondition(PriceSmaller)->GetTriggerValue(), Record->GetCondition(PriceSmaller)->GetProperty(),
		Record->GetCondition(IncreaseGreater)->GetTriggerValue(), Record->GetCondition(IncreaseGreater)->GetProperty(),
		Record->GetCondition(DecreaseGreater)->GetTriggerValue(), Record->GetCondition(DecreaseGreater)->GetProperty(),
		Record->GetCondition(ExchangeGreater)->GetTriggerValue(), Record->GetCondition(ExchangeGreater)->GetProperty(),
		Record->GetLatestInfoMineCrc(), (unsigned long)Record->GetBeginCmpTime(),
		PlatformCode, UserID, RecordID
		);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("UpdateUserEWarning faild[%s]", sql);
	}
	
	return iReturn;
}

// 删除用户的预警记录
const int
DatabaseSnapshot::DeleteUserEWarning(const char *UserID, const unsigned char PlatformCode,
	const unsigned int RecordID)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"delete from  %s where PlatformCode  = %u and UserID = '%s' and UniqueID = %u;",
		sPushUserEWarnDbName,
		PlatformCode, UserID, RecordID
		);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("DeleteUserEWarning faild[%s]", sql);
	}
	
	return iReturn;
}

// 删除指定用户的所有预警记录
const int
DatabaseSnapshot::DeleteUserAllEWarning(const char *UserID, const unsigned char PlatformCode)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"delete from  %s where PlatformCode  = %u and UserID = '%s';",
		sPushUserEWarnDbName,
		PlatformCode, UserID
		);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("DeleteUserAllEWarning faild[%s]", sql);
	}
	
	return iReturn;
}

// 读取用户的预警记录
const int
DatabaseSnapshot::ReadUserEWarning(const char *UserID, const unsigned char PlatformCode,
	const unsigned int RecordID, CEarlyWarningConditionRecord &Record)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"SELECT RecordProperty, StkIndex, StockCode, \
		PriceGreaterTrigger, PriceGreaterProperty, PriceSmallerTrigger, PriceSmallerProperty, IncreaseGreaterTrigger, \
		IncreaseGreaterProperty, DecreaseGreaterTrigger, DecreaseGreaterProperty, ExchangeGreaterTrigger, ExchangeGreaterProperty, \
		LatestInfoMineCrc, UNIX_TIMESTAMP(TriggerTime) \
		From %s where PlatformCode  = %u and UserID = '%s' and UniqueID = %u;",
		sPushUserEWarnDbName, PlatformCode, UserID, RecordID);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeQuery(sql);
	if (iReturn < 0)
	{
		DEBUG("ReadUserEWarning faild[%s]", sql);
	}
	else if (iReturn > 0)
	{
		int column = 0;
		
		Record.m_cRecordProperty = m_pCppODBC->GetUCharValue(column++);
		Record.m_nStkIndex = m_pCppODBC->GetUIntValue(column++);
		Record.m_nStkIndex = 0xFFFF;
		StrCopy(Record.m_strStkCode, m_pCppODBC->GetStrValue(column++), MAX_STKCODE_LEN);
		Record.GetCondition(PriceGreater)->m_uTriggerValue = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(PriceGreater)->m_sProperty = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(PriceGreater)->m_eCompareType = GREATER_AND_EQUAL;
		
		Record.GetCondition(PriceSmaller)->m_uTriggerValue = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(PriceSmaller)->m_sProperty = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(PriceSmaller)->m_eCompareType = SMALLER_AND_EQUAL;
		
		Record.GetCondition(IncreaseGreater)->m_uTriggerValue = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(IncreaseGreater)->m_sProperty = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(IncreaseGreater)->m_eCompareType = GREATER_AND_EQUAL;
		
		Record.GetCondition(DecreaseGreater)->m_uTriggerValue = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(DecreaseGreater)->m_sProperty = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(DecreaseGreater)->m_eCompareType = GREATER_AND_EQUAL;
		
		Record.GetCondition(ExchangeGreater)->m_uTriggerValue = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(ExchangeGreater)->m_sProperty = m_pCppODBC->GetUIntValue(column++);
		Record.GetCondition(ExchangeGreater)->m_eCompareType = GREATER_AND_EQUAL;
		
		Record.UpdateSetLatestInfoMineCrc(m_pCppODBC->GetUIntValue(column++));
		Record.m_nBeginCmpTime = m_pCppODBC->GetTimetValue(column++);
	}
	
	return iReturn;
}

// 查找用户的预警记录
const int
DatabaseSnapshot::FindUserEWarning(const char *UserID, const unsigned char PlatformCode,
	const unsigned int RecordID)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"SELECT count(*) From %s where PlatformCode  = %u and UserID = '%s' and UniqueID = %u;",
		sPushUserEWarnDbName, PlatformCode, UserID, RecordID);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeQuery(sql);
	if (iReturn < 0)
	{
		DEBUG("ReadUserEWarning faild[%s]", sql);
		return iReturn;
	}
	else if (iReturn > 0)
	{
		int column = 0;
		
		int iMatchCount = m_pCppODBC->GetIntValue(column++);
		return iMatchCount;
	}
	
	return 0;
}

// 添加历史记录
const int
DatabaseSnapshot::InsertHistory(const char *UserID, const unsigned char PlatformCode, 
	const unsigned int MsgID, const PushMsg *Record)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"INSERT INTO %s (UserID, PlatformCode, MsgID, MsgType, MsgSubType, \
		TriggerDateTime, LatestValue, SettingRecordID, StockCode, LastestActiveTime) \
		VALUE( '%s', %u, %u, %u, %u, \
		from_unixtime(%lu), %u, %u, '%s', now());",
		sPushHisDbName,
		UserID, PlatformCode, MsgID, Record->MsgType, Record->MsgSubType,
		(unsigned long)Record->DateTime, Record->LatestValue, Record->SettingRecordID, Record->strStkCode
		);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("InsertHistory faild[%s]", sql);
	}
	
	return iReturn;
}

// 删除小于指定ID历史记录
const int
DatabaseSnapshot::DeleteUserHistroySmallerThanID(const char *UserID, const unsigned char PlatformCode,
	const unsigned int MsgID, const unsigned char MsgType)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"delete from  %s where PlatformCode  = %u and UserID = '%s' and MsgType = %u and MsgID < %u;",
		sPushHisDbName,
		PlatformCode, UserID, MsgType, MsgID
		);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("DeleteUserHistroySmallerThanID faild[%s]", sql);
	}
	
	return iReturn;
}

// 读取历史记录
const int
DatabaseSnapshot::ReadHistory(const char *UserID, const unsigned char PlatformCode,
	const unsigned int MsgID, const unsigned char MsgType, PushMsg &Record)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"SELECT MsgSubType, UNIX_TIMESTAMP(TriggerDateTime), LatestValue, SettingRecordID, StockCode \
		From %s where PlatformCode  = %u and UserID = '%s' and MsgID = %u and MsgType = %u;",
		sPushHisDbName, 
		PlatformCode, UserID, MsgID, MsgType);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeQuery(sql);
	if (iReturn < 0)
	{
		DEBUG("ReadHistory faild[%s]", sql);
	}
	else if (iReturn > 0)
	{
		int column = 0;
		
		Record.MsgType = MsgType;
		Record.MsgSubType = m_pCppODBC->GetUCharValue(column++);
		Record.DateTime =  m_pCppODBC->GetTimetValue(column++);
		Record.LatestValue = m_pCppODBC->GetUIntValue(column++);
		Record.SettingRecordID = m_pCppODBC->GetUIntValue(column++);
		StrCopy(Record.strStkCode, m_pCppODBC->GetStrValue(column++), MAX_STKCODE_LEN);
	}
	
	return iReturn;
}

// 更新用户的自选股设置
const int 
DatabaseSnapshot::UpdateUserSelfStock(const char *UserID, const unsigned char PlatformCode,
	const char *StkCodeList)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"update %s set SelfSelectStk = '%s' where PlatformCode  = %u and UserID = '%s';",
		sPushUserDbName, StkCodeList, PlatformCode, UserID
		);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeOnly(sql);
	if (iReturn < 0)
	{
		DEBUG("UpdateUserSelfStock faild[%s]", sql);
	}
	
	return iReturn;
}

// 读取用户的自选股设置
const int
DatabaseSnapshot::ReadUserSelfStock(const char *UserID, const unsigned char PlatformCode,
	char *OutBuf, const unsigned int OutBufSize)
{
	const unsigned short MaxBufLen = 1024;
	char sql[MaxBufLen] = {0};
	int iReturn = snprintf(sql, MaxBufLen, 
		"SELECT SelfSelectStk From %s where PlatformCode = %u and UserID = '%s';",
		sPushUserDbName, PlatformCode, UserID);
		
	if (iReturn > MaxBufLen)
	{
		DEBUG("MaxBufLen is small");
		return -1;
	}
	
	iReturn = SqlExeQuery(sql);
	if (iReturn < 0)
	{
		DEBUG("ReadUserSelfStock faild[%s]", sql);
	}
	else if (iReturn > 0)
	{
		int column = 0;
		
		StrCopy(OutBuf, m_pCppODBC->GetStrValue(column++), OutBufSize);
	}
	
	return iReturn;
}

DatabaseSnapshot* 
DatabaseSnapshot::GetMain()
{
	if (NULL == m_sMainPointer)
	{
		const MySqlDbConfig *pDbConfig = GetDbSetting();
		m_sMainPointer = new DatabaseSnapshot(pDbConfig->DbName, pDbConfig->LogUser, pDbConfig->LogPwd);
	}
	
	return m_sMainPointer;
}

DatabaseSnapshot*
DatabaseSnapshot::GetSub()
{
	if (NULL == m_sSubPointer)
	{
		const MySqlDbConfig *pDbConfig = GetDbSetting();
		m_sSubPointer = new DatabaseSnapshot(pDbConfig->DbName, pDbConfig->LogUser, pDbConfig->LogPwd);
	}
	
	return m_sSubPointer;
}

void 
DatabaseSnapshot::Release()
{
	if (NULL != m_sMainPointer)
	{
		delete m_sMainPointer;
		m_sMainPointer = NULL;
	}
	
	if (NULL != m_sSubPointer)
	{
		delete m_sSubPointer;
		m_sSubPointer = NULL;
	}
}

