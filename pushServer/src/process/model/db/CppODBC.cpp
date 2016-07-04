
#include "CppODBC.h"
//#include "CLog.h"
//extern CLog _log;

CppODBC::CppODBC( )
{
	bOpened_ = false;
	bConnected_ = false;	
	nMaxFiledLen_ = FIELD_MAX_LEN;
	bEof_ = false;
	errorNO = 0;

	for(int i=0; i<FIELD_NUM; i++)
		pszField_[i] = new char[ nMaxFiledLen_+ 1 ];

}

CppODBC::~CppODBC( )
{	
	Uninitialize( );
}
	
bool CppODBC::Open( )
{
	if ( bOpened_ )//已经打开了
		return true;
		
	long V_OD_erg; // result of functions 存放错误代码
	errorNO = 0;
	// allocate Environment handle and register version 
	V_OD_erg = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &V_OD_Env_ );
	if ( (V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO) )
	{
		//printf("Error AllocHandle\n");
		return false;
	}
	
	V_OD_erg = SQLSetEnvAttr( V_OD_Env_, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0 ); 
	if ( (V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO) )
	{
		//printf("Error SetEnv\n");
		SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env_);
		return false;
	}
	
	bOpened_ = true;
	return true;
}

bool CppODBC::Close( )
{
	if ( bConnected_ )
		return false;
		
	SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env_);
	bOpened_ = false;
	
	errorNO = 0;
	return true;
}

bool CppODBC::Connect( const char* pszDSN, const char* pszUName, const char* pszUPasswd )
{
	if ( !bOpened_ )
		return false;
		
	if ( pszDSN == NULL )
		return false;
		
	errorNO = 0;
	long				V_OD_erg = 0;
	SQLCHAR			V_OD_stat[64]= {0};	// Status SQL 执行sql语句的结果状态
	SQLINTEGER		V_OD_err = 0;			// sql语句执行后的错误代码
	SQLSMALLINT		V_OD_mlen = 0;			// 错误返回的消息文本大小
	SQLCHAR        V_OD_msg[256] = {0};	// 错误消息缓冲区
	
	// allocate connection handle, set timeout
	V_OD_erg = SQLAllocHandle( SQL_HANDLE_DBC, V_OD_Env_, &V_OD_hdbc_ ); 
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
	{
		//printf("Error AllocHDB %d\n",V_OD_erg);
		return false;
	}
	
	//(SQLPOINTER *)
	V_OD_erg = SQLSetConnectAttr(V_OD_hdbc_, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
	{
		//printf("Error SQLSetConnectAttr %d\n",V_OD_erg);
		SQLFreeHandle( SQL_HANDLE_DBC, V_OD_hdbc_ );
		return false;
	}
	V_OD_erg = SQLSetConnectAttr(V_OD_hdbc_, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
	//if ( !( (V_OD_erg == SQL_SUCCESS) || (V_OD_erg == SQL_SUCCESS_WITH_INFO) ) )
	//{
	//	printf("Error SQL_ATTR_LOGIN_TIMEOUT %d\n",V_OD_erg);
	//	//SQLFreeHandle( SQL_HANDLE_DBC, V_OD_hdbc_ );
	//	//return false;
	//}
	//else
	//{
	//	printf("SUCCESS SQL_ATTR_LOGIN_TIMEOUT %d\n",V_OD_erg);
	//}
	V_OD_erg = SQLSetConnectAttr(V_OD_hdbc_, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)5, 0);
	//if ( !( (V_OD_erg == SQL_SUCCESS) || (V_OD_erg == SQL_SUCCESS_WITH_INFO) ) )
	//{
	//	printf("Error SQL_ATTR_CONNECTION_TIMEOUT %d\n",V_OD_erg);
	//	//SQLFreeHandle( SQL_HANDLE_DBC, V_OD_hdbc_ );
	//	//return false;
	//}
	//else
	//{
	//	printf("SUCCESS SQL_ATTR_CONNECTION_TIMEOUT %d\n",V_OD_erg);
	//}
	
	// Connect to the datasource 
	//MysqlODBC //MyPostgres // mysqlitedb
	V_OD_erg = SQLConnect(V_OD_hdbc_, (SQLCHAR*) pszDSN, SQL_NTS,
                                     (SQLCHAR*) pszUName, SQL_NTS,
                                     (SQLCHAR*) pszUPasswd, SQL_NTS);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
	{
		//printf("Error SQLConnect %d\n",V_OD_erg);
		SQLGetDiagRec( SQL_HANDLE_DBC, V_OD_hdbc_, 1, 
		              V_OD_stat, &V_OD_err, V_OD_msg, 256, &V_OD_mlen );
		//printf("%s (%d)\n",V_OD_msg, V_OD_err);
		SQLFreeHandle( SQL_HANDLE_DBC, V_OD_hdbc_ );
		return false;
	}
	
	//printf("Connected !\n");
	
	V_OD_erg = SQLAllocHandle(SQL_HANDLE_STMT, V_OD_hdbc_ , &V_OD_hstmt_);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
	{
		//printf("Fehler im AllocStatement %d\n",V_OD_erg);
		SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc_, 1, V_OD_stat, &V_OD_err, V_OD_msg, 256, &V_OD_mlen);
		//printf("%s (%d)\n", V_OD_msg, V_OD_err);
		SQLDisconnect( V_OD_hdbc_ );
		SQLFreeHandle( SQL_HANDLE_DBC, V_OD_hdbc_ );
		return false;
	}
	V_OD_erg = SQLSetStmtAttr(V_OD_hstmt_, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)5, SQL_IS_UINTEGER);
	//if ( !( (V_OD_erg == SQL_SUCCESS) || (V_OD_erg == SQL_SUCCESS_WITH_INFO) ) )
	//{
	//	printf("Error SQL_ATTR_QUERY_TIMEOUT %d\n",V_OD_erg);
	//	//SQLFreeHandle( SQL_HANDLE_DBC, V_OD_hdbc_ );
	//	//return false;
	//}
	//else
	//{
	//	printf("SUCCESS SQL_ATTR_QUERY_TIMEOUT %d\n",V_OD_erg);
	//}
	
	bConnected_ = true;
	return true;
}

