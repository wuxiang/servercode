#ifndef PUBLICNEWS_H
#define PUBLICNEWS_H

#include "PNmsg.h"
#include "../../data_type.h"
#include <string>
#include <map>
#include <vector>

#ifndef NOTICE_LINK_LEN
#define NOTICE_LINK_LEN 32
#endif

class CppODBC;
class CPushUser;

class CPublicNews
{
private:
	CPublicNews(const unsigned int, const char*, const char*, const char*);
	~CPublicNews();
	
private:
	// �����������
	const unsigned int m_uMaxCacheNum;
	// ���ڵ���odbc����
	CppODBC * m_pCppODBC;
	// ���ݻ���
	PNmessage *m_pRecordCache;
	// �ڵ��ϵӳ��(key��Ϣ���ݿ��ʶID valueͷ�ڵ�λ������)
	std::map<const unsigned long, unsigned int> m_mpMarkNodes;
	// ��ǰռ�õĽڵ���Ŀ����
	unsigned int m_uOccupyIndex;
	// ��ǰ�����ݿ�����¼��ʶID
	unsigned long m_luCurrentMaxRecID;
	// ��ǰ��״̬
	int m_iCurrentStatus;
	// ODBC����Դ
	char m_dsn[NOTICE_LINK_LEN];
	// �û���
	char m_uname[NOTICE_LINK_LEN];
	// ����
	char m_upass[NOTICE_LINK_LEN];
	// ��ָ��
	static CPublicNews * m_pPublicNews;
	// ��������Ϣ��ʾID
	std::vector<unsigned long> m_GotNewMsgID;
	
private:
	const unsigned int GetCirculateIndex(const unsigned int);
	// ��Ӽ�¼
	const int AddNewRecord(const PNmessage &);
	// ��ȡָ�������ļ�¼ָ��
	PNmessage* GetIndexPNmessage(const unsigned int);
	// �Ƴ�ָ�������ļ�¼
	const int RemoveIndexNode(const unsigned int);
	// ���½ڵ�����
	bool UpdateLinkNode(const unsigned long, const unsigned int Index);
	// ��ʼ����Դ
	void InitialRes();
	// ��������
	void resetData();
	// �������ݿ�����
	bool DBconnect();
	// �Ͽ����ݿ�����								
	bool DBdisconnect();
	// ��������
	const int RequestMsg(const char *, bool);
	// ��ȡ���10��Ĺ���
	const int ReadLatest10DaysMsg();
	// ����ƽ̨
	const int ParsePlatformCode(char*, PNmessage*);
	// ƥ�乫����Ϣ��������
	const int MatchNoticeSend(const CPushUser*, const PNmessage*);
	// ��������״̬
	void SetCurrentStatus(const int);
	// ��λ���ݿ��ʶΪ�Ѷ�ȡ
	const int SendCallBackToDb();
	// ����UTF8��ȡ�ַ�������
	char* LeftUtf8String(const int, const char *,
			char *, const int);
	// �����汾��
	const int ParseVersionRange(char*, PNmessage*);
	// ��ȡ�Ѿ����͵��������ID
	const unsigned long GetMaxDbIDMarkByOne();

public:
	static CPublicNews * getPublicNewsPointer();
	static void releasePublicNewsPointer();
	// ��ʼ������
	bool InitialData();
	// ��������
	const int GetDynaData();
	// �������µ���Ϣ
	const unsigned long GetLatestMsg(const unsigned long, const CPushUser*, PNmessage&);
	// ��ȡָ������Ϣ
	bool GetIndexPNmessage(const unsigned long, PNmessage&);
	// ��ȡ��ǰ��״ֵ̬
	const int GetCurrentStatus()const;
};

#endif //PUBLICNEWS_H
