
#ifndef	_CPP_ODBC_H_
#define	_CPP_ODBC_H_

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

//#include "mutex.h"

#include <string>
using namespace std;

//��ѯ������ֶ�����
#define FIELD_NUM 32
#define FIELD_MAX_LEN 512

class CppODBC
{
public:
	CppODBC( );
	~CppODBC( );
	
private:
	void Uninitialize();
	// ��ջ���
	void ResetRevBuf();
	
//�����ӿ�
public:
	bool		Open( );
	bool		Close( );
	bool 		Connect( const char* pszDSN, const char* pszUName, const char* pszUPasswd );
	bool 		DisConnect( );
	bool 		Clear( );
	int		SetFieldMaxLen( int nMaxLen );
	unsigned int 	SQLQuery( const char* pszSQL );
	unsigned int	SQLExec( const char* pszSQL );
	bool 	SQLExecAutoID( const char *pszSQL, const char* pIdField, long &lIdentity );
	bool  IsOpen( );
	//��ѯ�Ľ������������ʱ���ظ��µļ�¼������ɾ��ʱ����ɾ��������
	unsigned int  	GetCount( );
	//���ز�ѯ���������
	unsigned int  	GetColumns( );
	int  			GetIntValue( unsigned int uiIndex );
	double 			GetDoubleValue( unsigned int uiIndex );
	char *   		GetStrValue( unsigned int uiIndex );
	unsigned int    GetUIntValue( unsigned int uiIndex );
	float           GetFloatValue( unsigned int uiIndex );
	time_t			GetTimetValue( unsigned int uiIndex );
	unsigned char	GetUCharValue( unsigned int uiIndex);
	unsigned long   GetULongValue(unsigned int uiIndex);
	//ȡ������
   bool     		Cancel( );
	//��ȡ�������
	unsigned int GetError( );
	//��һ��
	bool Next( );
	bool Eof( );
	// ִ�г�ʼ��
	bool Initialize();			

private:
	SQLHENV			 	V_OD_Env_;				// Handle ODBC environment ��Ż�������
	SQLHDBC			 	V_OD_hdbc_;				// Handle connection ���Ӿ��
	SQLHSTMT 		 	V_OD_hstmt_;			// SQL���ľ��
	SQLLEN		 	V_OD_rowanz_;			// ����Ӱ��ļ�¼����
	SQLSMALLINT			V_OD_colanz_;			// ����Ӱ��ļ�¼�������ֶ�����
	char*				pszField_[FIELD_NUM];	// ���һ����ѯ�����,���������ݲ�ѯ�������
	int				nMaxFiledLen_;				//�ֶε����ֵ
	bool 				bOpened_;
	bool				bConnected_;
	bool 				bEof_;
	
//	CNI_Mutex 		mutex_;
	int				errorNO;		//������
	
	SQLLEN m_res;
};

#endif


