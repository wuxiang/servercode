#include "../util/util.h"
#include "../util/common/common_cmd.h"
#include "../command/startup.h"
#include "../command/shutdown.h"
#include "../command/checkup.h"
#include "../command/version.h"
#include "../controller/runtime.h"
#include "../util/config/ini/iniparser.h"
#include "../util/config/xml/libxml_parser.h"
#include "../process/config.h"
#include "../process/data_type.h"

/// 命令定义
static const struct cmd {
    const char  *fullname;
    int         (*func) (int, char**);
}
_cmds[] = {
    { "startup",        Startup     },
    { "shutdown",       Shutdown    },
    { "checkup",        Checkup     },
    { "version",        Version     },
    { "--version",      Version     },
    { (char *) NULL,    (int (*) (int, char**)) NULL    },
};
/// -------------------------           /


/// 提示信息定义
static const char *const _cmds_usage[] = {
    "Usage: pushserv command [-options]\n"
    "\n"
    "pushserv commands are:\n",
    "        [startup]    startup system\n",
    "        shutdown     shutdown system\n",
    "        checkup      checkup system status\n",
    "        version      checkup system version \n",
    (char *) NULL,
};
/// -------------------------           /

///内部函数定义
//static bool _ReadIniConfigFile();			// 读取INI配置文件信息
static bool _ReadXmlConfigFile();			// 读取XML配置文件信息
static bool _ReadLogConfig(const LibXml::DOMDocumentPtr &);
static bool _ReadServerConfig(const LibXml::DOMDocumentPtr &);
static bool _ReadDataServer(const LibXml::DOMDocumentPtr &);
static bool _ReadPlatformUserSetting(const LibXml::DOMDocumentPtr &);
static bool _ReadDbConfig(const LibXml::DOMDocumentPtr &);
static bool _ReadOuterPushConfig(const LibXml::DOMDocumentPtr &);
static bool _ReadPushRuntimeConfig(const LibXml::DOMDocumentPtr &);
static bool _ReadPlatformClassifiedConfig(const LibXml::DOMDocumentPtr &);
static bool _ReadSelfSelectStockInfomineConfigT(const LibXml::DOMDocumentPtr &);
/// -------------------------           /

/// <summary>
/// 主程序
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns>int</returns>
int
main(int argc, char* argv[]) {
	char *appName = NULL;
	char *commandName = NULL;
	const struct cmd    *pCmd;

    if (argc < 1) {
        fprintf(stderr, "\nERROR: param!\n");

        Usage(_cmds_usage);
        return 0;
    } else {
        appName = argv[0];
        argc --;
        argv ++;
    }
    
    if (strrchr(appName, '/') != NULL) {
        appName = strrchr(appName, '/') + 1;
    }
    SetSystemName(appName);    
    
    if (argc > 0)
    {
    	commandName = argv[0];
    }
    else
    {
    	commandName = (char*)_cmds[0].fullname;
    }
    for (pCmd = _cmds; pCmd->fullname; pCmd++) {
        if (!strcmp(commandName, pCmd->fullname))
            break;
    }

    if (!pCmd->fullname) {
        Usage(_cmds_usage);
        return 0;
    }
    
    if (!_ReadXmlConfigFile()) {
    	fprintf(stderr, "\nERROR: ReadXmlConfigFile \n");
    	return 1;
    }
        
    return (*(pCmd->func)) (argc, argv);
}

/*
static bool 
_ReadIniConfigFile() {
	char *pConfigFile = "setting.ini";
	dictionary	*	ini ;
	char *pLogLevel = NULL;
	int iFd = -1;
	int iValue = 0;
	
	ini = iniparser_load(pConfigFile);
	if (NULL == ini) {
		fprintf(stderr, "cannot parse file: %s\n", pConfigFile);
		return false;
	}
		
	// 取日志级别描述符
	pLogLevel = iniparser_getstring(ini, "RUN_TIME:LogLevel", NULL);
	// 取日志输出控制
	iValue = iniparser_getint(ini, "RUN_TIME:OutPutLogToConsole", 1);
	
		
	// 取网络配置
	ServerTag configSer;
	iValue = iniparser_getint(ini, "RUN_TIME:ListenPort", 76687);
	configSer.ListenPort = iValue;
	
	SetServerConfig(&configSer);
		
	// 释放
	iniparser_freedict(ini);
	
	return true;
}
*/

