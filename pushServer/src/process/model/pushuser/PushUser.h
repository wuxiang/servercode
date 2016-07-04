#ifndef _INCLUDE_PUSH_USER_H
#define _INCLUDE_PUSH_USER_H

#include "../../data_type.h"
#include "../../config.h"
#include <string>
#ifdef _MSC_VER
#include <Winsock2.h>
#else
#include <time.h>
#endif

class CBuffWriter;
class CEarlyWarningConditionRecord;
class CPlatformEWarnManage;
struct sub_head;
struct PriceAlarm;
class LargeMemoryCache;
struct PushMsg;
struct Payload;
struct WPmessage;
struct PNmessage;
struct IMmessage;

// 推送用户属性标识定义
enum PushUserPropertyFlag
{
	// 0 - 2位 标识状态
	PU_ACTIVE =						(int) 0x00000,					// 存活状态
	PU_INACTIVE =					(int) 0x00001,					// 非活动状态
	PU_RELIVE =						(int) 0x00002,					// 重新激活(从非存活状态到存活状态)
	PU_DEAD =						(int) 0x00003,					// 死亡(认为用户已经不再存在)
	
	// 3位 标识是否接收推送消息
	PU_NO_RECEIVE_PUSH_MSG =		(int) 0x00000,					// 不接收推送消息
	PU_RECEIVE_PUSH_MSG =			(int) 0x00008,					// 接收推送消息
	
	// 4位 标识是否设置了有效的自选股预警
	PU_NO_VALID_EARLY_WARNING_SET =	(int) 0x00000,					// 没有设置自选预警信息
	PU_VALID_EARLY_WARNING_SET = 	(int) 0x00010,					// 设置了有效的自选预警信息
	
	// 5 - 6位 定义CPushUser同步库属性字段
	PU_DB_ADD =	0x0020,												// 添加记录
	PU_DB_UPDATE =	0x0040,											// 更新记录(若前一状态是添加，则应只更新记录，不更新状态)
	PU_DB_DEL =	0x0060,												// 删除记录
	PU_DB_NULL = 0x0000,											// 置空
};

#pragma pack(1)

// 用户登录信息
typedef struct UserLogInfor
{
	char m_strUserID[MAX_USER_ID_LEN + 1];							// 用户标识ID
	char m_strUserName[MAX_USER_NAME_LEN + 1];						// 注册用户名
	char m_strUserPwd[MAX_USER_PWD_LEN + 1];						// 注册用户密码
	unsigned short m_uLocalVersion;									// 用户所用软件的当前版本
	unsigned char m_cPlatformCode;									// 所属平台代码
	char m_strTelNum[MAX_TEL_NUMBER_LEN + 1];						// 手机号码
	char m_strTelID[MAX_TEL_ID_LEN + 1];							// 手机ID
	
public:
	UserLogInfor();
	UserLogInfor(const char*, const char*, const char*,
		const char*, unsigned char, const char*, const char*);
	~UserLogInfor();
};

////////用户基本信息头
struct  CUserBaseInfoHead
{
	TIME_T32	m_tDBSynTime;										//和库最后同步时间
	unsigned int m_nSpace;											//用户信息最大空间
	unsigned int m_nOccupyNum;										//已经使用的用户信息数(单向增加，不移动用户的位置)

	unsigned int m_nHisSpace;										//每个用户历史信息空间数，即每个用户PushMsg的空间数目

	unsigned int m_nEarlyWarningSpace;								//预警信息的空间数
	unsigned int m_nUseEarlyWarning;								//已经使用的预警信息的空间数(单向增加，不移动记录)
	unsigned int m_nEarlyWarningPerUser;							//每个用户预警信息的空间数
};

