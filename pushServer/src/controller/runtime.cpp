#include "runtime.h"
#include "../util/util.h"
#include "../util/string/string_util.h"
#include "../util/log/log.h"
#include "../util/time/time_util.h"
#include "../process/config.h"
#include "../process/data_type.h"

using namespace std;

// 系统runtime信息定义
typedef struct RuntimeInfo{
	volatile enum SystemStatus status;								// 系统运行状态
	volatile int MainProcessID;										// 主控线程ID
	char MainAppName[100];											// 应用程序名称
	                                                       	
	ServerTag SerConfig;											// 服务配置
	
	DataServerTag DataServer;										// 行情服务器信息
	DataServerTag DataServer2;										// 备用行情服务器信息
	
	LogControlTag LogControl;										// 日志控制
	
	MySqlDbConfig DbConfig;											// 股价预警数据库设置
	WapInfoMineConfig IMConfig;										// 信息地雷数据库设置
	WapPublicNewsConfig PNConfig;									// 公共消息数据库设置
	
	PushRuntimeConfigT PushRuntimeConfig;							// 运行配置	
	map<unsigned short, OuterPushServ> lOuterPushServ;				// 推送设置	
	map<unsigned short, PlatformUserConfig> lPlatformUserConfig; 	// 平台用户配置
	map<unsigned short, PlatformClassifiedT> lPlatformClsMap;		// 平台归类映射
	
	SelfSelectStockInfomineConfigT SelfSelectStockInfomineConfig;	// 自选股信息地雷设置
	
	RuntimeInfo()
	{
		status = STOP;
		MainProcessID = -1;
		bzero(MainAppName, 100);
		bzero(&SerConfig, sizeof(ServerTag));
		bzero(&DataServer, sizeof(DataServerTag));
		bzero(&DataServer2, sizeof(DataServerTag));
		bzero(&LogControl, sizeof(LogControlTag));
		bzero(&DbConfig, sizeof(MySqlDbConfig));
		bzero(&IMConfig, sizeof(WapInfoMineConfig));
		bzero(&PNConfig, sizeof(WapPublicNewsConfig));
		bzero(&PushRuntimeConfig, sizeof(PushRuntimeConfigT));
		lPlatformUserConfig.clear();
		lOuterPushServ.clear();
		lPlatformClsMap.clear();
		bzero(&SelfSelectStockInfomineConfig, sizeof(SelfSelectStockInfomineConfigT));
	}
	
	~RuntimeInfo()
	{
		lPlatformUserConfig.clear();
		lOuterPushServ.clear();
		lPlatformClsMap.clear();
		if (NULL != SelfSelectStockInfomineConfig.StatisticsTime)
		{
			SelfSelectStockInfomineConfig.StatisticsTimeCount > 1 ? delete []SelfSelectStockInfomineConfig.StatisticsTime : 
				delete SelfSelectStockInfomineConfig.StatisticsTime;
		}
		
		if(NULL != SelfSelectStockInfomineConfig.ExcludeCode)
		{
			delete []SelfSelectStockInfomineConfig.ExcludeCode;
		}		
	}
};

// 运行环境变量定义
static RuntimeInfo sRuntimeInfo;
static time_t sNowTime = 0;
static unsigned int sSyncUserInterval = 11;
///-----------------						/

OuterPushServ& 
OuterPushServ::operator = (const OuterPushServ &item)
{
	ConnectNum = item.ConnectNum;
	strncpy(HostAdd, item.HostAdd, DEFAULT_USE_LEN);
	HostPort = item.HostPort;
	strncpy(CertFile, item.CertFile, DEFAULT_USE_LEN);
	strncpy(KeyFile, item.KeyFile, DEFAULT_USE_LEN);
	PlatformCode = item.PlatformCode;
	return *this;
}

PlatformUserConfig& 
PlatformUserConfig::operator= (const PlatformUserConfig& item)
{
	memcpy(this, &item, sizeof(PlatformUserConfig));
	return *this;
}

PlatformClassifiedT::PlatformClassifiedT()
	: PlatformClassifiedCode(0),
	NoticeClassifiedCode(0)
{
}

PlatformClassifiedT::PlatformClassifiedT(const unsigned char cPlatformClassifiedCode,
	const unsigned short cNoticeClassifiedCode)
	: PlatformClassifiedCode(cPlatformClassifiedCode),
	NoticeClassifiedCode(cNoticeClassifiedCode)
{
}

PlatformClassifiedT& 
PlatformClassifiedT::operator=(const PlatformClassifiedT &item)
{
	PlatformClassifiedCode = item.PlatformClassifiedCode;
	NoticeClassifiedCode = item.NoticeClassifiedCode;
	return *this;
}

