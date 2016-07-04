#include "startup.h"
#include "shutdown.h"
#include "../util/util.h"
#include "../util/common/common_types.h"
#include "../util/common/common_cmd.h"
#include "../util/common/common_env.h"
#include "../util/string/string_util.h"
#include "../util/clib/getopt.h"
#include "../util/log/log.h"
#include "../controller/engine.h"
#include "../controller/runtime.h"

/*
 * 提示信息定义
 */
static const char *const _startup_usage[] = {
    "Usage: pushserv startup [-v]\n",
    "        -t  --test         set the user synchronization time interval value.\n",
    (char *) NULL,
};
/* -------------------------           */

// 成员函数定义
static bool _InitialEnv();						// 运行环境初始化
static bool _CheckupAppStatus();               // 检查应用状态
static void  _StopStartup();                   // 中止启动
///---------------------------			/


int
Startup(int argc, char* argv[]) {
    static const char       short_options[] = "t:";
    static struct option    long_options[] = {
        { "test",    0,  NULL, 't' },
        { 0, 0, 0, 0 }
    };

    int     option_index = 0;
    int     c = 0;
 	int childPid = -1;
   
    opterr = 0;
    while ((c = getopt_long(argc, argv, short_options,
            long_options, &option_index)) != EOF) {
        switch (c) {
            case 't':
            	{
            		if (optarg)
            		{
            			if (!IsNumeric(optarg))
            			{
            				printf("t whith error option tag.\n");
            				Usage(_startup_usage);
                			return -1;
            			}
            			
            			unsigned int iSet = atoi(optarg);
            			if (iSet > 0)
            			{
            				OperSyncUserInterval(true, iSet);
            			}
            		}            		
            	}
            	break;

            case '?':
            default:
                Usage(_startup_usage);
                return -1;
        }
    }
    
    // 检查应用状态
    if (! _CheckupAppStatus()) {
        return -1;
    }
    
    // 初始化环境
    if (!_InitialEnv()) {
    	printf("InitialEnv failed, can not Startup \n");
    }    
    
    // 启动主控程序
    printf("Start engine...\n");
    if ((childPid = StartEngine()) < 0) {
        printf(" StartEngine failed!\n");
        _StopStartup();
        return -1;
    }
    else if (childPid > 0) {
    	printf("System Started Success. \nMain Controller ID=[%d].\n%s is running now\n\n",
	 		childPid, GetSystemName());
    }
    
    return 0;
}

static bool 
_InitialEnv() {	
	return true;
}

static bool
_CheckupAppStatus() {
   
    // 检查应用运行状态
    printf(" checkup status ... \n");
    if (IsRunning()) {
        printf("system has been started or system has been terminated abnormal!\n\n\n");
        return false;
    }

    return true;
}

static void
_StopStartup() {
    printf("\nWARN: terminate operation! \n");
    ShutdownApplication(-1);
}