// 读取Log配置
static bool
_ReadLogConfig(const LibXml::DOMDocumentPtr &pDoc)
{
	LibXml::DOMNodePtr pNode = pDoc->getNode("/Configure/Log");
    if (!pNode.get()) 
	{
		return false;
	}
	std::string strTemp;
	LibXml::DOMAttributesPtr attr;

	attr = pNode->getAttributes();

	LogControlTag logControl;
	// 取日志级别描述符
	strTemp = "";
	attr->get("LogLevel", strTemp);
	strncpy(logControl.LogLevel, strTemp.c_str(), 10);
	
	// 取日志输出控制
	strTemp = "";
	attr->get("OutPutLogToConsole", strTemp);
	logControl.LogRoute = atoi(strTemp.c_str());
	SetLogControl(&logControl);
	
	return true;
}

// 取服务配置
static bool 
_ReadServerConfig(const LibXml::DOMDocumentPtr &pDoc)
{
	LibXml::DOMNodePtr pNode = pDoc->getNode("/Configure/Server");
    if (!pNode.get()) 
	{
		return false;
	}
	std::string strTemp;
	LibXml::DOMAttributesPtr attr;
	attr = pNode->getAttributes();
	
	ServerTag configSer;
	strTemp = "";
	attr->get("AppLogInValid", strTemp);
	strncpy(configSer.AppLogInValid, strTemp.c_str(), AppLogInValidLen);
	
	strTemp = "";
	attr->get("CalThreadNum", strTemp);
	configSer.CalThreadNum = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("MaxLinkToHq", strTemp);
	configSer.MaxLinkToHq = atoi(strTemp.c_str());
		
	strTemp = "";
	attr->get("ListenPort", strTemp);
	configSer.ListenPort = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("begintime", strTemp);
	configSer.StartInitialTime = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("endtime", strTemp);
	configSer.EndInitialTime = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("ServerCodeNum", strTemp);
	configSer.ServerCodeNum = atoi(strTemp.c_str());
	
	SetServerConfig(&configSer);
	
	return true;
}

static bool
_ReadDataServer(const LibXml::DOMDocumentPtr &pDoc)
{
	LibXml::DOMNodePtr pNode = pDoc->getNode("/Configure/DataServer");
    if (!pNode.get()) 
	{
		return false;
	}
	std::string strTemp;
	LibXml::DOMAttributesPtr attr;

	attr = pNode->getAttributes();
	
	DataServerTag DataSer;
	strTemp = "";
	attr->get("Add1", strTemp);
	strncpy(DataSer.Addr, strTemp.c_str(), 25);
	
	strTemp = "";
	attr->get("Port1", strTemp);
	DataSer.Port = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("MaxRecord", strTemp);
	DataSer.MaxRecordNum = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("MarketInclude", strTemp);
	strncpy(DataSer.MarketInclude, strTemp.c_str(), MaxMarketCount * MarketCodeLen);
	SetDataServerTag(&DataSer);
	
	strTemp = "";
	attr->get("Add2", strTemp);
	strncpy(DataSer.Addr, strTemp.c_str(), 25);
	
	strTemp = "";
	attr->get("Port2", strTemp);
	DataSer.Port = atoi(strTemp.c_str());
	SetDataServerTag2(&DataSer);
	return true;
}