const char*
GetSystemName(){
	return sRuntimeInfo.MainAppName;
}

void
SetSystemName(const char *appName){
	if (NULL != appName && !IsEmptyString(appName)
		&& strlen(appName) < 100) {
		StrCopy(sRuntimeInfo.MainAppName, appName, strlen(appName));
		return;
	}
	ERROR("SetSystemName failed.");
}

int
GetMainProcessId() {
	return sRuntimeInfo.MainProcessID;
}

void
SetMainProcessId(int id) {
	if (id > 0) {
		sRuntimeInfo.MainProcessID = id;
		return;
	}
	FATAL("SetMainProcessId failed!");
}

void
ResetMainProcessId() {
	sRuntimeInfo.MainProcessID = -1;
}

bool
IsRunning() {
	return GetSystemStatus() == RUNNING;
}

bool
IsShutdown() {
	return GetSystemStatus() == STOP;
}

const enum SystemStatus
GetSystemStatus() {
	return sRuntimeInfo.status;
}

void
SetSystemStatus(const enum SystemStatus status) {
	sRuntimeInfo.status = status;
}

// 更新当前的系统时间
void 
UpdateNowTime()
{
	sNowTime = time(NULL);
}

// 获取当前系统时间
const time_t 
GetNowTime()
{
	return sNowTime;
}

// 浮点型放大为无符号整形
const unsigned int
ChangeToUINT(const float PreValue, const unsigned char Precision)
{
	return (unsigned int)(PreValue * powf(10, Precision) + 0.5);
}

// 浮点型放大为整形
const int
ChangeToINT(const float PreValue, const unsigned char Precision)
{
	if (PreValue < 0)
	{
		return (int)(PreValue * powf(10, Precision) - 0.5);
	}
	return (int)(PreValue * powf(10, Precision) + 0.5);
}

// 设置监听端口
void
SetServerConfig(const ServerTag *config) {
	sRuntimeInfo.SerConfig.CalThreadNum = config->CalThreadNum;
	sRuntimeInfo.SerConfig.MaxLinkToHq = config->MaxLinkToHq;
	sRuntimeInfo.SerConfig.ListenPort = config->ListenPort;
	sRuntimeInfo.SerConfig.StartInitialTime	= config->StartInitialTime;
	sRuntimeInfo.SerConfig.EndInitialTime = config->EndInitialTime;
	sRuntimeInfo.SerConfig.ServerCodeNum = config->ServerCodeNum;
	strncpy(sRuntimeInfo.SerConfig.AppLogInValid, config->AppLogInValid, AppLogInValidLen);
}

const int
GetMaxLinkedHq(void) {
	return sRuntimeInfo.SerConfig.MaxLinkToHq;
}

const int
GetCalcThreadNum(void) {
	return sRuntimeInfo.SerConfig.CalThreadNum;
}

// 获取最大处理用户数
const int
GetMaxUserProc() {
	return (GetAndroidUserConfig()->MaxUser 
		+ GetIosUserConfig()->MaxUser 
		+ GetWp7UserConfig()->MaxUser
		+ GetWin8UserConfig()->MaxUser);
}

// 获取监听端口
const int GetListenPort() {
	return sRuntimeInfo.SerConfig.ListenPort;
}

// 获取需要启动的全部线程总数
const int
GetTotoalThreadNum() {
	// 一个网络线程 一个动态请求行情数线程 一个请求新闻类线程 一个外部数据发送线程 计算线程
	return (4 + GetCalcThreadNum());
}

// 获取每个线程可以处理的最大Hq链接数
const int
GetMaxHqLinkPerThread()
{
	if (GetCalcThreadNum() <= 0)
	{
		return 60;
	}
	return GetMaxLinkedHq() / GetCalcThreadNum();
}

// 获取每个连接节点可以处理的最大用户数
const int GetMaxUserPerHqLinkNode()
{
	if (GetCalcThreadNum() <= 0 || GetMaxHqLinkPerThread() <= 0)
	{
		return 500;
	}

	return GetMaxUserProc() / GetCalcThreadNum() / GetMaxHqLinkPerThread();
}

// 获取启动初始化时间
const int
GetInitialStartTime()
{
	return sRuntimeInfo.SerConfig.StartInitialTime;
}

// 获取结束初始化时间
const int
GetInitialEndTime()
{
	return sRuntimeInfo.SerConfig.EndInitialTime;
}

// 设置数据服务器地址
void
SetDataServerTag(const DataServerTag *item)
{
	if (NULL == item)
	{
		return;
	}
	sRuntimeInfo.DataServer.Port = item->Port;
	if (NULL != item->Addr)
	{
		strncpy(sRuntimeInfo.DataServer.Addr, item->Addr, 25);
	}
	
	if (strlen(item->MarketInclude) > 0)
	{
		strncpy(sRuntimeInfo.DataServer.MarketInclude, item->MarketInclude, MaxMarketCount * MarketCodeLen);
	}
}

