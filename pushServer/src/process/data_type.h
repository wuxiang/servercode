#ifndef _DATA_TYPE_H
#define _DATA_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../util/util.h"
#include "../util/common/common_types.h"
#include "model/data/mword.h"

#pragma pack(1)
// 线程运行状态定义
enum ThreadRunState {
	Thread_Initial = 0,							// 初始状态
	Thread_Waiting,								// 等待运行
	Thread_Running,								// 正在运行
	Thread_Terminate							// 终止运行
};

// 网络运行状态
enum NetStatus
{
	Net_Normal = 0x001,							// 正常状态
	Net_Closed,									// 关闭状态
	Net_Abnormal,								// 异常状态
};

// 使用的长度定义
enum SystemCommonLen
{
	MAX_USER_ID_LEN = 32,			// 最大客户端标识ID长度
	MAX_USER_NAME_LEN = 50,			// 最大用户名
	MAX_USER_PWD_LEN = 20,			// 最大用户登录密码长度
	MAX_LOCAL_VERSION = 6,			// 版本标识最大长度
	MAX_TEL_NUMBER_LEN = 16,		// 电话号码最大长度
	MAX_TEL_ID_LEN = 15,			// 绑定的id最大长度
	MAX_IOS_PUSH_TOKEN_LEN = 64,	// IOS用户推送TOKEN最大长度
	MAX_WP_PUSH_TOKEN_LEN = 128,	// WP用户推送TOKEN最大长度
	MAX_STKCODE_LEN = 10,			// 证券代码最大长度
	MAX_STKNAME = 32,				// 股票名称最大长度
	MAX_LINK_KEY_LEN = 32,			// 链接最大长度
	MAX_IP_LEN = 24,				// IP最大字符串长度
	MAX_SERVER_CODE_LEN = 8,		// 服务器编码最大长度
	MAX_SINGLE_MSG = 256,			// 最大消息长度
	MAX_WIN8_PUSH_TOKEN_LEN = 256,	// WIN8用户推送TOKEN最大长度
};
// -------------------------           /

// 数据解析返回结果
enum ParseResEnum
{
	PR_SUCCESS = 0,								// 正确的返回
	PR_ERROR,									// 请求数据有错误
	PR_CACHE_FULL,								// 缓存已满
	PR_UNDEFINE_TYPE,							// 未识别的类型
};

// 通用数据头
struct ACC_CMDHEAD
{
	unsigned short	m_wCmdType;					//请求类型
	unsigned short	m_wAttr;					//属性字段
	unsigned int	m_nLen;						//数据包的长度,不包括该数据头
	unsigned int	m_nExpandInfo;				//扩展信息，如果是请求数据，响应要原样返回
};
// -------------------------           /

//接口类型，m_wCmdType取值
enum ENUM_ACC_CMDHEAD
{
	ACCCMD_KEEPALIVE = 0x8080,					// 心跳数据包
	ACCCMD_SERVERLOGIN,							// 应用服务器登陆, 通用服务器也使用该接口
	ACCCMD_STATCOMMINTERFACE = 0xA0A0,			// 统计用户信息通用接口
	ACCCMD_REQ_GUID = 0xA0A1					// 用于请求推送
};
// -------------------------           /

//属性定义，加密和压缩属性开发阶段不使用
enum
{
	ACCATTR_AES = 0x0001,						//AES 加密
	ACCATTR_MD5 = 0x0002,						//md5加密
	ACCATTR_ENCYMASK = 0x000F,					//加密掩码

	ACCATTR_CPSMASK = 0x00F0,					//压缩掩码

	ACCATTR_NAMETYPE = 0x0100,					//帐户是用的用户名密码形式

	ACCATTR_COMINTERFACE_RET = 0x0200,			//通用接口属性定义,设置了该位表示通用服务器必须返回
	ACCATTR_PUSHINFO = 0x0400,					//通用接口属性定义,设置了该位表示该信息是通用服务器推送返回的
};
// -------------------------           /

//	ACCCMD_KEEPALIVE，客户端和服务器端在ACC_KEEPALIVESEC时间内都没有数据通信的话，客户端发送该数据包，
//	服务器原样返回；
//	m_nLen = 0; m_wAttr = 0;
// -------------------------           /

// 应用服务器登陆帐户服务器，如果不容许登陆就关闭连接；该请求没有响应
// m_nLen = sizeof(ACC_SERVERLOGIN); m_wAttr = 0;
struct ACC_SERVERLOGIN
{
	char		m_cValid[8];//服务器验证串
	unsigned int m_nSerId;	//服务器id，该id可以用ip标示
};
// -------------------------           /