static bool _ReadClassifiedPlatformUserSetting(const LibXml::DOMDocumentPtr &pDoc,
	const std::string &NodePath)
{
	LibXml::DOMNodePtr pNode = pDoc->getNode(NodePath);
    if (!pNode.get()) 
	{
		return false;
	}
	std::string strTemp;
	LibXml::DOMAttributesPtr attr;
		
	LibXml::DOMNodePtr ScanNode = pNode->getFirstChildNode();
	while (NULL != ScanNode.get())
	{
		if (XML_ELEMENT_NODE == ScanNode->getType())
		{
			attr = ScanNode->getAttributes();
	
			PlatformUserConfig UserConfig;
			bzero(&UserConfig, sizeof(UserConfig));
			
			strTemp = "";
			attr->get("PlatformClassifiedCode", strTemp);
			UserConfig.PlatformClassifiedCode = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("MaxUser", strTemp);
			UserConfig.MaxUser = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("UserFileName", strTemp);
			strncpy(UserConfig.UserFileName, strTemp.c_str(), DEFAULT_USE_LEN);
			
			strTemp = "";
			attr->get("MaxUserWithEWarn", strTemp);
			UserConfig.MaxUserWithEWarn = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("MsgHisFileName", strTemp);
			strncpy(UserConfig.MsgHisFileName, strTemp.c_str(), DEFAULT_USE_LEN);
				
			strTemp = "";
			attr->get("MaxEWarningHisNum", strTemp);
			UserConfig.MaxEWarningHisNum = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("MaxInfoHisNum", strTemp);
			UserConfig.MaxInfoHisNum = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("MaxNoticeHisNum", strTemp);
			UserConfig.MaxNoticeHisNum = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("EWarningFileName", strTemp);
			strncpy(UserConfig.EWarningFileName, strTemp.c_str(), DEFAULT_USE_LEN);
			
			strTemp = "";
			attr->get("MaxEWarnPerUser", strTemp);
			UserConfig.MaxEWarnPerUser = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("DefaultEWarnPerUser", strTemp);
			UserConfig.DefaultEWarnPerUser = atoi(strTemp.c_str());
			
			SetUserConfig(UserConfig);
		}
		
		ScanNode = ScanNode->getNextSiblingNode();
	}
	
	return true;
}

static bool 
_ReadPlatformUserSetting(const LibXml::DOMDocumentPtr &pDoc)
{
	if (!_ReadClassifiedPlatformUserSetting(pDoc, "/Configure/PlatformUserSetting"))
	{
		return false;
	}
	
	return true;
}

static bool
_ReadDbConfig(const LibXml::DOMDocumentPtr &pDoc)
{
	LibXml::DOMNodePtr pNode = pDoc->getNode("/Configure/DataBaseConfig/MySqlDbConfig");
    if (!pNode.get()) 
	{
		return false;
	}
	std::string strTemp;
	LibXml::DOMAttributesPtr attr;
	attr = pNode->getAttributes();
	
	MySqlDbConfig dbconfig;
	strTemp = "";
	attr->get("DbName", strTemp);
	strncpy(dbconfig.DbName, strTemp.c_str(), DEFAULT_USE_LEN);
	
	strTemp = "";
	attr->get("LogUser", strTemp);
	strncpy(dbconfig.LogUser, strTemp.c_str(), DEFAULT_USE_LEN);
	
	strTemp = "";
	attr->get("LogPwd", strTemp);
	strncpy(dbconfig.LogPwd, strTemp.c_str(), DEFAULT_USE_LEN);
	SetDbSetting(&dbconfig);
	
	// 读取信息地雷数据库配置
	WapInfoMineConfig imconfig;
	pNode = pDoc->getNode("/Configure/DataBaseConfig/WapInfoMineConfig");
	if (!pNode.get()) 
	{
		return false;
	}	
	attr = pNode->getAttributes();
	strTemp = "";
	attr->get("DSN", strTemp);
	strncpy(imconfig.DSN, strTemp.c_str(), DEFAULT_USE_LEN);
	
	strTemp = "";
	attr->get("LogUser", strTemp);
	strncpy(imconfig.LogUser, strTemp.c_str(), DEFAULT_USE_LEN);
	
	strTemp = "";
	attr->get("LogPwd", strTemp);
	strncpy(imconfig.LogPwd, strTemp.c_str(), DEFAULT_USE_LEN);
	SetInfoMineSetting(&imconfig);
	
	// 读取公共消息数据库配置
	WapPublicNewsConfig pnconfig;
	pNode = pDoc->getNode("/Configure/DataBaseConfig/WapPublicNewsConfig");
	if (!pNode.get()) 
	{
		return false;
	}	
	attr = pNode->getAttributes();
	strTemp = "";
	attr->get("DSN", strTemp);
	strncpy(pnconfig.DSN, strTemp.c_str(), DEFAULT_USE_LEN);
	
	strTemp = "";
	attr->get("LogUser", strTemp);
	strncpy(pnconfig.LogUser, strTemp.c_str(), DEFAULT_USE_LEN);
	
	strTemp = "";
	attr->get("LogPwd", strTemp);
	strncpy(pnconfig.LogPwd, strTemp.c_str(), DEFAULT_USE_LEN);
	SetPublicNewsSetting(&pnconfig);
	
	return true;
}

