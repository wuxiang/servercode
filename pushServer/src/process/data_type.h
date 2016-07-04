#ifndef _DATA_TYPE_H
#define _DATA_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../util/util.h"
#include "../util/common/common_types.h"
#include "model/data/mword.h"

#pragma pack(1)
// �߳�����״̬����
enum ThreadRunState {
	Thread_Initial = 0,							// ��ʼ״̬
	Thread_Waiting,								// �ȴ�����
	Thread_Running,								// ��������
	Thread_Terminate							// ��ֹ����
};

// ��������״̬
enum NetStatus
{
	Net_Normal = 0x001,							// ����״̬
	Net_Closed,									// �ر�״̬
	Net_Abnormal,								// �쳣״̬
};

// ʹ�õĳ��ȶ���
enum SystemCommonLen
{
	MAX_USER_ID_LEN = 32,			// ���ͻ��˱�ʶID����
	MAX_USER_NAME_LEN = 50,			// ����û���
	MAX_USER_PWD_LEN = 20,			// ����û���¼���볤��
	MAX_LOCAL_VERSION = 6,			// �汾��ʶ��󳤶�
	MAX_TEL_NUMBER_LEN = 16,		// �绰������󳤶�
	MAX_TEL_ID_LEN = 15,			// �󶨵�id��󳤶�
	MAX_IOS_PUSH_TOKEN_LEN = 64,	// IOS�û�����TOKEN��󳤶�
	MAX_WP_PUSH_TOKEN_LEN = 128,	// WP�û�����TOKEN��󳤶�
	MAX_STKCODE_LEN = 10,			// ֤ȯ������󳤶�
	MAX_STKNAME = 32,				// ��Ʊ������󳤶�
	MAX_LINK_KEY_LEN = 32,			// ������󳤶�
	MAX_IP_LEN = 24,				// IP����ַ�������
	MAX_SERVER_CODE_LEN = 8,		// ������������󳤶�
	MAX_SINGLE_MSG = 256,			// �����Ϣ����
	MAX_WIN8_PUSH_TOKEN_LEN = 256,	// WIN8�û�����TOKEN��󳤶�
};
// -------------------------           /

// ���ݽ������ؽ��
enum ParseResEnum
{
	PR_SUCCESS = 0,								// ��ȷ�ķ���
	PR_ERROR,									// ���������д���
	PR_CACHE_FULL,								// ��������
	PR_UNDEFINE_TYPE,							// δʶ�������
};

// ͨ������ͷ
struct ACC_CMDHEAD
{
	unsigned short	m_wCmdType;					//��������
	unsigned short	m_wAttr;					//�����ֶ�
	unsigned int	m_nLen;						//���ݰ��ĳ���,������������ͷ
	unsigned int	m_nExpandInfo;				//��չ��Ϣ��������������ݣ���ӦҪԭ������
};
// -------------------------           /

//�ӿ����ͣ�m_wCmdTypeȡֵ
enum ENUM_ACC_CMDHEAD
{
	ACCCMD_KEEPALIVE = 0x8080,					// �������ݰ�
	ACCCMD_SERVERLOGIN,							// Ӧ�÷�������½, ͨ�÷�����Ҳʹ�øýӿ�
	ACCCMD_STATCOMMINTERFACE = 0xA0A0,			// ͳ���û���Ϣͨ�ýӿ�
	ACCCMD_REQ_GUID = 0xA0A1					// ������������
};
// -------------------------           /

//���Զ��壬���ܺ�ѹ�����Կ����׶β�ʹ��
enum
{
	ACCATTR_AES = 0x0001,						//AES ����
	ACCATTR_MD5 = 0x0002,						//md5����
	ACCATTR_ENCYMASK = 0x000F,					//��������

	ACCATTR_CPSMASK = 0x00F0,					//ѹ������

	ACCATTR_NAMETYPE = 0x0100,					//�ʻ����õ��û���������ʽ

	ACCATTR_COMINTERFACE_RET = 0x0200,			//ͨ�ýӿ����Զ���,�����˸�λ��ʾͨ�÷��������뷵��
	ACCATTR_PUSHINFO = 0x0400,					//ͨ�ýӿ����Զ���,�����˸�λ��ʾ����Ϣ��ͨ�÷��������ͷ��ص�
};
// -------------------------           /

//	ACCCMD_KEEPALIVE���ͻ��˺ͷ���������ACC_KEEPALIVESECʱ���ڶ�û������ͨ�ŵĻ����ͻ��˷��͸����ݰ���
//	������ԭ�����أ�
//	m_nLen = 0; m_wAttr = 0;
// -------------------------           /

// Ӧ�÷�������½�ʻ�������������������½�͹ر����ӣ�������û����Ӧ
// m_nLen = sizeof(ACC_SERVERLOGIN); m_wAttr = 0;
struct ACC_SERVERLOGIN
{
	char		m_cValid[8];//��������֤��
	unsigned int m_nSerId;	//������id����id������ip��ʾ
};
// -------------------------           /