class CPushUser
{
public:
	CPushUser(void);
	CPushUser(const UserLogInfor*);
	// 析构函数不用虚函数，继承类主动调用
	~CPushUser();
	friend class HqLinkNode;
	friend class CAndroidPushUserManage;
	friend class CIosPushUserManage;
	friend class CWp7PushUserManage;
	friend class CWin8PushUserManage;
	friend class DatabaseSnapshot;
	friend class CPushUserManage;

// 用户静态信息
protected:
	char m_strUserID[MAX_USER_ID_LEN + 1];							// 用户标识ID
	char m_strUserName[MAX_USER_NAME_LEN + 1];						// 注册用户名
	char m_strUserPwd[MAX_USER_PWD_LEN + 1];						// 注册用户密码
	unsigned short m_uLocalVersion;									// 用户所用软件的当前版本
	unsigned char m_cPlatformCode;									// 所属平台代码
	char m_strTelNum[MAX_TEL_NUMBER_LEN + 1];						// 手机号码
	unsigned short m_nPushServId;									// 推送服务器id
	
	unsigned short m_iUserProperty;									// 用户属性定义
	TIME_T32 m_uRegTime;											// 用户注册时间
	
// 用户动态信息
protected:
	TIME_T32 m_tLastestAliveTime;									// 最近一次活动时间
	unsigned int m_uExpandInfoValue;								// 行情的扩展信息
	unsigned short m_uUserBasicCalc;								// 用户基本标识信息(0~11位标识所属行情编码 12~15标识所属线程编码)
	
	// 预警记录
	unsigned int m_uRecordSetMarkID;								// 预警记录标识ID，指向CEarlyWarningConditionRecord的位置， -1表示未设置预警
	unsigned int m_iPushWarningTotal;								// 已经生成的预警信息数目
	unsigned int m_iPushWarningNum;									// 已经发送的预警信息数目
	unsigned int m_iPushWarningDBNum;								// 已经存储的预警信息数目
	unsigned int m_iRebackWarningNum;								// 回应的已经收到的预警数目
	
	// 信息地雷
	unsigned int m_iInfoMineTotal;									// 已经生存的信息地雷总数
	unsigned int m_iInfoMineNum;									// 已经发送的信息地雷数目
	unsigned int m_iInfoMineDBNum;									// 已经同步的信息地雷数目
	
	// 公告
	unsigned int m_iPublicNoticeTotal;								// 已经生存的公共消息总数
	unsigned int m_iPublicNoticeNum;								// 已经发送的公共消息数目
	unsigned int m_iPublicNoticeDBNum;								// 已经同步的公共消息数目
	
// 预警条件管理
protected:
	const unsigned char GetCurrentUserPermitWarningNum(				// 获取当前用户的最大允许预警数目
			const CPlatformEWarnManage*)const;
	const int AddEarlyWarningRecord(								// 添加预警记录
			CEarlyWarningConditionRecord&);
	const int RemoveEarlyWarningRecord(								// 移除指定ID的预警记录
			const unsigned int);			
	const int EditEarlyWarningRecord(					 			// 编辑指定ID的预警记录
			const unsigned int, CEarlyWarningConditionRecord*);	
			