static bool 
_ReadOuterPushConfig(const LibXml::DOMDocumentPtr &pDoc)
{
	LibXml::DOMNodePtr pNode = pDoc->getNode("/Configure/PushServerConfig");
    if (!pNode.get()) 
	{
		return false;
	}
	std::string strTemp;
	LibXml::DOMAttributesPtr attr;

	LibXml::DOMNodePtr ScanNode = pNode->getFirstChildNode();
	while (NULL != ScanNode.get())
	{
		if (XML_ELEMENT_NODE == ScanNode->getType())
		{
			attr = ScanNode->getAttributes();
	
			OuterPushServ appServ;
			bzero(&appServ, sizeof(OuterPushServ));
			
			strTemp = "";
			attr->get("ConnectNum", strTemp);
			appServ.ConnectNum = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("HostAdd", strTemp);
			strncpy(appServ.HostAdd, strTemp.c_str(), DEFAULT_USE_LEN);
			
			strTemp = "";
			attr->get("HostPort", strTemp);
			appServ.HostPort = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("CertFile", strTemp);
			strncpy(appServ.CertFile, strTemp.c_str(), DEFAULT_USE_LEN);
			
			strTemp = "";
			attr->get("KeyFile", strTemp);
			strncpy(appServ.KeyFile, strTemp.c_str(), DEFAULT_USE_LEN);
			
			strTemp = "";
			attr->get("PlatformCode", strTemp);
			appServ.PlatformCode = atoi(strTemp.c_str());
			
			SetOuterPushServ(appServ);
		}
		
		ScanNode = ScanNode->getNextSiblingNode();
	}
	
	return true;
}

