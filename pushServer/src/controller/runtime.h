#ifndef _CONTROL_RUNTIME_H
#define _CONTROL_RUNTIME_H

#include <map>

// 常量定义-----
const int DEFAULT_USE_LEN = 128;							// 默认长度
const int AppLogInValidLen = 8;								// 应用服务连接验证长度
const int MaxMarketCount = 50;								// 市场分类的最大数
const int MarketCodeLen = 2;								// 市场简码长度
//		-------------------/

// 系统状态定义
enum SystemStatus{
	STOP,
	RUNNING
};

// 日志控制
typedef struct LogControlTag
{
	char LogLevel[10];										// 日志级别
	int LogRoute;											// 日志输出介质
};

// 系统服务配置定义
typedef struct ServerTag {
	char AppLogInValid[AppLogInValidLen + 1];				// 应用服务器登录通用服务器验证
	int CalThreadNum; 										// 计算线程总数
	int MaxLinkToHq; 										// 最大行情连接数
	int ListenPort;											// 监听端口
	int StartInitialTime;									// 启动初始化时间
	int EndInitialTime;										// 终止初始化时间
	unsigned short ServerCodeNum;							// 推送服务器的编码
};

// 数据服务器
typedef struct DataServerTag {
	int Port;												// 服务器端口
	char Addr[25];											// 服务器地址
	int MaxRecordNum;										// 总记录最大数
	char MarketInclude[MaxMarketCount * MarketCodeLen + 1];	// 支持的市场编码列表
};

// 用户配置
typedef struct PlatformUserConfig 
{
	unsigned short PlatformClassifiedCode;					// 平台代码归类代码
	int MaxUser;											// 最大用户数
	char UserFileName[DEFAULT_USE_LEN];						// 用户信息文件名称
	int MaxUserWithEWarn;									// 最大设置预警的用户数(不应该超过MaxUser)
	char MsgHisFileName[DEFAULT_USE_LEN];					// 历史记录文件名称
	unsigned char MaxEWarningHisNum;						// 历史记录保留的最大预警记录数
	unsigned char MaxInfoHisNum;							// 历史记录文件保留的最大信息地雷数
	unsigned char MaxNoticeHisNum;							// 历史记录文件保留的最大公告数
	char EWarningFileName[DEFAULT_USE_LEN];					// 预警记录文件名称
	unsigned char MaxEWarnPerUser;							// 每个用户的最大预警记录数
	unsigned char DefaultEWarnPerUser;						// 每个用户的默认预警记录数
	
	PlatformUserConfig& operator= (const PlatformUserConfig& item);
};

// 股价预警数据库记录
typedef struct MySqlDbConfig {
	char DbName[DEFAULT_USE_LEN];							// 数据库名称
	char LogUser[DEFAULT_USE_LEN];							// 登陆用户名
	char LogPwd[DEFAULT_USE_LEN];							// 登陆密码
};

// 信息地雷数据库记录
typedef struct WapInfoMineConfig {
	char DSN[DEFAULT_USE_LEN];								// 数据库名称
	char LogUser[DEFAULT_USE_LEN];							// 登陆用户名
	char LogPwd[DEFAULT_USE_LEN];							// 登陆密码
};

// 公共消息数据库记录
typedef struct WapPublicNewsConfig {
	char DSN[DEFAULT_USE_LEN];								// 数据库名称
	char LogUser[DEFAULT_USE_LEN];							// 登陆用户名
	char LogPwd[DEFAULT_USE_LEN];							// 登陆密码
};

// 外部推送服务器设置
typedef struct OuterPushServ {
	int ConnectNum;											// 连接数
	char HostAdd[DEFAULT_USE_LEN];							// 地址
	int HostPort;											// 端口
	char CertFile[DEFAULT_USE_LEN];							// 证书文件
	char KeyFile[DEFAULT_USE_LEN];							// 秘钥文件
	unsigned short PlatformCode;							// 平台代码
	
	OuterPushServ& operator = (const OuterPushServ &item);
};

// 节点器配置
typedef struct UserNodeServerT {
	int CheckStart;											// 检查用户节点状态起始时刻
	int CheckEnd;											// 检查用户节点状态终止时刻
	int SUserDeadTime;										// 短用户死亡时间(单位:分钟 超过此界限标识为死亡)
	int LUserDeadTime;										// 判定长间隔用户死亡时间(单位:分钟 超过此界限标识为死亡)
};

typedef struct RuntimeSetting
{
	int RunStartTime;										// 运行开始
	int RunEndTime;											// 运行终止
	int ScanInterval;										// 运行间隔
	int ProcessUnit;										// 单次处理单位
	int GetLastestInterval;									// 取最新数据时间间隔
	int RemainValidTime;									// 消息最长有效期(单位天)
};

// 推送运行配置
typedef struct PushRuntimeConfigT
{
	UserNodeServerT UserNodeServer;							// 节点配置
	RuntimeSetting EWarningRuntime;							// 预警运行配置
	RuntimeSetting InfoMineRuntime;							// 信息地雷运行配置
	RuntimeSetting NoticeRuntime;							// 公告运行配置
};

typedef struct PlatformClassifiedT
{
	unsigned char PlatformClassifiedCode;					// 平台归类代码
	unsigned short NoticeClassifiedCode;					// 公告归类代码
	
	PlatformClassifiedT();
	PlatformClassifiedT(const unsigned char, const unsigned short);
	PlatformClassifiedT& operator=(const PlatformClassifiedT&);
};