	int GetEarlyWarningMsgID();										// 生成预警记录的序列号
	void SetEarlyWarningMsgID(const unsigned int);					// 设置预警记录ID
	bool GenerateEarlyWarningMsg(const int,							// 形成推送记录
						const struct PriceAlarm *, const unsigned char,
						CEarlyWarningConditionRecord *,	PushMsg *);
	const char* ChanageToPartEarlyMsg(const char*, const PushMsg*,	// 转换为字符串记录形式
						char*, const unsigned int,
						const unsigned char);
	const char* ChangeToTotalEarlyMsg(const char*, const PushMsg*,	// 转换为记录全格式
						char*, const unsigned int,
						const std::string &,const std::string &,
						const unsigned char);
	const char* GetEalyMsgFormat(const PushMsg*,					// 获取预警记录Format
						const unsigned char, char *,
						const unsigned int, const int type = 0);
	int GetEWarningAndInfoMineHisMsg(const unsigned int,			// 获取区间预警及信息地雷历史推送消息
						const unsigned int, const unsigned int,
						CBuffWriter *writer);
	int GetEWarningRangeHisMsg(const unsigned int,		 			// 获取区间预警历史推送消息
						const unsigned int,const unsigned int,
						const unsigned int, CBuffWriter *writer);
	const char* GetEarlyWarningFormat(const int,					// 获取预警记录格式
						const unsigned char, char *,
						const unsigned int, const int type = 0);
	const int ClearWarnRecord();									// 清空预警记录设置
	const int WriteHisRecord(const unsigned int, const char*, 		// 写入历史消息
						const char*, const char*, CBuffWriter*);
	
// 信息地雷管理						
protected:
	int GetInfoMineMsgID();											// 生成信息地雷记录的序列号
	void SetInfoMineMsgID(const unsigned int);						// 设置信息地雷记录ID
	bool GenerateInfoMineMsg(const char *,							// 形成信息地雷记录
						IMmessage*, PushMsg *);
	int GetInfoMineRangeHisMsg(const unsigned int,  				// 获取区间信息地雷推送历史消息
						const unsigned int, const unsigned int,
						const unsigned int, CBuffWriter*);
	const char* ChangeToInfoMineMsg(const IMmessage*,				// 转化为信息地雷的字符串形式
						char*, const unsigned int);
	bool GenerateInfoMineStatisMsg(const unsigned int,				// 形成统计信息地雷记录
						PushMsg*);
	
// 公告管理
protected:
	int GetNoticeMsgID();											// 生成公告记录的序列号
	void SetNoticeMsgID(const unsigned int);						// 设置公告记录ID
	bool GenerateNoticeMsg(PNmessage*, PushMsg*);					// 形成公告信息记录
	int GetPublicNoticeRangeHisMsg(const unsigned int,  			// 获取区间公告历史推送消息
						const unsigned int ,
						const unsigned int, CBuffWriter*);
	const char* ChangeToPublicMsg(const PNmessage*,					// 转化为公共推送过的字符串形式
						char*, const unsigned int);

// 
protected:
	char* GetGivenTimeStamp(const time_t, const char*,				// 返回指定时间戳
						char*, const unsigned int);
	bool PushBackMsg(const unsigned int, PushMsg&);					// 记录消息

// 用户请求
protected:
	int CheckUserLogValid(const UserLogInfor*);						// 检查用户的登录信息合法性
	int GetTotalEWarnRecords(CBuffWriter*);							// 获取全部预警记录
	int GetEWarnRecordsStatus(const int, CBuffWriter*);				// 获取全部预警记录的状态
	int ParseReq300(const sub_head *, const UserLogInfor*, 			// 处理300请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq301(const sub_head *, const UserLogInfor*, 			// 处理301请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq302(const sub_head *, const UserLogInfor*, 			// 处理302请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq303(const sub_head *, const UserLogInfor*, 			// 处理303请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq305(const sub_head *, const UserLogInfor*, 			// 处理305请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq307(const sub_head *, const UserLogInfor*, 			// 处理307请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq308(const sub_head *, const UserLogInfor*, 			// 处理308请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReqUndefined(const sub_head *, void *data,				// 处理未实现的请求
					const unsigned int, CBuffWriter*);
	int ParseReq312(const sub_head *, const UserLogInfor*, 			// 处理312请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq315(const sub_head *, const UserLogInfor*, 			// 处理315请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq316(const sub_head *, const UserLogInfor*, 			// 处理316请求
					void *data, const unsigned int, CBuffWriter*);
	int ParseReq318(const sub_head *, const UserLogInfor*, 			// 处理318请求
					void *data, const unsigned int, CBuffWriter*);

// 用户设置
protected:
	void SetUserActiveProperty(enum PushUserPropertyFlag);			// 设置用户存活状态属性
	void SetHaveSetPrivateWarningProperty(enum PushUserPropertyFlag);// 设置否设置了自选股预警信息
	void SetAllowPushMsgProperty(enum PushUserPropertyFlag);		// 设置否允许接收推送消息
	void SetOperDbProperty(enum PushUserPropertyFlag);				// 设置新的操作数据库的方式
	void UpdateLastestAliveTime(const sub_head *);					// 更新节点最新活动时间
	void UpdateLastestAliveTime(const time_t);						// 更新节点最新活动时间
	const int ShortItvlCheckUserAlive(const time_t);				// 检查短用户节点的存活状态
	const int LongItvlCheckUserAlive(const time_t);					// 检查长用户节点的存活状态		
	bool IsLocalUser();												// 判断用户是否为本服务器本地的用户
	void UpdatePushUserExpandInfo(const unsigned int);				// 更新推送用户扩展信息
	int ProcessUserRequest(const sub_head*, const UserLogInfor*,	// 处理用户请求(sub_head后面除去用户信息的数据)
						void*, const unsigned int, CBuffWriter*);
	void SetUserThreadNum(const unsigned short);					// 设置用户线程编号
	void SetUserHqLinkIndex(const unsigned short);					// 设置用户所属行情编码
	void SetUserServerCodeNum(const unsigned short);				// 设置用户预警服务器编码
	const unsigned short GetUserServerCodeNum()const;				// 获取用户预警服务器编码
	bool CanContinueExeScan(const unsigned char);					// 是否还可以继续执行扫描
	bool HaveDataSend(const unsigned char);							// 是否有数据需要发送
	void SetUserLocalVersion(unsigned short);						// 更新用户版本
	