static bool
_ReadPushRuntimeConfig(const LibXml::DOMDocumentPtr &pDoc)
{
	// 读取节点配置
	LibXml::DOMNodePtr pNode = pDoc->getNode("/Configure/PushRuntimeConfig/UserNodeServer");

	std::string strTemp;
	LibXml::DOMAttributesPtr attr;
	UserNodeServerT UserNodeServer;
	if (pNode.get()) 
	{
		attr = pNode->getAttributes();
		strTemp = "";
		attr->get("CheckStart", strTemp);
		UserNodeServer.CheckStart = atoi(strTemp.c_str());
		
		strTemp = "";
		attr->get("CheckEnd", strTemp);
		UserNodeServer.CheckEnd = atoi(strTemp.c_str());
		
		strTemp = "";
		attr->get("SUserDeadTime", strTemp);
		UserNodeServer.SUserDeadTime = atoi(strTemp.c_str());
		
		strTemp = "";
		attr->get("LUserDeadTime", strTemp);
		UserNodeServer.LUserDeadTime = atoi(strTemp.c_str());
	}
	else
	{
		UserNodeServer.CheckStart = USER_DEFAULT_CHECK_START;
		UserNodeServer.CheckEnd = USER_DEFAULT_CHECK_END;
		UserNodeServer.SUserDeadTime = S_USER_DEFAULT_DEAD_TIME;
		UserNodeServer.LUserDeadTime = L_USER_DEFAULT_DEAD_TIME;
	}
	SetUserNodeServer(&UserNodeServer);
	
	// 预警运行配置
	RuntimeSetting EWarningRuntime;
	pNode = pDoc->getNode("/Configure/PushRuntimeConfig/EWarningRuntime");
	if (!pNode.get()) 
	{
		return false;
	}	
	attr = pNode->getAttributes();
	strTemp = "";
	attr->get("RunStartTime", strTemp);
	EWarningRuntime.RunStartTime = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("RunEndTime", strTemp);
	EWarningRuntime.RunEndTime = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("ScanInterval", strTemp);
	EWarningRuntime.ScanInterval = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("GetLastestInterval", strTemp);
	EWarningRuntime.GetLastestInterval = atoi(strTemp.c_str());
	
	EWarningRuntime.ProcessUnit = 0;
	
	strTemp = "";
	attr->get("RemainValidTime", strTemp);
	EWarningRuntime.RemainValidTime = atoi(strTemp.c_str());
	SetEWarningRuntime(&EWarningRuntime);
	
	RuntimeSetting InfoMineRuntime;
	pNode = pDoc->getNode("/Configure/PushRuntimeConfig/InfoMineRuntime");
	if (!pNode.get()) 
	{
		return false;
	}	
	attr = pNode->getAttributes();
	strTemp = "";
	attr->get("RunStartTime", strTemp);
	InfoMineRuntime.RunStartTime = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("RunEndTime", strTemp);
	InfoMineRuntime.RunEndTime = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("ScanInterval", strTemp);
	InfoMineRuntime.ScanInterval = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("GetLastestInterval", strTemp);
	InfoMineRuntime.GetLastestInterval = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("ProcessUnit", strTemp);
	InfoMineRuntime.ProcessUnit = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("RemainValidTime", strTemp);
	InfoMineRuntime.RemainValidTime = atoi(strTemp.c_str());
	SetInfoMineRuntime(&InfoMineRuntime);
	
	RuntimeSetting NoticeRuntime;
	pNode = pDoc->getNode("/Configure/PushRuntimeConfig/NoticeRuntime");
	if (!pNode.get()) 
	{
		return false;
	}	
	attr = pNode->getAttributes();
	strTemp = "";
	attr->get("RunStartTime", strTemp);
	NoticeRuntime.RunStartTime = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("RunEndTime", strTemp);
	NoticeRuntime.RunEndTime = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("ScanInterval", strTemp);
	NoticeRuntime.ScanInterval = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("GetLastestInterval", strTemp);
	NoticeRuntime.GetLastestInterval = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("ProcessUnit", strTemp);
	NoticeRuntime.ProcessUnit = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("RemainValidTime", strTemp);
	NoticeRuntime.RemainValidTime = atoi(strTemp.c_str());
	SetNoticeRuntime(&NoticeRuntime);
	
	return true;
}

static bool 
_ReadXmlConfigFile() {
	char *pConfigFile = "config.xml";
		
	LibXml::DOMDocumentPtr pDoc = LibXml::DOMDocumentPtr(new LibXml::LIBXML_DOMDocument());
    if(!pDoc->load(pConfigFile))
    {
        return false;
    }
    
    if (!_ReadLogConfig(pDoc))
    {
    	return false;
    }
    
    if (!_ReadServerConfig(pDoc))
    {
    	return false;
    }
    
    if (!_ReadDataServer(pDoc))
    {
    	return false;
    }
    
    if (!_ReadPlatformUserSetting(pDoc))
    {
    	return false;
    }
    
    if (!_ReadDbConfig(pDoc))
    {
    	return false;
    }
    
    if (!_ReadOuterPushConfig(pDoc))
    {
    	return false;
    }
    
    if (!_ReadPushRuntimeConfig(pDoc))
    {
    	return false;
    }
    
    if (!_ReadPlatformClassifiedConfig(pDoc))
    {
    	return false;
    }
    
    if (!_ReadSelfSelectStockInfomineConfigT(pDoc))
    {
    	return false;
    }
	
	return true;
}

