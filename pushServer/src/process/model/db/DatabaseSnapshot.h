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
	// ���ڵ���odbc����
	CppODBC * m_pCppODBC;
	// ODBC����Դ
	char m_dsn[SHOT_LINK_LEN];
	// �û���
	char m_uname[SHOT_LINK_LEN];
	// ����
	char m_upass[SHOT_LINK_LEN];
	// ��
	static DatabaseSnapshot *m_sMainPointer;
	// ��
	static DatabaseSnapshot *m_sSubPointer;

private:
    DatabaseSnapshot(const char*, const char*, const char*);
    ~DatabaseSnapshot();
    // ��ʼ����Դ
	void InitialRes();
    // �������ݿ�����
	bool DBconnect();
	// �Ͽ����ݿ�����
	bool DBdisconnect();
	// ִ��sql���
	const int SqlExeOnly(const char*);
	// ִ��sql��ѯ���
	const int SqlExeQuery(const char*);
	
public:
	// ������û���¼
	const int InsertNewUser(const CPushUser*, const char*);
	// �����û�
	const int UpdateUser(const CPushUser*, const char*);
	// ɾ���û�
	const int DeleteUser(const CPushUser*);
	// ����ָ��ID���û�
	const int FindUser(const char *, const unsigned char, CPushUser *, char*, const unsigned int);

	// ����û�������Ϣ
	const int InsertUserBasicInfo(const CUserBaseInfoHead*, const unsigned short, const unsigned char,
				const unsigned int, const unsigned int);
	// �����û�������Ϣ
	const int FindUserBasicInfo(const unsigned short, const unsigned char, CUserBaseInfoHead &);
	
	// ����û���Ԥ����¼
	const int InsertUserEWarning(const char*, const unsigned char, const unsigned int, CEarlyWarningConditionRecord*);
	// �����û���Ԥ����¼
	const int UpdateUserEWarning(const char*, const unsigned char, const unsigned int, CEarlyWarningConditionRecord*);
	// ɾ���û���Ԥ����¼
	const int DeleteUserEWarning(const char*, const unsigned char, const unsigned int);
	// ɾ��ָ���û�������Ԥ����¼
	const int DeleteUserAllEWarning(const char*, const unsigned char);
	// ��ȡ�û���Ԥ����¼
	const int ReadUserEWarning(const char*, const unsigned char, const unsigned int, CEarlyWarningConditionRecord &);
	// �����û���Ԥ����¼
	const int FindUserEWarning(const char*, const unsigned char, const unsigned int);
	
	// �����ʷ��¼
	const int InsertHistory(const char*, const unsigned char, const unsigned int, const PushMsg*);
	// ɾ��С��ָ��ID��ʷ��¼
	const int DeleteUserHistroySmallerThanID(const char*, const unsigned char, const unsigned int, const unsigned char);
	// ��ȡ��ʷ��¼
	const int ReadHistory(const char*, const unsigned char, const unsigned int, const unsigned char, PushMsg&);
	
	// �����û�����ѡ������
	const int UpdateUserSelfStock(const char*, const unsigned char, const char*);
	// ��ȡ�û�����ѡ������
	const int ReadUserSelfStock(const char*, const unsigned char, char *, const unsigned int);
	
	static DatabaseSnapshot* GetMain();
	static DatabaseSnapshot* GetSub();
	static void Release();
};

#endif // DATABASESNAPSHOT_H