bool CppODBC::DisConnect( )
{
	errorNO = 0;
	if ( bConnected_ )
	{
		SQLFreeHandle( SQL_HANDLE_STMT,V_OD_hstmt_ );
    	SQLDisconnect( V_OD_hdbc_ );
		SQLFreeHandle( SQL_HANDLE_DBC, V_OD_hdbc_ );
		bConnected_ = false;
	}
	
	return true;
}

int CppODBC::SetFieldMaxLen( int nMaxLen )
{
	errorNO = 0;
	int nPreLen = nMaxFiledLen_;
	nMaxFiledLen_ =  nMaxLen;
	
	return nPreLen;
}

unsigned int CppODBC::SQLQuery( const char* pszSQL )
{
	if ( pszSQL == NULL )
		return 0;
		
	long				V_OD_erg = 0;
	SQLCHAR			V_OD_stat[64]= {0};	// Status SQL 执行sql语句的结果状态
	SQLINTEGER		V_OD_err = 0;			// sql语句执行后的错误代码
	SQLSMALLINT		V_OD_mlen = 0;			// 错误返回的消息文本大小
	SQLCHAR        V_OD_msg[256] = {0};	// 错误消息缓冲区
//	char*				pszBuf = NULL;


	 //清空缓冲区
    Clear();
	errorNO = 0;

	//查询 
    V_OD_erg=SQLExecDirect(V_OD_hstmt_, (SQLCHAR*)pszSQL, SQL_NTS);   
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
//		_log.WriteLog("CppODBC::SQLQuery Select %d,sql=%s\n",V_OD_erg,pszSQL);
      // printf("Error in Select %d\n", V_OD_erg);
       SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc_, 1, V_OD_stat, &V_OD_err, V_OD_msg, 256, &V_OD_mlen);
       //printf("%s (%d)\n",V_OD_msg, V_OD_err);
	   //_log.WriteLog("CppODBC::SQLQuery %s (%d)",V_OD_msg, V_OD_err);
	   errorNO = -1;
       return 0;
    }
    
        //获取查询结果的数量
    V_OD_erg = SQLRowCount(V_OD_hstmt_, &V_OD_rowanz_);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
      	return 0;
    }
    

    if ( V_OD_rowanz_ == 0 )//没有查询结果    	
    	return 0;
    
    //获取结果字段的数量
    V_OD_erg=SQLNumResultCols(V_OD_hstmt_, &V_OD_colanz_);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
		return 0;
    }
    //printf("Number of Columns %d\n",V_OD_colanz_);
      
    //绑定字段缓冲区
