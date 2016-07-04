#include "log.h"
#include "../common/common_types.h"
#include "../common/common_file.h"
#include "../time/time_util.h"
#include "../string/string_util.h"
#include "../util.h"

///////////////////宏及常量//////////////////
static const short MAX_LOG_INFO_SIZE = 8192;                // 日志内容的最大长度
// -------------------------           //

///////////////////静态变量//////////////////
static int sLogFd = STDOUT_FILENO;							// 当前日志文件描述符
static char sLogFileName[MAX_PATH_LEN] = "RunInfo.log";		// 当前日志文件名称
static char sLine[MAX_LINE_SIZE + 1] = {0};					// 日志文件内容行
static int sLogStatus = 0;									// 日志文件状态(sLogStatus=0:正常 sLogStatus>0:异常)
static LOG_LEVEL* sLevel = NULL;							// 日志级别
static char LogLevelDesc[10] = {"TRACE"};					// 日志级别描述符
// -------------------------           //

///////////////////函数声明//////////////////
LOG_LEVEL* GetCurrentLogLevel();							//获取当前日志级别
void ActionLogChain(const char *, int, const char *,		// 记录日志
	const LOG_LEVEL *,const char *, int *);
// -------------------------           /

/// <summary>
/// 获取当前日志级别
/// </summary>
/// <param>none</param>
/// <returns>LOG_LEVEL*</returns>
LOG_LEVEL*
GetCurrentLogLevel() {
	return GetLogLevel(LogLevelDesc);
}

/// <summary>
/// 日志登记函数
/// </summary>
/// <param name="srcFile">调用程序的代码文件名称</param>
/// <param name="srcLine">调用处在代码文件中的行数</param>
/// <param name="strFunction">来源模块名称</param>
/// <param name="level">日志级别</param>
/// <param name="fmt">日志内容, 支持可变参数</param>
/// <param name="...">可变参数列表</param>
/// <returns>void</returns>
void
LogImpl(const char *srcFile, int srcLine, const char *strFunction, const LOG_LEVEL *level,
        const char *fmt, ...) {
    va_list             ap;
    char                buf[MAX_LOG_INFO_SIZE];

    if (NULL == sLevel){
    	printf("Log level not defined error!\n");
    	return;
    }

    // 日志信息过滤
    if (level->level < sLevel ->level){
    	return;
    }

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    // 登记日志
    ActionLogChain(srcFile, srcLine, strFunction, level, buf, &sLogFd);
}

void ActionLogChain(const char *srcFile, int srcLine, const char *strFunction,
	const LOG_LEVEL *level, const char *contents, int *pFd){
	bzero(sLine, MAX_LINE_SIZE);
	struct timeval	tv;
    int				usec;
    char 			tmp[64];
    int isize = 0;

    GetTimeOfDay(&tv);
    FormatTime(tmp, localtime(&tv.tv_sec), TIME_FORMAT_FORMATTED_TIMESTAMP);
    usec = tv.tv_usec / 100;

	isize = snprintf(sLine, MAX_LINE_SIZE, "%s.%04d %-5s [%5d] (%s#%d) <%s> - %s\n",
            tmp, usec, level->name, getpid(), srcFile, srcLine,
            strFunction, contents);

    if (isize <= 0) {
		printf("contents format error!\n");
		return;
	}

	if ((flock(*pFd, LOCK_EX)) < 0 ){
		printf("flock LOCK_EX error![%s]\n", strerror(errno));
		return;
	}

    if (write(*pFd, sLine, isize) < 0) {
        printf("write to file fail! - file: [%s]; error: [%d:%s]\n",
            sLogFileName, errno, strerror(errno));
    }

    if ((flock(*pFd, LOCK_UN)) < 0 ){
    	printf("flock LOCK_UN error!\n");
 	}
}

/// <summary>
/// 初始化日志
/// </summary>
/// <param name="pFd"></param>
/// <param name = "name"></param>
/// <returns>LOG_LEVEL*</returns>
bool
InitialLog(int *pFd, const char *name){
	if (NULL == name || IsEmptyString(name) || strlen(name) >= 10){
		return false;
	}

	bzero(LogLevelDesc, 10);
	StrCopy(LogLevelDesc, name, strlen(name));
	sLevel = GetCurrentLogLevel();
	if (NULL == sLevel)
	{
		return false;
	}

	// 创建目录
	if (!IsDir("./log"))
	{
		MkDir("./log");
	}

	// 形成名称
	char cDate[16] = {0};
	GetDate(cDate);
	snprintf(sLogFileName, MAX_PATH_LEN, "./log/%s_RunInfo.log", cDate);

	if (! pFd) {
        pFd = &sLogFd;
    }
    else if (-1 == *pFd){
    	if ((*pFd = open(sLogFileName, O_WRONLY | O_CREAT | O_TRUNC, F_MODE_RW)) < 0) {
            printf("open file fail! - file: [%s]; error: [%d:%s]\n",
                    sLogFileName, errno, strerror(errno));
            sLogStatus = 1;
            return false;
        }
        sLogFd = *pFd;
        sLogStatus = 0;
    }

    return true;
}

/// <summary>
/// 释放日志占用的资源
/// </summary>
/// <returns>void</returns>
void
ReleaseLog(){
	if (STDOUT_FILENO != sLogFd){
		CloseFile(&sLogFd);
	}
}