// 平台类型定义
enum PlatFormEnum
{
	J2ME = 1,							// J2ME
	WP7 = 2,							// WP7
	OPHONE_GPHONE = 3,					// android
	IPHONE = 4,							// IPHONE
	S60 = 5,							// S60
	IPAD = 6,							// IPAD标准版
	IPAD_HIGH_END = 7,					// IPAD高端版
	GPAD = 8,							// GPAD
	WP7CMCC = 9,                       	// 手机证券WP7
	GPHONECMCC = 10,                   	// 手机证券android手机
	IPHONECMCC = 11,                   	// 手机证券iphone
	WIN8_PHONE = 12,					// WIN8手机
};

enum PlatFormClassifiedCodeEnum
{
	PFCC_OTHER = 0,						// 其它归类
	PFCC_ANDROID,						// ANDROID平台
	PFCC_IOS,							// IOS平台
	PFCC_WP7,							// WP7平台
	PFCC_WIN8,							// WIN8平台
};

enum NoticeClassifiedCodeEnum
{
	NCCE_OTHER = 0,						// 其它归类
	NCCE_IPHONE,
	NCCE_ANDROID,
	NCCE_WP7,
	NCCE_IPAD,
	NCCE_GPAD,
};

// 客户端数据头定义
typedef struct sub_head
{
	unsigned short	sub_type;
	unsigned short  sub_attrs;
	unsigned short  sub_length;
	unsigned int	sub_extend;
};
// -------------------------           /

// 子数据类型sub_type定义
enum SubType {
	ST_ADD = 300,					// 添加记录
	ST_DEL = 301,					// 删除记录
	ST_HIS = 302,					// 历史记录
	ST_LATEST = 303,				// 当前预警记录
	ST_PUSH = 304,					// 推送的预警消息
	ST_KEEP = 305,					// 心跳
	ST_TOKEN = 306,					// 上传令牌
	ST_RECGOT = 307,				// 获取当前设置
	ST_EDIT = 308,					// 编辑预警记录
	ST_REVERIFY = 309,				// 要求重新验证
	ST_PUSH_PUBLIC = 310,			// 推送公共消息
	ST_PUSH_MINE = 311,				// 推送信息地雷
	ST_PUBLIC_HIS = 312,			// 获取公共消息历史信息
	ST_ERROR = 313,					// 返回错误
	ST_REQ_NOTICE = 314,			// 请求最新的公告消息
	ST_UPD_SELF_STOCK = 315,		// 上传自选股
	ST_SELF_STOCK_RECVMARK = 316,	// 自选股信息地雷回应
	ST_GET_NOWARNING_MG = 317,		// android非预警用户请求最新的消息
	ST_GET_INFOMINE_COUNT = 318,	// 请求信息地雷的数目
	
	ST_GET_LASTEST_USER_DATA = 1000,// 请求用户的最新数据
};

enum
{
	HQTYPE_STATIC	=	64000,
	HQTYPE_DYNA		=	64001,
	HQTYPE_SIMPLEDYNA	=	64002,
	HQTYPE_FUTUREDYNA	=	64003,
	HQTYPE_ORIGINSTATIC	=	64005,
};

//数据请求头
struct ACC_JAVA_HEAD
{
	unsigned char	cSparate;//分隔符号'{', ':'等, 除了0和‘}’'H' tag
	unsigned short	type;
	unsigned short  attrs;//0
	unsigned short  length;
};
//数据返回头
struct ACC_JAVA_HEAD32
{
	unsigned char	cSparate;//分隔符号'{', ':'等, 除了0和‘}’'H' tag
	unsigned short	type;
	unsigned short  attrs;//0x0008
	unsigned int    length;
};

//静态数据 HQTYPE_STATIC, 请求 sizeof(ACC_STATICASKHEAD)
struct ACC_STATICASKHEAD
{
	WORD	m_nMarket;
	DWORD	m_nDate;
	DWORD	m_nCrc;
};
// -------------------------           /

//响应sizeof(ACC_STATICASKHEAD)+sizeof(ACC_STK_STATIC)*N
struct ACC_STK_STATIC
{
	WORD	m_wMarket;					//详见MARKET_TYPE
	char	m_szLabel[MAX_STKCODE_LEN];	//代码
	char	m_szName[MAX_STKNAME];		//名称
	BYTE	m_cType;					//STK_TYPE
	BYTE	m_nPriceDigit;				//价格最小分辨率，非常重要，每一个DWORD类型的价格都要除以10^m_nPriceDigit才是真正的价格
	short	m_nVolUnit;					//成交量单位，每一成交量单位表示多少股
	MWORD	m_mFloatIssued;				//流通股本
	MWORD	m_mTotalIssued;				//总股本
	