//  SQLLEN lll;
    for ( int i=0; i<V_OD_colanz_; i++ )
    {
		SQLBindCol(V_OD_hstmt_, i+1, SQL_C_CHAR, pszField_[ i ], nMaxFiledLen_, &m_res);
    } 
   
    //得到一行结果，结果放在绑定的缓冲区里面
    V_OD_erg = SQLFetch( V_OD_hstmt_ ); 
    if ( V_OD_erg != SQL_NO_DATA )
    	bEof_ = false;
    	
    return V_OD_rowanz_;
}

unsigned int CppODBC::SQLExec( const char* pszSQL )
{
	if ( pszSQL == NULL )
		return 0;
		
	long				V_OD_erg = 0;
	SQLCHAR			V_OD_stat[64]= {0};	// Status SQL 执行sql语句的结果状态
	SQLINTEGER		V_OD_err = 0;			// sql语句执行后的错误代码
	SQLSMALLINT		V_OD_mlen = 0;			// 错误返回的消息文本大小
	SQLCHAR        V_OD_msg[256] = {0};	// 错误消息缓冲区
//	char*				pszBuf = NULL;

	 //清空缓冲区
    Clear();
	errorNO = 0;
	
    V_OD_erg=SQLExecDirect(V_OD_hstmt_, (SQLCHAR*)pszSQL, SQL_NTS);   
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO) \
		&& (V_OD_erg != SQL_ERROR) ) //2012.2.6 对ODBC操作数据库进行处理,SQL_INVALID_HANDLE才表示网络连接出错.SQL_ERROR语法出错不管
    {
		//_log.WriteLog("CppODBC::SQLExecDirect=%d,sql=%s\n",V_OD_erg,pszSQL);
       //printf("Error in Select %d\n", V_OD_erg);
       SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc_, 1, V_OD_stat, &V_OD_err, V_OD_msg, 256, &V_OD_mlen);
       //printf("%s (%d)\n",V_OD_msg, V_OD_err);
		errorNO = -1;
       return 0;
    }
    
    //获取查询结果的数量
    V_OD_erg = SQLRowCount(V_OD_hstmt_, &V_OD_rowanz_);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
      	return 0;
    }
  	
    return V_OD_rowanz_;
}

bool CppODBC::SQLExecAutoID( const char *pszSQL, const char* pszIdField, long &lIdentity  )
{
	if ( pszSQL == NULL || pszIdField == NULL )
		return false;
	
	errorNO = 0;
	//不同的数据库处理方法不同
	//通用办法比较笨，需要增加自动增长字段的名字作为参数
	int nRes = 0;
	lIdentity = 0;
	
	nRes = SQLExec( pszSQL );
	if ( nRes == 0 )
		return false;

	//获取插入操作的数据表	
	string str = pszSQL;

	transform(str.begin(), str.end(), str.begin(), toupper);  
	int nLen1 = str.find("INTO");
	int nLen2 = str.find("VALUES");

	str = pszSQL;
	nLen1 += strlen("INTO");
	string str1 = str.substr(nLen1, nLen2-nLen1);
	nLen1 = 0;
	nLen1 = str1.find("(", 0);//从开始寻找左括号
	if ( nLen1 > 0 )
		str1 = str1.substr(0, nLen1);//消除带括号部分
	
	str = "SELECT MAX(";
	str += pszIdField;
	str += ") FROM ";
	str += str1;
	
	nRes = SQLQuery(str.c_str());
	if ( !Eof() )
		lIdentity = GetIntValue(0);
	else
		return false;
	return true;		
}

bool CppODBC::IsOpen( )
{
	return bOpened_;
}

//查询的结果数量，更新时返回更新的记录数量，删除时返回删除的数量
unsigned int  	CppODBC::GetCount( )
{
	return V_OD_rowanz_;
}

//返回查询结果的列数两
unsigned int CppODBC::GetColumns( )
{
	return V_OD_colanz_;
}

int CppODBC::GetIntValue( unsigned int uiIndex )
{
	if ( (SQLSMALLINT)uiIndex > V_OD_colanz_ )
		return 0;
		
	errorNO = 0;
	int nField = 0;
	if (pszField_[uiIndex] != NULL)
		nField = atoi( pszField_[uiIndex] );//nField = _atoi64( pszField_[uiIndex] );//

	return nField;	
}

