#ifndef DATABASESNAPSHOT_H
#define DATABASESNAPSHOT_H

#include "time.h"
#include <string>

class CppODBC;
class CPushUser;
struct  CUserBaseInfoHead;
struct PushMsg;
class CEarlyWarningCondition;
class CEarlyWarningConditionRecord;
#define SHOT_LINK_LEN 32

class DatabaseSnapshot
{
private:
	// 用于调用odbc函数
	CppODBC * m_pCppODBC;
	// ODBC数据源
	char m_dsn[SHOT_LINK_LEN];
	// 用户名
	char m_uname[SHOT_LINK_LEN];
	// 密码
	char m_upass[SHOT_LINK_LEN];
	// 主
	static DatabaseSnapshot *m_sMainPointer;
	// 副
	static DatabaseSnapshot *m_sSubPointer;

private:
    DatabaseSnapshot(const char*, const char*, const char*);
    ~DatabaseSnapshot();
    // 初始化资源
	void InitialRes();
    // 建立数据库连接
	bool DBconnect();
	// 断开数据库连接
	bool DBdisconnect();
	// 执行sql语句
	const int SqlExeOnly(const char*);
	// 执行sql查询语句
	const int SqlExeQuery(const char*);
	
public:
	// 添加新用户记录
	const int InsertNewUser(const CPushUser*, const char*);
	// 更新用户
	const int UpdateUser(const CPushUser*, const char*);
	// 删除用户
	const int DeleteUser(const CPushUser*);
	// 查找指定ID的用户
	const int FindUser(const char *, const unsigned char, CPushUser *, char*, const unsigned int);

	// 添加用户基本信息
	const int InsertUserBasicInfo(const CUserBaseInfoHead*, const unsigned short, const unsigned char,
				const unsigned int, const unsigned int);
	// 查找用户基本信息
	const int FindUserBasicInfo(const unsigned short, const unsigned char, CUserBaseInfoHead &);
	
	// 添加用户的预警记录
	const int InsertUserEWarning(const char*, const unsigned char, const unsigned int, CEarlyWarningConditionRecord*);
	// 更新用户的预警记录
	const int UpdateUserEWarning(const char*, const unsigned char, const unsigned int, CEarlyWarningConditionRecord*);
	// 删除用户的预警记录
	const int DeleteUserEWarning(const char*, const unsigned char, const unsigned int);
	// 删除指定用户的所有预警记录
	const int DeleteUserAllEWarning(const char*, const unsigned char);
	// 读取用户的预警记录
	const int ReadUserEWarning(const char*, const unsigned char, const unsigned int, CEarlyWarningConditionRecord &);
	// 查找用户的预警记录
	const int FindUserEWarning(const char*, const unsigned char, const unsigned int);
	
	// 添加历史记录
	const int InsertHistory(const char*, const unsigned char, const unsigned int, const PushMsg*);
	// 删除小于指定ID历史记录
	const int DeleteUserHistroySmallerThanID(const char*, const unsigned char, const unsigned int, const unsigned char);
	// 读取历史记录
	const int ReadHistory(const char*, const unsigned char, const unsigned int, const unsigned char, PushMsg&);
	
	// 更新用户的自选股设置
	const int UpdateUserSelfStock(const char*, const unsigned char, const char*);
	// 读取用户的自选股设置
	const int ReadUserSelfStock(const char*, const unsigned char, char *, const unsigned int);
	
	static DatabaseSnapshot* GetMain();
	static DatabaseSnapshot* GetSub();
	static void Release();
};

#endif // DATABASESNAPSHOT_H