// ƽ̨���Ͷ���
enum PlatFormEnum
{
	J2ME = 1,							// J2ME
	WP7 = 2,							// WP7
	OPHONE_GPHONE = 3,					// android
	IPHONE = 4,							// IPHONE
	S60 = 5,							// S60
	IPAD = 6,							// IPAD��׼��
	IPAD_HIGH_END = 7,					// IPAD�߶˰�
	GPAD = 8,							// GPAD
	WP7CMCC = 9,                       	// �ֻ�֤ȯWP7
	GPHONECMCC = 10,                   	// �ֻ�֤ȯandroid�ֻ�
	IPHONECMCC = 11,                   	// �ֻ�֤ȯiphone
	WIN8_PHONE = 12,					// WIN8�ֻ�
};

enum PlatFormClassifiedCodeEnum
{
	PFCC_OTHER = 0,						// ��������
	PFCC_ANDROID,						// ANDROIDƽ̨
	PFCC_IOS,							// IOSƽ̨
	PFCC_WP7,							// WP7ƽ̨
	PFCC_WIN8,							// WIN8ƽ̨
};

enum NoticeClassifiedCodeEnum
{
	NCCE_OTHER = 0,						// ��������
	NCCE_IPHONE,
	NCCE_ANDROID,
	NCCE_WP7,
	NCCE_IPAD,
	NCCE_GPAD,
};

// �ͻ�������ͷ����
typedef struct sub_head
{
	unsigned short	sub_type;
	unsigned short  sub_attrs;
	unsigned short  sub_length;
	unsigned int	sub_extend;
};
// -------------------------           /

// ����������sub_type����
enum SubType {
	ST_ADD = 300,					// ��Ӽ�¼
	ST_DEL = 301,					// ɾ����¼
	ST_HIS = 302,					// ��ʷ��¼
	ST_LATEST = 303,				// ��ǰԤ����¼
	ST_PUSH = 304,					// ���͵�Ԥ����Ϣ
	ST_KEEP = 305,					// ����
	ST_TOKEN = 306,					// �ϴ�����
	ST_RECGOT = 307,				// ��ȡ��ǰ����
	ST_EDIT = 308,					// �༭Ԥ����¼
	ST_REVERIFY = 309,				// Ҫ��������֤
	ST_PUSH_PUBLIC = 310,			// ���͹�����Ϣ
	ST_PUSH_MINE = 311,				// ������Ϣ����
	ST_PUBLIC_HIS = 312,			// ��ȡ������Ϣ��ʷ��Ϣ
	ST_ERROR = 313,					// ���ش���
	ST_REQ_NOTICE = 314,			// �������µĹ�����Ϣ
	ST_UPD_SELF_STOCK = 315,		// �ϴ���ѡ��
	ST_SELF_STOCK_RECVMARK = 316,	// ��ѡ����Ϣ���׻�Ӧ
	ST_GET_NOWARNING_MG = 317,		// android��Ԥ���û��������µ���Ϣ
	ST_GET_INFOMINE_COUNT = 318,	// ������Ϣ���׵���Ŀ
	
	ST_GET_LASTEST_USER_DATA = 1000,// �����û�����������
};

enum
{
	HQTYPE_STATIC	=	64000,
	HQTYPE_DYNA		=	64001,
	HQTYPE_SIMPLEDYNA	=	64002,
	HQTYPE_FUTUREDYNA	=	64003,
	HQTYPE_ORIGINSTATIC	=	64005,
};

//��������ͷ
struct ACC_JAVA_HEAD
{
	unsigned char	cSparate;//�ָ�����'{', ':'��, ����0�͡�}��'H' tag
	unsigned short	type;
	unsigned short  attrs;//0
	unsigned short  length;
};
//���ݷ���ͷ
struct ACC_JAVA_HEAD32
{
	unsigned char	cSparate;//�ָ�����'{', ':'��, ����0�͡�}��'H' tag
	unsigned short	type;
	unsigned short  attrs;//0x0008
	unsigned int    length;
};

//��̬���� HQTYPE_STATIC, ���� sizeof(ACC_STATICASKHEAD)
struct ACC_STATICASKHEAD
{
	WORD	m_nMarket;
	DWORD	m_nDate;
	DWORD	m_nCrc;
};
// -------------------------           /

//��Ӧsizeof(ACC_STATICASKHEAD)+sizeof(ACC_STK_STATIC)*N
struct ACC_STK_STATIC
{
	WORD	m_wMarket;					//���MARKET_TYPE
	char	m_szLabel[MAX_STKCODE_LEN];	//����
	char	m_szName[MAX_STKNAME];		//����
	BYTE	m_cType;					//STK_TYPE
	BYTE	m_nPriceDigit;				//�۸���С�ֱ��ʣ��ǳ���Ҫ��ÿһ��DWORD���͵ļ۸�Ҫ����10^m_nPriceDigit���������ļ۸�
	short	m_nVolUnit;					//�ɽ�����λ��ÿһ�ɽ�����λ��ʾ���ٹ�
	MWORD	m_mFloatIssued;				//��ͨ�ɱ�
	MWORD	m_mTotalIssued;				//�ܹɱ�
	