// 设置数据服务器地址
void
SetDataServerTag2(const DataServerTag *item)
{
	if (NULL == item)
	{
		return;
	}
	sRuntimeInfo.DataServer2.Port = item->Port;
	if (NULL != item->Addr)
	{
		strncpy(sRuntimeInfo.DataServer2.Addr, item->Addr, 25);
	}
	
	if (strlen(item->MarketInclude) > 0)
	{
		strncpy(sRuntimeInfo.DataServer2.MarketInclude, item->MarketInclude, MaxMarketCount * MarketCodeLen);
	}
}

// 获取数据服务器
const struct DataServerTag*
GetDataServer(bool exchange)
{
	static char iFlat = 0;
	if (exchange)
	{
		if (0 == iFlat)
		{
			iFlat++;
		}
		else
		{
			iFlat = 0;
		}
	}

	if (0 == iFlat)
	{
		return &sRuntimeInfo.DataServer;
	}
	else
	{
		return &sRuntimeInfo.DataServer2;
	}
}

// 获取推送服务器的编码
const int
GetServerCodeNum()
{
	return sRuntimeInfo.SerConfig.ServerCodeNum;
}

// 获取登录验证码
const char* 
GetAppLogInValid()
{
	return sRuntimeInfo.SerConfig.AppLogInValid;
}

// 分发计算线程
const short
DistributeUserThread(const unsigned int UserMapID)
{
	return UserMapID % GetCalcThreadNum();
}

// 设置日志控制
void
SetLogControl(const LogControlTag *item)
{
	memcpy(&sRuntimeInfo.LogControl, item, sizeof(LogControlTag));
}

// 获取日志控制
const LogControlTag*
GetLogControl()
{
	return &sRuntimeInfo.LogControl;
}

// 设置股价预警数据配置项
void
SetDbSetting(const MySqlDbConfig *item)
{
	memcpy(&sRuntimeInfo.DbConfig, item, sizeof(MySqlDbConfig));
}

// 获取股价预警数据配置项
const MySqlDbConfig*
GetDbSetting()
{
	return &sRuntimeInfo.DbConfig;
}

// 设置信息地雷数据配置项
void
SetInfoMineSetting(const WapInfoMineConfig *item)
{
	memcpy(&sRuntimeInfo.IMConfig, item, sizeof(WapInfoMineConfig));
}

// 获取信息地雷数据配置项
const WapInfoMineConfig*
GetInfoMineSetting()
{
	return &sRuntimeInfo.IMConfig;
}

// 设置公共消息数据配置项
void
SetPublicNewsSetting(const WapPublicNewsConfig *item)
{
	memcpy(&sRuntimeInfo.PNConfig, item, sizeof(WapPublicNewsConfig));
}

// 获取公共消息数据配置项
const WapPublicNewsConfig*
GetPublicNewsSetting()
{
	return &sRuntimeInfo.PNConfig;
}

// 设置推送选项
void
SetOuterPushServ(const OuterPushServ &item)
{
	sRuntimeInfo.lOuterPushServ.insert(pair<unsigned short, OuterPushServ>(item.PlatformCode,
		item));
}

// 获取推送选项
const OuterPushServ* 
GetOuterPushServ(const unsigned short PlatformCode)
{
	map<unsigned short, OuterPushServ>::iterator iterFind = 
		sRuntimeInfo.lOuterPushServ.find(PlatformCode);
	if (sRuntimeInfo.lOuterPushServ.end() == iterFind)
	{
		return NULL;
	}
	return &iterFind->second;
}

// 获取选项容器
std::map<unsigned short, OuterPushServ>& 
GetOuterPushServMap()
{
	map<unsigned short, OuterPushServ> &OutItem = sRuntimeInfo.lOuterPushServ;
	return OutItem;
}

// 设置节点配置项
void
SetUserNodeServer(const UserNodeServerT *item)
{
	memcpy(&sRuntimeInfo.PushRuntimeConfig.UserNodeServer, item, sizeof(UserNodeServerT));
}

// 获取节点配置项
const UserNodeServerT*
GetUserNodeServer()
{
	return &sRuntimeInfo.PushRuntimeConfig.UserNodeServer;
}

// 设置预警运行配置
void
SetEWarningRuntime(RuntimeSetting *item)
{
	memcpy(&sRuntimeInfo.PushRuntimeConfig.EWarningRuntime, item, sizeof(RuntimeSetting));
}

// 获取预警运行配置
const RuntimeSetting*
GetEWarningRuntime()
{
	return &sRuntimeInfo.PushRuntimeConfig.EWarningRuntime;
}

