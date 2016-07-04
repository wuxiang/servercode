#include "shutdown.h"
#include "../util/util.h"
#include "../util/common/common_types.h"
#include "../util/log/log.h"
#include "../util/clib/getopt.h"
#include "../util/common/common_cmd.h"
#include "../controller/runtime.h"
#include "../util/process/process_manager.h"
#include "../util/string/string_util.h"


/*
 * 提示信息定义
 */
static const char *const _shutdown_usage[] = {
    "Usage: pushserv shutdown -I{pid} | -F{pid}\n",
    "        -I{pid}    shutdown immediately \n",
    "        -F{pid}    force shutdown \n",
    (char *) NULL,
};
/* -------------------------           */

/*
 * 属性
 */
BOOL            _isForceShutdown = FALSE;
/* -------------------------           */


/*
 * 内部函数声明
 */
static BOOL     _InitApp();                                 /* 初始化应用 */

static int      _ImmediatelyShutdown(int);                  /* 立即关闭 */
static int      _ForceShutdown(int);                        /* 强制退出 */

static void     _OnShutdown();                              /* 系统清理 */
/* -------------------------           */


int
Shutdown(int argc, char* argv[]) {
    int     c           = 0;
    int     pid         = 0;
    char    *pidString  = (char *) NULL;

    opterr = 0;
    while ((c = getopt(argc, argv, "I::F::")) != -1) {
        switch (c) {
            case 'I':
                if ((pidString = optarg)) {
                	if (IsEmptyString(pidString) || !IsNumeric(pidString)) {
                		Usage(_shutdown_usage);
                		return -1;
                	}
                    pid = atoi(pidString);
                    return _ImmediatelyShutdown(pid);
                }
                else {
                	Usage(_shutdown_usage);
                	return -1;
                }               

            case 'F':
                if ((pidString = optarg)) {
                	if (IsEmptyString(pidString) || !IsNumeric(pidString)) {
                		Usage(_shutdown_usage);
                		return -1;
                	}
                	
                    pid = atoi(pidString);
                    return _ForceShutdown(pid);
                }
                else {
                	Usage(_shutdown_usage);
                	return -1;
                }
                
            case '?':
            default:
                Usage(_shutdown_usage);
                return -1;
        }
    }

    Usage(_shutdown_usage);
    return -1;
}

int
ShutdownApplication(int mainPid) {
	if (mainPid <= 0 && (mainPid = GetMainProcessId()) <= 0) {
        return -1;
    }

    if (Abort(mainPid) < 0) {
        return -2;
    }

    _OnShutdown();

    return 0;
}

static int
_ImmediatelyShutdown(int pid) {
    printf("\n closing[%s]system ...\n", GetSystemName());

    if (! _InitApp()) {
        return -1;
    }

    if (! IsRunning()) {
        printf(" [%s] has not been started \n", GetSystemName());
        return -1;
    }

    _isForceShutdown = FALSE;
    if (ShutdownApplication(pid) < 0) {
        printf(" %s shutdown failed\n", GetSystemName());
        return -1;
    }

    printf(" %s has been closed.\n", GetSystemName());
    return 0;
}

static int
_ForceShutdown(int pid) {
    printf("\n closing [%s] system , reset state...\n", GetSystemName());

    _isForceShutdown = TRUE;
    if (ShutdownApplication(pid) < 0) {
        _OnShutdown();
        printf(" %s close failed.\n", GetSystemName());
    } else {
        printf(" %s has been closed.\n", GetSystemName());
    }

    return 0;
}

static void
_OnShutdown() {
    SetSystemStatus(STOP);
}

static BOOL
_InitApp() {	
    return TRUE;
}

