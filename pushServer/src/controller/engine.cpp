#include "engine.h"
#include "../util/log/log.h"
#include "../util/util.h"
#include "../util/common/common_types.h"
#include "../controller/runtime.h"
#include "../util/process/process_manager.h"
#include "../util/process/daemon.h"
#include "../process/process.h"
#include "../process/model/loguser/Loguser.h"


// 内部函数声明
static void     _Startup();                             /* 启动主控程序 */
static void     _Shutdown(int);                         /* 关闭主控程序 */
static void     _OnStartup();                           /* 启动处理 */
static void     _OnShutdown();                          /* 终止处理 */
static BOOL     _InitSystem();                          /* 系统初始化 */
/// -----------------------------------				/


int
StartEngine() {
	int childPid = 0;

    if ((childPid = TurnIntoDaemon(-1)) < 0) {
        // 创建守护进程失败
        FATAL("TurnIntoDaemon failed!");
        return -1;
    } else if (childPid > 0) {
        // 等待系统运行状态设定
        usleep(100000);
        return childPid;
    } else {
        // 守护进程返回
        _Startup();
        exit(0);
    }
}

static void
_Startup() {
    printf("System Starting ...");

    // 初始化命令监听
    if (!_InitSystem()) {
        printf("_Startup failed!");
    }

    // 注册SIG_ABORT信号，以执行中止前的清理操作
    signal(SIGABRT, _Shutdown);

    // 执行启动时处理
    _OnStartup();

    // 初始化日志
	int iFd = -1;
	const LogControlTag *pLogControl = GetLogControl();
	if (NULL == pLogControl)
	{
		printf("Log not initialized. \n");
		return;
	}
	if (!InitialLog((1 == pLogControl->LogRoute ? NULL : &iFd), pLogControl->LogLevel)) {
		printf("InitialLog failed!\n");
		return;
	}

	// 初始化跟踪用户日志
	LogUserInit("log_user_list.txt");

    // 执行指标计算服务
    if (!ExecuteGuideServer()) {
    	printf("Push Server Execute Failed.\n");
    }

    // 关闭引擎
    _Shutdown(0);
}

static void
_Shutdown(int s) {
    printf("Close main controller.");

    // 执行终止时处理
    _OnShutdown();
    exit(0);
}

static BOOL
_InitSystem() {
    return TRUE;
}

static void
_OnStartup() {
    SetMainProcessId(getpid());
    SetSystemStatus(RUNNING);
    // 初始系统时间
	UpdateNowTime();
}

static void
_OnShutdown() {
	ExitServerInstance();
	ReleaseLog();
	LogUserClose();
    SetSystemStatus(STOP);
}
