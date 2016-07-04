#include "common_env.h"
#include "../util.h"
#include <unistd.h>
#include <stdio.h>
#include "../string/string_util.h"


/// <summary>
/// 获取应用程序执行绝对路径
/// </summary>
/// <param name="buf"></param>
/// <param name="count"></param>
/// <returns>char*</returns>
const char* 
GetApplicationPath(char *buf, const int count)
{    
	FILE *fp;
	char *cmd = "pwd";
	
	if(NULL == (fp = popen(cmd, "r")))
	{
		perror("popen");
		return NULL;
	}
	   
	while(NULL != (fgets(buf, count, fp)))
	{
	}
	pclose(fp);
	return buf;
}

bool 
IsAppRunning(const char *appName, const int pid) {
	char Cmd[256] = {0};
	char cGotStr[256] = {0};
	FILE *fp;
	int bRun = 0;
		
	if (0 >= pid) {
		return false;
	}
	snprintf(Cmd, 256, "ps -ef|awk '{print $2}'|grep %d", pid);
	
	if((fp = popen(Cmd, "r")) == NULL) {
		perror("popen failed\n");
		return false;
	}
	else {
		while(fgets(cGotStr, 255, fp) != NULL) {
			bRun = 1;
			break;
		}
	}
	pclose(fp);
	
	return (1 == bRun);
}

// 调用线程休眠
void SleepMs(const int millsec) {
	usleep(millsec * 1000);
}

