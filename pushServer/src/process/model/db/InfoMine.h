#ifndef INFOMINE_H
#define INFOMINE_H

#include "../template/ContainerCmp.h"
#include "../../data_type.h"
#include <string>
#include <map>

#ifndef INFOMINE_MAXTITLELEN
#define INFOMINE_MAXTITLELEN 190
#endif

#ifndef INFOMINE_LINK_LEN
#define INFOMINE_LINK_LEN 32
#endif

struct IMmessage
{
	// ���ݿ��ID
	unsigned long MsgID;
	// ֤ȯ����
	char strStockCode[MAX_STKCODE_LEN + 1];
	// ����(������190�ֽ�)
	char strTitle[INFOMINE_MAXTITLELEN + 1];
	// ����ʱ��
	time_t ttNewsTime;
	// ǰһ��ͬ������
	unsigned int Previous;
	// ��һ��ͬ������
	unsigned int Next;

	IMmessage()
	{
		MsgID = (unsigned long)-1;
		memset(strStockCode, 0, MAX_STKCODE_LEN);
		memset(strTitle, 0, INFOMINE_MAXTITLELEN);
		ttNewsTime = 0;
		Previous = (unsigned int)-1;
		Next = (unsigned int)-1;
	}

	IMmessage& operator = (const IMmessage &item)
	{
		MsgID = item.MsgID;
		strncpy(strStockCode, item.strStockCode, MAX_STKCODE_LEN);
		strncpy(strTitle, item.strTitle, INFOMINE_MAXTITLELEN);
		ttNewsTime = item.ttNewsTime;
		Previous = (unsigned int)-1;
		Next = (unsigned int)-1;
		return *this;
	}
};

class CppODBC;
struct InfoMineStatisTag;

class CInfoMine
{
private:
	CInfoMine(const unsigned int, const char*, const char*, const char*);
	~CInfoMine();
	
private:
	// �����������
	const unsigned int m_uMaxCacheNum;
	// ���ڵ���odbc����
	CppODBC * m_pCppODBC;
	// ���ݻ���
	IMmessage *m_pRecordCache;
	// �ڵ��ϵӳ��(key֤ȯ���� valueͷ�ڵ�λ������)
	std::map<const char*, unsigned int, NodeMarkCmpFun> m_mpMarkNodes;
	// ��Ϣ���׸�����ͳ��ֵ(key֤ȯ���� value��֤ȯ��Ӧ����Ϣ������Ŀ)
	std::map<std::string, InfoMineStatisTag> m_mpInfoMineStatics;
	// ��Ҫʵʱ���͵���Ϣ�����б�
	std::map<unsigned long, unsigned int> m_mpInfoMineRT;
	// ��ǰռ�õĽڵ���Ŀ����
	unsigned int m_uOccupyIndex;
	// ��ǰ�����ݿ�����¼��ʶID
	unsigned long m_luCurrentMaxRecID;
	// �Ƿ�ʼ����ʵʱ������Ϣͳ��
	bool m_bStartRTStatis;
	// ��ǰ��״̬
	int m_iCurrentStatus;
	// ODBC����Դ
	char m_dsn[INFOMINE_LINK_LEN];
	// �û���
	char m_uname[INFOMINE_LINK_LEN];
	// ����
	char m_upass[INFOMINE_LINK_LEN];
	// ��ָ��
	static CInfoMine * m_pInfoMine;
	
private:
	const unsigned int GetCirculateIndex(const unsigned int);
	// ��Ӽ�¼
	const int AddNewRecord(const IMmessage &, unsigned int &);
	// ��ȡָ��֤ȯ��ͷλ������
	const unsigned int GetStkInfoMineHeadIndex(const char*);
	// ��ȡָ��֤ȯ��βλ������
	const unsigned int GetStkInfoMineTailIndex(const unsigned int);
	// ����ָ��MsgID����Ϣλ������
	const unsigned int GetIMmessageIndexByMsgID(const unsigned int, const unsigned long);
	// ���ұ�ָ��ID�����Ϣλ������
	const unsigned int GetIMmessageIndexLargerThanMsgID(const unsigned int, const unsigned long);
	// ��ȡָ�������ļ�¼ָ��
	IMmessage* GetIndexIMmessage(const unsigned int);
	// �Ƴ�ָ�������ļ�¼
	const int RemoveIndexNode(const unsigned int);
	// ���½ڵ�����
	bool UpdateLinkNode(const char *stkCode, const unsigned int Index);
	// ��ʼ����Դ
	void InitialRes();
	// ��������
	void resetData();
	// �������ݿ�����
	bool DBconnect();
	// �Ͽ����ݿ�����								
	bool DBdisconnect();
	// ��ȡ�������Ϣ����
	int CompanyNews_Query_CurDate();
	// ��ʼ��ȡ��Ч����Ϣ����
	int ReadValidInfoMine();
	// ���ʵʱ���͵���Ϣ��������
	bool AddRTInfoMine(const unsigned int, const IMmessage &);
	// ��������״̬
	void SetCurrentStatus(const int);
	
public:
	static CInfoMine * getInfoMinePointer();
	static void releaseInfoMinePointer();
	// ��ʼ������
	bool InitialData();
	// ��������
	const int GetDynaData();
	// �������µ���Ϣ
	const unsigned long GetLatestMsg(const char*, const unsigned long, IMmessage&);
	// ��ȡָ������Ϣ
	bool GetIndexIMmessage(const char*, const unsigned long, IMmessage&);
	// ִ����Ϣ���׵ĸ���ͳ��
	const int ExecuteInfoMineStatis(const int);
	// ��ȡ֤ȯ����Ϣ���׸���
	const unsigned int GetStockInfoMineCount(const char*);
	// ��Ϣ����ͳ���Ƿ�ִ�гɹ�
	bool IsInfoMineStatisSuccess();
	// �����Ƿ�ʼʵʱ������Ϣ����
	void SetStartRTInfoMineFlag(bool);
	// �������µ���Ϣ
	const int GetLatestMsg(const unsigned long, IMmessage&);
	// ��ȡ��ǰ��״ֵ̬
	const int GetCurrentStatus()const;
};

#endif //INFOMINE_H