	const int ReloadEWarningFromDb();								// 从数据库中重新加载预警记录设置
	const int ReloadPushHistoryFromDb(const unsigned int);			// 从数据库中重新加载已经发生的历史
	const int ReloadTypePushHistoryFromDb(const unsigned int,		// 从数据库中重新加载已经发生的历史
					const unsigned char, const unsigned int,
					const unsigned int);
	void DeleteTypeOutofDateHisMsg(const unsigned char,				// 从数据库中移除无效的历史记录信息
					const unsigned int);
	void RemoveFromCalcQueue(const unsigned int);					// 从计算队列当中移除
	const char* GetIndexUserTiltleSeting(const int);				// 获取特定缩影的title

// 自选股管理
protected:
	int UploadSelfSelectStk(const unsigned int, 					// 用户上传自选股
					const unsigned char, const unsigned char,
					const char**, const int, const int);
	int TryLoadUserSelfStkList(const unsigned int);					// 读取用户的自选股
	const int ProcessSelfStkInfoMineRT(const unsigned int);			// 执行自选股信息地雷的实时推送
	const char* GetSeflSkkStatisFormat();							// 获取自选股统计格式字符控制
	
// 公有函数
public:
	const unsigned short GetActiveProperty()const;					// 获取用户存活状态
	const unsigned short GetHaveSetPrivateWarningProperty()const;	// 获取是否设置了自选股预警信息
	const unsigned short GetAllowPushMsgProperty()const;			// 获取是否允许接收推送消息
	const unsigned short GetOperDbProperty()const;					// 获取最新的操作数据库的方式
	bool IsDead()const;												// 获取是否已经无效
	
	const char* GetUserID()const;									// 获取用户标识
	const unsigned short GetUserHqLinkIndex();						// 获取用户所属行情编号
	const unsigned short GetUserThreadNum();						// 获取用户的线程编号	
	const unsigned char GetUserPlatform()const;						// 获取当前用户平台定义
	const unsigned short GetUserLocalVersion()const;				// 获取用户本地软件版本
	const TIME_T32 GetUserRegTime()const;							// 获取用户的注册时间
	
	int ResetAllEarlyWarningRecordState();							// 重置所有预警节点记录的状态
	int ResetEarlyWarningRecordLastestStkInfo();					// 重置预警节点最新状态
	void UpdateUserRegTime(const time_t);							// 更新用户的注册时间
	void UpdateBelongCalcThreadProperty(const unsigned int);		// 更新用户所属的计算线程属性
	
	const int SyncLatestToDb(const unsigned int,					// 最新同步到数据库
					const char *);
	void DeleteOutofDateHisMsg();									// 清除失效的历史记录信息
	int ResetSelfSelectStkState(const unsigned int);				// 重置自选股记录的状态
};

#pragma pack()

#endif	/* _INCLUDE_PUSH_USER_H */