// 设置信息地雷运行配置
void
SetInfoMineRuntime(RuntimeSetting *item)
{
	memcpy(&sRuntimeInfo.PushRuntimeConfig.InfoMineRuntime, item, sizeof(RuntimeSetting));
}

// 获取信息地雷运行配置
const
RuntimeSetting* GetInfoMineRuntime()
{
	return &sRuntimeInfo.PushRuntimeConfig.InfoMineRuntime;
}

// 设置公告运行配置
void
SetNoticeRuntime(RuntimeSetting *item)
{
	memcpy(&sRuntimeInfo.PushRuntimeConfig.NoticeRuntime, item, sizeof(RuntimeSetting));
}

// 获取公告运行配置
const
RuntimeSetting* GetNoticeRuntime()
{
	return &sRuntimeInfo.PushRuntimeConfig.NoticeRuntime;
}

// 设置平台用户配置
void 
SetUserConfig(const PlatformUserConfig &item)
{
	sRuntimeInfo.lPlatformUserConfig.insert(pair<unsigned short, PlatformUserConfig>(item.PlatformClassifiedCode, 
			item));
}

// 获取android平台的用户配置
const PlatformUserConfig* 
GetAndroidUserConfig()
{
	map<unsigned short, PlatformUserConfig>::iterator iterFind = 
		sRuntimeInfo.lPlatformUserConfig.find(PFCC_ANDROID);
	if (sRuntimeInfo.lPlatformUserConfig.end() == iterFind)
	{
		return NULL;
	}
	
	return &iterFind->second;
}

// 获取Wp7平台的用户配置
const PlatformUserConfig* 
GetWp7UserConfig()
{
	map<unsigned short, PlatformUserConfig>::iterator iterFind = 
		sRuntimeInfo.lPlatformUserConfig.find(PFCC_WP7);
	if (sRuntimeInfo.lPlatformUserConfig.end() == iterFind)
	{
		return NULL;
	}
	
	return &iterFind->second;
}

// 获取Win8平台的用户配置
const PlatformUserConfig* 
GetWin8UserConfig()
{
	map<unsigned short, PlatformUserConfig>::iterator iterFind = 
		sRuntimeInfo.lPlatformUserConfig.find(PFCC_WIN8);
	if (sRuntimeInfo.lPlatformUserConfig.end() == iterFind)
	{
		return NULL;
	}
	
	return &iterFind->second;
}

// 获取Ios平台的用户配置
const PlatformUserConfig* 
GetIosUserConfig()
{
	map<unsigned short, PlatformUserConfig>::iterator iterFind = 
		sRuntimeInfo.lPlatformUserConfig.find(PFCC_IOS);
	if (sRuntimeInfo.lPlatformUserConfig.end() == iterFind)
	{
		return NULL;
	}
	
	return &iterFind->second;
}

// 设置平台归类
void 
SetPlatformClassifiedMap(const unsigned short PlatformCode,
	const PlatformClassifiedT &PlatformClass)
{
	sRuntimeInfo.lPlatformClsMap.insert(pair<unsigned short, PlatformClassifiedT>(
		PlatformCode, PlatformClass));
}

// 获取平台归类代码
const unsigned short 
GetPlatformClassifiedMap(const unsigned short PlatformCode)
{
	unsigned short OutRes = PFCC_OTHER;
	map<unsigned short, PlatformClassifiedT>::iterator iterFind = 
		sRuntimeInfo.lPlatformClsMap.find(PlatformCode);
	if (sRuntimeInfo.lPlatformClsMap.end() != iterFind)
	{
		OutRes = iterFind->second.PlatformClassifiedCode;
	}
	
	return OutRes;
}

// 获取平台的公告归类代码
const unsigned short
GetPlatformNoticeClassifiedMap(const unsigned short PlatformCode)
{
	unsigned short OutRes = NCCE_OTHER;
	map<unsigned short, PlatformClassifiedT>::iterator iterFind = 
		sRuntimeInfo.lPlatformClsMap.find(PlatformCode);
	if (sRuntimeInfo.lPlatformClsMap.end() != iterFind)
	{
		OutRes = iterFind->second.NoticeClassifiedCode;
	}
	
	return OutRes;
}

// 获取自选股信息地雷相关设置		
SelfSelectStockInfomineConfigT* 
GetSelfSelectStockInfomineConfig()
{
	return &sRuntimeInfo.SelfSelectStockInfomineConfig;
}

int
OperSyncUserInterval(bool bSet, unsigned int &ValueItem)
{
	if (bSet)
	{
		sSyncUserInterval = ValueItem;
	}
	else
	{
		ValueItem = sSyncUserInterval;
	}
	
	return sSyncUserInterval;
}