double CppODBC::GetDoubleValue( unsigned int uiIndex )
{
	if ( (SQLSMALLINT)uiIndex > V_OD_colanz_ )
		return 0;
	errorNO = 0;	
	double nField = 0;
	if (pszField_[uiIndex] != NULL)
		nField = atof( pszField_[uiIndex] );
	
	return nField;	
}

char * CppODBC::GetStrValue( unsigned int uiIndex )
{
	if ( (SQLSMALLINT) uiIndex > V_OD_colanz_ )
		return NULL;
	
	return pszField_[uiIndex];
}

//取消操作
bool CppODBC::Cancel( )
{
	return true;
}

//获取错误代码
unsigned int CppODBC::GetError( )
{
	return errorNO;
}

//下一个
bool CppODBC::Next( )
{
	errorNO = 0;
	long V_OD_erg = 0;
	
	ResetRevBuf();
	V_OD_erg = SQLFetch( V_OD_hstmt_ ); 
    if ( V_OD_erg != SQL_NO_DATA )
    	bEof_ = false;
    else
    	bEof_ = true;
    
    return !bEof_;   	
}

bool CppODBC::Eof()
{
	return bEof_;
}

unsigned int CppODBC::GetUIntValue( unsigned int uiIndex)
{
    if ( uiIndex < 0 || (int)uiIndex > (int)V_OD_colanz_ )
		return 0;

	errorNO = 0;
	unsigned int nField = 0;
	if (pszField_[uiIndex] != NULL)
		sscanf(pszField_[uiIndex], "%u", &nField);

	return (unsigned int)nField;
}

float   CppODBC::GetFloatValue( unsigned int uiIndex )
{
    if ( uiIndex < 0 || (int)uiIndex > (int)V_OD_colanz_ )
		return 0;
	errorNO = 0;
	double nField = 0;
	if (pszField_[uiIndex] != NULL)
		nField = atof( pszField_[uiIndex] );

	return (float)nField;
}


time_t CppODBC::GetTimetValue( unsigned int uiIndex )
{
	if ( uiIndex < 0 || (int)uiIndex > (int)V_OD_colanz_ )
		return 0;
	errorNO = 0;
	
	unsigned long mmm;
	sscanf(pszField_[uiIndex], "%lu", &mmm);
	return (time_t)mmm;
}

unsigned char CppODBC::GetUCharValue( unsigned int uiIndex)
{
	if ( uiIndex < 0 || (int)uiIndex > (int)V_OD_colanz_ )
		return 0;

	errorNO = 0;
	int nField = 0;
	if (pszField_[uiIndex] != NULL)
		nField = atoi( pszField_[uiIndex] );

	return (unsigned char)nField;
}

unsigned long  
CppODBC::GetULongValue(unsigned int uiIndex)
{
	if ( uiIndex < 0 || (int)uiIndex > (int)V_OD_colanz_ )
		return 0;
	errorNO = 0;
	
	unsigned long mmm;
	sscanf(pszField_[uiIndex], "%lu", &mmm);
	return mmm;
}

// 执行初始化
bool
CppODBC::Initialize()
{
	char *pszBuf = NULL;
	for(int i=0; i < FIELD_NUM; i++)
	{
		pszBuf = new char[ nMaxFiledLen_+ 1 ];
    	memset( pszBuf, 0, nMaxFiledLen_+ 1);
		pszField_[i] = pszBuf;
	}
	return true;
}

void
CppODBC::Uninitialize()
{
	for (int i = 0; i < FIELD_NUM; i++)
	{
		if ( pszField_[i] != NULL )
			delete [] pszField_[i];
		pszField_[i] = NULL;
	}
}

bool CppODBC::Clear( )
{
	SQLCloseCursor(V_OD_hstmt_);
	errorNO = 0;
	V_OD_rowanz_ = 0;
	V_OD_colanz_ = 0;
	bEof_ = true;
	ResetRevBuf();
	return true;
}

// 清空缓冲
void
CppODBC::ResetRevBuf()
{
	for (int i = 0; i < FIELD_NUM; i++)
	{
		if ( pszField_[i] != NULL )
		{
			memset(pszField_[i], 0, nMaxFiledLen_+ 1);
		}
	}
}