	DWORD	m_dwLastClose;				//����
	DWORD	m_dwAdvStop;				//��ͣ
	DWORD	m_dwDecStop;				//��ͣ
};
// -------------------------           /

//HQTYPE_ORIGINSTATIC�����HQTYPE_STATICһ��;��Ӧsizeof(ACC_STATICASKHEAD)+sizeof(ACC_ORIGINSTK_STATIC)*N
struct ACC_ORIGINSTK_STATIC
{
	enum STK_TYPE
	{
		INDEX = 0,				//ָ��
		STOCK = 1,				//��Ʊ
		FUND = 2,				//����
		BOND = 3,				//ծȯ
		OTHER_STOCK = 4,		//������Ʊ
		OPTION = 5,				//ѡ��Ȩ
		EXCHANGE = 6,			//���
		FUTURE = 7,				//�ڻ�
		FTR_IDX = 8,			//��ָ
		RGZ = 9,				//�Ϲ�֤
		ETF = 10,				//ETF
		LOF = 11,				//LOF
		COV_BOND = 12,			//��תծ
		TRUST = 13,				//����
		WARRANT = 14,			//Ȩ֤
		REPO = 15,				//�ع�
		COMM = 16,				//��Ʒ�ֻ�
	};
	WORD	m_wStkID;						//���г���Ψһ��ʾ,�ڱ��г��ڵ����
	char	m_strLabel[MAX_STKCODE_LEN];	//����

	char	m_strName[MAX_STKNAME];			//����
	BYTE	m_cType;						//STK_TYPE
	BYTE	m_nPriceDigit;					//�۸���С�ֱ��ʣ��ǳ���Ҫ��ÿһ��DWORD���͵ļ۸�Ҫ����10^m_nPriceDigit���������ļ۸�
	short	m_nVolUnit;						//�ɽ�����λ��ÿһ�ɽ�����λ��ʾ���ٹ�
	MWORD	m_mFloatIssued;					//��ͨ�ɱ�
	MWORD	m_mTotalIssued;					//�ܹɱ�
	
	DWORD	m_dwLastClose;					//����
	DWORD	m_dwAdvStop;					//��ͣ
	DWORD	m_dwDecStop;					//��ͣ
};
// -------------------------           /

//HQTYPE_DYNA ��̬��������sizeof(WORD),��ʾ�г�����;����sizeof(ACC_STK_DYNA)*N
struct ACC_STK_DYNA
{
	WORD	m_wStkID;				//��ƱID
	DWORD	m_time;					//�ɽ�ʱ��, 32λ
	DWORD	m_dwOpen;				//����
	DWORD	m_dwHigh;				//���
	DWORD	m_dwLow;				//���
	DWORD	m_dwNew;				//����
	MWORD	m_mVolume;				//�ɽ���
	MWORD	m_mAmount;				//�ɽ���
	MWORD	m_mInnerVol;			//���̳ɽ���,<0��ʾ�ñʳɽ�Ϊ��������>=0��ʾ������,����ֵ��ʾ���̳ɽ���
	DWORD	m_dwTickCount;			//�ۼƳɽ�����
	DWORD	m_dwBuyPrice[5];		//ί��۸�
	DWORD	m_dwBuyVol[5];			//ί����
	DWORD	m_dwSellPrice[5];		//ί���۸�
	DWORD	m_dwSellVol[5];			//ί����
	DWORD	m_dwOpenInterest;		//�ֲ���(�ڻ���ָ����)
	DWORD	m_dwSettlePrice;		//�����(�ڻ���ָ�ֻ�����)
};

// ������Ϣ���Ͷ���
enum PushTypeEnum
{
	PTE_EWARNING = 0,				// �ɼ�Ԥ��
	PTE_INFOMINE,					// ��Ϣ����
	PTE_NOTICE						// ������Ϣ
};

// ���͹������Ͷ���
enum PublicNoticeTypeEnum
{
	NOTICE_NORMAL = 0,				// ��ͨ����
	NOTICE_EXTENSION,				// �����
};

// ��Ϣ��������
enum InfoMineTypeEnum
{
	INFOMINE_NORMAL = 0,			// ��ͨ��Ϣ����
	INFOMINE_STATISTIC,				// ͳ����Ϣ����
};

enum UploadSelfStkOperTypeEnum
{
	USSOT_ADD = 0,					// ���
	USSOT_REMOVE,					// ɾ��
	USSOT_OVERLAP					// ����
};


#pragma pack()

#ifdef __cplusplus
}
#endif

#endif  /* _DATA_TYPE_H */