// 自选股信息地雷
typedef struct SelfSelectStockInfomineConfigT
{
	int CacheCount;											// 自选股记录的最大数
	char FileName[DEFAULT_USE_LEN];							// 记录文件的名字
	short StatisticsTimeCount;								// 统计信息地雷个数的时段的个数
	int *StatisticsTime;									// 统计信息地雷个数的时段
	short ExcludeCodeCount;									// 排除统计的证券代码个数
	char *ExcludeCode;										// 排除统计的证券代码，新添加以"|"分割
	time_t LastestUpdStatistics;							// 执行信息地雷统计的时刻
	unsigned char InfoMineStatisTimes;						// 执行信息统计的次数
};

const char* GetSystemName();                                /* 返回系统名称 */
void        SetSystemName(const char*);                     /* 设置系统名称 */

int     GetMainProcessId();                                 /* 返回应用系统主进程号 */
void    SetMainProcessId(int);                              /* 设置应用系统主进程号 */
void    ResetMainProcessId();                               /* 重置应用系统主进程号 */
bool    IsRunning();                                        /* 返回应用系统是否已启动 */
bool    IsShutdown();                                       /* 返回应用系统是否已进入Shutdown状态 */
const enum SystemStatus     GetSystemStatus();              /* 返回系统运行状态 */
void    SetSystemStatus(const enum SystemStatus);           /* 设置系统运行状态 */
void UpdateNowTime();										// 更新当前的系统时间
const time_t GetNowTime();									// 获取当前系统时间
const unsigned int ChangeToUINT(const float,				// 浮点型放大为无符号整形
		const unsigned char);
const int ChangeToINT(const float,							// 浮点型放大为整形
		const unsigned char);

void SetServerConfig(const ServerTag*);						// 设置Server配置
const int GetInitialStartTime();							// 获取启动初始化时间
const int GetInitialEndTime();								// 获取结束初始化时间
const int GetMaxLinkedHq(void);								/* 获取处理连接数 */
const int GetCalcThreadNum(void);							/* 获取计算线程总数 */
const int GetListenPort();									// 获取监听端口 
const int GetMaxUserProc();									// 获取最大处理用户数
const int GetTotoalThreadNum();								// 获取需要启动的全部线程总数
const int GetMaxHqLinkPerThread();							// 获取每个线程可以处理的最大Hq链接数
const int GetMaxUserPerHqLinkNode();						// 获取每个连接节点可以处理的最大用户数
const int GetServerCodeNum();								// 获取推送服务器的编码
const char* GetAppLogInValid();								// 获取登录验证码

const short DistributeUserThread(const unsigned int);		// 分发计算线程

void SetLogControl(const LogControlTag*);					// 设置日志控制
const LogControlTag* GetLogControl();						// 获取日志控制

void SetDataServerTag(const DataServerTag*);				// 设置数据服务器地址
const struct DataServerTag* GetDataServer(bool exchange);	// 获取数据服务器
void SetDataServerTag2(const DataServerTag*);				// 设置数据服务器地址

void SetDbSetting(const MySqlDbConfig*);					// 设置股价预警数据配置项
const MySqlDbConfig* GetDbSetting();						// 获取股价预警数据配置项

void SetInfoMineSetting(const WapInfoMineConfig*);			// 设置信息地雷数据配置项
const WapInfoMineConfig* GetInfoMineSetting();				// 获取信息地雷数据配置项

void SetPublicNewsSetting(const WapPublicNewsConfig*);		// 设置公共消息数据配置项
const WapPublicNewsConfig* GetPublicNewsSetting();			// 获取公共消息数据配置项

void SetOuterPushServ(const OuterPushServ &);				// 设置推送选项
const OuterPushServ* GetOuterPushServ(const unsigned short);// 获取推送选项
std::map<unsigned short, OuterPushServ>& 					// 获取选项容器
	GetOuterPushServMap();

void SetUserNodeServer(const UserNodeServerT*);				// 设置节点配置项
const UserNodeServerT* GetUserNodeServer();					// 获取节点配置项

void SetEWarningRuntime(RuntimeSetting*);					// 设置预警运行配置
const RuntimeSetting* GetEWarningRuntime();					// 获取预警运行配置

void SetInfoMineRuntime(RuntimeSetting*);					// 设置信息地雷运行配置
const RuntimeSetting* GetInfoMineRuntime();					// 获取信息地雷运行配置

void SetNoticeRuntime(RuntimeSetting*);						// 设置公告运行配置
const RuntimeSetting* GetNoticeRuntime();					// 获取公告运行配置

void SetUserConfig(const PlatformUserConfig&);				// 设置平台用户配置
const PlatformUserConfig* GetAndroidUserConfig();			// 获取android平台的用户配置
const PlatformUserConfig* GetIosUserConfig();				// 获取Ios平台的用户配置
const PlatformUserConfig* GetWp7UserConfig();				// 获取Wp7平台的用户配置
const PlatformUserConfig* GetWin8UserConfig();				// 获取Win8平台的用户配置

void SetPlatformClassifiedMap(const unsigned short,			// 设置平台归类
		const PlatformClassifiedT&);
const unsigned short GetPlatformClassifiedMap(				// 获取平台归类代码
		const unsigned short);
const unsigned short GetPlatformNoticeClassifiedMap(		// 获取平台的公告归类代码
		const unsigned short);
		
int OperSyncUserInterval(bool, unsigned int&); 				// 操作同步用户信息判断时间间隔，测试用

// 获取自选股信息地雷相关设置		
SelfSelectStockInfomineConfigT* GetSelfSelectStockInfomineConfig();



#endif  /* _CONTROL_RUNTIME_H */