	DWORD	m_dwLastClose;				//昨收
	DWORD	m_dwAdvStop;				//涨停
	DWORD	m_dwDecStop;				//跌停
};
// -------------------------           /

//HQTYPE_ORIGINSTATIC请求和HQTYPE_STATIC一样;响应sizeof(ACC_STATICASKHEAD)+sizeof(ACC_ORIGINSTK_STATIC)*N
struct ACC_ORIGINSTK_STATIC
{
	enum STK_TYPE
	{
		INDEX = 0,				//指数
		STOCK = 1,				//股票
		FUND = 2,				//基金
		BOND = 3,				//债券
		OTHER_STOCK = 4,		//其它股票
		OPTION = 5,				//选择权
		EXCHANGE = 6,			//外汇
		FUTURE = 7,				//期货
		FTR_IDX = 8,			//期指
		RGZ = 9,				//认购证
		ETF = 10,				//ETF
		LOF = 11,				//LOF
		COV_BOND = 12,			//可转债
		TRUST = 13,				//信托
		WARRANT = 14,			//权证
		REPO = 15,				//回购
		COMM = 16,				//商品现货
	};
	WORD	m_wStkID;						//本市场内唯一标示,在本市场内的序号
	char	m_strLabel[MAX_STKCODE_LEN];	//代码

	char	m_strName[MAX_STKNAME];			//名称
	BYTE	m_cType;						//STK_TYPE
	BYTE	m_nPriceDigit;					//价格最小分辨率，非常重要，每一个DWORD类型的价格都要除以10^m_nPriceDigit才是真正的价格
	short	m_nVolUnit;						//成交量单位，每一成交量单位表示多少股
	MWORD	m_mFloatIssued;					//流通股本
	MWORD	m_mTotalIssued;					//总股本
	
	DWORD	m_dwLastClose;					//昨收
	DWORD	m_dwAdvStop;					//涨停
	DWORD	m_dwDecStop;					//跌停
};
// -------------------------           /

//HQTYPE_DYNA 动态行情请求sizeof(WORD),表示市场代码;返回sizeof(ACC_STK_DYNA)*N
struct ACC_STK_DYNA
{
	WORD	m_wStkID;				//股票ID
	DWORD	m_time;					//成交时间, 32位
	DWORD	m_dwOpen;				//开盘
	DWORD	m_dwHigh;				//最高
	DWORD	m_dwLow;				//最低
	DWORD	m_dwNew;				//最新
	MWORD	m_mVolume;				//成交量
	MWORD	m_mAmount;				//成交额
	MWORD	m_mInnerVol;			//内盘成交量,<0表示该笔成交为主动卖，>=0表示主动买,绝对值表示内盘成交量
	DWORD	m_dwTickCount;			//累计成交笔数
	DWORD	m_dwBuyPrice[5];		//委买价格
	DWORD	m_dwBuyVol[5];			//委买量
	DWORD	m_dwSellPrice[5];		//委卖价格
	DWORD	m_dwSellVol[5];			//委卖量
	DWORD	m_dwOpenInterest;		//持仓量(期货期指特有)
	DWORD	m_dwSettlePrice;		//结算价(期货期指现货特有)
};

// 推送消息类型定义
enum PushTypeEnum
{
	PTE_EWARNING = 0,				// 股价预警
	PTE_INFOMINE,					// 信息地雷
	PTE_NOTICE						// 公告信息
};

// 推送公告类型定义
enum PublicNoticeTypeEnum
{
	NOTICE_NORMAL = 0,				// 普通公告
	NOTICE_EXTENSION,				// 活动公告
};

// 信息地雷类型
enum InfoMineTypeEnum
{
	INFOMINE_NORMAL = 0,			// 普通信息地雷
	INFOMINE_STATISTIC,				// 统计信息地雷
};

enum UploadSelfStkOperTypeEnum
{
	USSOT_ADD = 0,					// 添加
	USSOT_REMOVE,					// 删除
	USSOT_OVERLAP					// 覆盖
};


#pragma pack()

#ifdef __cplusplus
}
#endif

#endif  /* _DATA_TYPE_H */
