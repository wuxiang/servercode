/*****
** ���㴦���߳�
** ���Դ��������жϣ���������
********************************/
#ifndef _CAL_OPERATING_THREAD_H
#define _CAL_OPERATING_THREAD_H

#include "ThreadImp.h"
#include "../model/template/IndexList.h"
#include <list>

class LargeMemoryCache;

enum TimesliceFlag
{
	// ��ʼ
	TF_BEGIN = 0,
	// �����ϴ�
	TF_CONTINUE,
	// ����
	TF_END,
};

// �ֶ�ɨ���ʶ
struct StageScanTag
{
	TimesliceFlag uState;
	IndexMap UserMark;
};

class CalOperatingThread : public ThreadImp {
public:
	CalOperatingThread();
	~CalOperatingThread();
	
	enum CalcSignalPropertyEnum
	{
		CSP_HQ_INDEX_RESET = NSP_RESERVE + 1,
		CSP_EXE_WARN,
		CSP_EXE_INFOMINE,
		CSP_EXE_NOTICE,
		CSP_EXE_INFOMINE_STATIS,
	};

	/*** m_iSignalProperty����
	**	bit						����˵��(0����ʶ�޲���)
	**	CSP_HQ_INDEX_RESET		Ҫ���������鶯̬��¼ָ��
	**	CSP_EXE_WARN			��̬�����и��£�ִ�йɼ�Ԥ��
	**	CSP_EXE_INFOMINE		��Ϣ�����и��£�ִ����Ϣ����ɨ��
	**	CSP_EXE_NOTICE			�����и��£�ִ�й���ɨ��
	**	CSP_EXE_INFOMINE_STATIS ��Ϣ����ͳ��
	***************************************************************/

private:
	LargeMemoryCache *m_pSendBuf;								// �������ݻ���(�߳���user�û��ظ�������)
	StageScanTag m_PublicNoticeFlag;							// ����ɨ���ʶ
	StageScanTag m_SelfStkInfoMineStatisFlag;					// ��ѡ����Ϣ����ͳ��ɨ���ʶ

// �����б�
private:
	CIndexList *m_pEwarningUserList;							// Ԥ���û��б�
	CIndexList *m_pActiveUserList;								// ��Ծ�û��б�

// ��ʱʱ��
private:
	time_t m_ExecuteStkWarningTimer;							// ���һ��ִ�йɼ�Ԥ������ʱ��
	time_t m_ExecuteStkPrivateNoticTimer;						// ���һ��ִ����ѡ��Ϣ���׼���ʱ��
	time_t m_ExecutePublicNoticTimer;							// ���һ��ִ�й�����Ϣ����ʱ��
	time_t m_ExecuteUserAliveCheckTimer;						// ���һ��ִ�м���û����״̬ʱ��
	time_t m_ExecuteSelfMineStatisTimer;						// ���һ��ִ����ѡ����Ϣ����ͳ��ʱ��

private:
	int ProcessStkPriceWarningCal();							// ִ�нڵ�ɼ�Ԥ������
	int PlatformStkPriceWarningCal(const IndexMap*);			// ƽ̨�ɼ�Ԥ��
	
	int ProcessPublicNoticeScan(int &);							// ִ�й������Ź������ɨ��
	int PlatformNoticeScan(const IndexMap*);					// ƽ̨����
	
	int ProcessUserAliveCheck();								// ִ�м���û����״̬
	int PlatformUserAliveCheck(const IndexMap*, const time_t);	// ƽ̨�û�״̬���
	
	int ExecuteInitial();										// ִ�г�ʼ��
	int PlatformUserInitial(const IndexMap*);					// ƽ̨�û����г�ʼ��
	
	int DetectStkLatestResetSignal();							// �������֤ȯ����������Ϣ�ź�
	int PlatformStkLatestReset(const IndexMap*);				// ƽ̨�û���������������Ϣ����
	
	int ProcedureMsgQueue();									// ������Ϣ����
	
	int ProcessSelfStkInfoMineScan();							// ִ����ѡ����Ϣ����ɨ��
	int PlatformSelfStkInfoMineScan(const IndexMap*);			// ƽ̨��ѡ����Ϣ����ɨ��
	
protected:
	bool InitialThreadEnv();									// ��ʼ�̻߳���
	int StartThread();											// �����߳�
	static void* ThreadProc(void*);								// �̴߳�����

public:
	// ���Ԥ���û�
	void AddEWarningUser(const unsigned int, const unsigned char);
	// �Ƴ�Ԥ���û�
	void RemoveEWarningUser(const unsigned int, const unsigned char);
	// ��ӻ�Ծ�û�
	void AddActiveUser(const unsigned int, const unsigned char);
	// �Ƴ���Ծ�û�
	void RemoveActiveUser(const unsigned int, const unsigned char);
	// ��ȡ��Ӧ��Դ����
	LargeMemoryCache* GetResponseCache();
};


#endif  /* _CAL_OPERATING_THREAD_H */