static bool 
_ReadPlatformClassifiedConfig(const LibXml::DOMDocumentPtr &pDoc)
{
	LibXml::DOMNodePtr pNode = pDoc->getNode("/Configure/PlatformClassified");
    if (!pNode.get()) 
	{
		return false;
	}
	std::string strTemp;
	LibXml::DOMAttributesPtr attr;

	LibXml::DOMNodePtr ScanNode = pNode->getFirstChildNode();
	while (NULL != ScanNode.get())
	{
		if (XML_ELEMENT_NODE == ScanNode->getType())
		{
			attr = ScanNode->getAttributes();
	
			PlatformClassifiedT cPlatformClassified;
			unsigned short uPlatform = 0;
			
			strTemp = "";
			attr->get("PlatformCode", strTemp);
			uPlatform = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("PlatformClassifiedCode", strTemp);
			cPlatformClassified.PlatformClassifiedCode = atoi(strTemp.c_str());
			
			strTemp = "";
			attr->get("NoticeClassifiedCode", strTemp);
			cPlatformClassified.NoticeClassifiedCode = atoi(strTemp.c_str());
			
			SetPlatformClassifiedMap(uPlatform, cPlatformClassified);
		}
		
		ScanNode = ScanNode->getNextSiblingNode();
	}
	
	return true;
}

int TimeCmp(const void* left, const void* right)
{
	int *pLeft = (int*)left;
	int *pRight = (int*)right;
	return (*pLeft) - (*pRight);
}

static bool
_ReadSelfSelectStockInfomineConfigT(const LibXml::DOMDocumentPtr &pDoc)
{
	LibXml::DOMNodePtr pNode = pDoc->getNode("/Configure/PushExtendConfig/SelfSelectStockInfomineConfig");
    if (!pNode.get()) 
	{
		return false;
	}
	std::string strTemp;
	LibXml::DOMAttributesPtr attr;
	SelfSelectStockInfomineConfigT *pConfig = GetSelfSelectStockInfomineConfig();
		
	attr = pNode->getAttributes();
	strTemp = "";
	attr->get("CacheCount", strTemp);
	pConfig->CacheCount = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("FileName", strTemp);
	strncpy(pConfig->FileName, strTemp.c_str(), DEFAULT_USE_LEN);
	
	strTemp = "";
	attr->get("StatisticsTimeCount", strTemp);
	pConfig->StatisticsTimeCount = atoi(strTemp.c_str());
	// SelfSelectRecHead分配了三个bit用户存储
	if (pConfig->StatisticsTimeCount > 7)
	{
		pConfig->StatisticsTimeCount = 7;
	}
	
	strTemp = "";
	attr->get("StatisticsTime", strTemp);
	pConfig->StatisticsTime = pConfig->StatisticsTimeCount > 1 ? new int[pConfig->StatisticsTimeCount] : new int;
	int iStartPos = 0;
	int iPosSep = strTemp.find("|", iStartPos);
	std::string code("");
	int iCount = 0;
	while(iPosSep >= 0)
	{
		code = strTemp.substr(iStartPos, iPosSep - iStartPos);
		if ("" != code && iCount < pConfig->StatisticsTimeCount);
		{
			pConfig->StatisticsTime[iCount++] = atoi(code.c_str());
		}
		
		iStartPos = iPosSep + 1;
		iPosSep = strTemp.find("|", iStartPos);
	}
	code = strTemp.substr(iStartPos);
	if ("" != code && iCount < pConfig->StatisticsTimeCount);
	{
		pConfig->StatisticsTime[iCount++] = atoi(code.c_str());
	}
	if (iCount > 1)
	{
		qsort(pConfig->StatisticsTime, iCount, sizeof(int), TimeCmp);
	}
	
	if (pConfig->StatisticsTimeCount != iCount)
	{
		return false;
	}
	
	strTemp = "";
	attr->get("ExcludeCodeCount", strTemp);
	pConfig->ExcludeCodeCount = atoi(strTemp.c_str());
	
	strTemp = "";
	attr->get("ExcludeCode", strTemp);
	if (pConfig->ExcludeCodeCount > 0)
	{
		pConfig->ExcludeCode = new char[pConfig->ExcludeCodeCount * MAX_STKCODE_LEN + 1];
		bzero(pConfig->ExcludeCode, pConfig->ExcludeCodeCount * MAX_STKCODE_LEN);
		strncpy(pConfig->ExcludeCode, strTemp.c_str(), pConfig->ExcludeCodeCount * MAX_STKCODE_LEN);
	}

	return true;
}

