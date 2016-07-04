#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h>
#include "../../../util/util.h"
#include "../../../util/log/log.h"
#include "../../../util/time/time_util.h"
#include "../../../util/common/common_file.h"
#include "Loguser.h"
#include <unistd.h>


#define MAX_USER_ID_LEN 64
struct UserNode
{
	// User ID
	char strUserID[MAX_USER_ID_LEN + 1];

	// Platform ID
	int platformCode;
	
	UserNode& operator=(const UserNode &item)
	{
		strncpy(strUserID, item.strUserID, MAX_USER_ID_LEN);
		platformCode = item.platformCode;
		return *this;
	}
};


pthread_mutex_t vecMutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<UserNode> vecUsersInList;
time_t lastModifiedTime = -1;
static int sLogFd = -1;
char sLogFileName[MAX_PATH_LEN + 1] = {0};
static const short MAX_LOG_INFO_SIZE = 8192;
char sUserListFileName[MAX_PATH_LEN + 1] = {0};


// #_LOG_USER=0

bool
ReadUserList(const char *userList, std::vector<UserNode> &users)
{
	assert(userList != NULL);

	struct stat statbuf;
	stat(userList, &statbuf);
	if(!(statbuf.st_mtime > lastModifiedTime))
		return true;

	FILE *fp = fopen(userList, "r");
	if(!fp)
	{
		ERROR("Open user-list file failed\n");
		return false;
	}

	std::string s;
	UserNode userNode;

	std::ifstream in(userList);

	// Read user list into vector, threadsafe
	pthread_mutex_lock(&vecMutex);
	// Clear old list
	users.clear();
	while(!in.eof())
	{
		in >> s;
		strcpy(userNode.strUserID, s.c_str());
		in >> userNode.platformCode;
		users.push_back(userNode);
	}
	pthread_mutex_unlock(&vecMutex);

	in.close();
	fclose(fp);
	time(&lastModifiedTime);
	
	TRACE("ReadUserList count=%u", users.size());

	return true;
}

bool
IsUserInList(UserNode &user)
{
	pthread_mutex_lock(&vecMutex);
	std::vector<UserNode>::iterator iter;
	bool bRes = false;
	for(iter = vecUsersInList.begin(); iter !=  vecUsersInList.end(); ++iter)
	{
		if(!strcmp(user.strUserID, iter->strUserID) && user.platformCode == iter->platformCode)
		{
			bRes = true;
			break;
		}
	}
	pthread_mutex_unlock(&vecMutex);

	return bRes;
}

bool
FileExist(const char *filename)
{
	assert(filename != NULL);
	return (access(filename, 0) == 0);
}

bool
InitialLogUserFile()
{
	// 形成名称
	char cDate[16] = {0};
	GetDate(cDate);
	snprintf(sLogFileName, MAX_PATH_LEN, "./trace/%s.bt", cDate);
	
	sLogFd = open(sLogFileName, O_WRONLY | O_CREAT | O_TRUNC, F_MODE_RW);
	if (sLogFd < 0)
	{
		ERROR("open file error");
		return false;
	}
	
	return true;
}

void
LogUserInit(const char *userListFile)
{
#ifdef _LOG_USER

	// 创建目录
	if (!IsDir("./trace"))
	{
		MkDir("./trace");
	}

	strcpy(sUserListFileName, userListFile);
	if(!ReadUserList(sUserListFileName, vecUsersInList))
	{
		ERROR("Reading user list file failed.");
	}
	
	InitialLogUserFile();
	
#endif
}

void
LogUserPrint(const char *srcFile, int srcLine, const char *strFunction,
	const char *UserID, const unsigned char platformCode, 
	const short level, const char *fmt, ...)
{
#ifdef _LOG_USER
	assert(fmt!= NULL);
	
	if (level<_LOG_USER)
		return;

	char sLine[1024];
	va_list ap;
    char buf[MAX_LOG_INFO_SIZE] = {0};

	UserNode user;
	strncpy(user.strUserID, UserID, MAX_USER_ID_LEN);
	user.platformCode = platformCode;
	
	if (!IsUserInList(user))
	{
		return;
	}
	
	bzero(sLine, 1024);
	struct timeval	tv;
    int				usec;
    char 			tmp[64];
    int isize = 0;
    
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    GetTimeOfDay(&tv);
    FormatTime(tmp, localtime(&tv.tv_sec), TIME_FORMAT_FORMATTED_TIMESTAMP);
    usec = tv.tv_usec / 100;
    
	isize = snprintf(sLine, 1024, "%s.%04d %u (%s#%d) <%s> - %s\n",
            tmp, usec, level, srcFile, srcLine, strFunction, buf);
    
    if (isize <= 0) {
		DEBUG("contents format error!\n");
		return;
	}
	
	if ((flock(sLogFd, LOCK_EX)) < 0 ){
		DEBUG("flock LOCK_EX error![%s]\n", strerror(errno));
		return;
	}
	
    if (write(sLogFd, sLine, isize) < 0) {
        DEBUG("write to file fail! - file: [%s]; error: [%d:%s]\n",
            sLogFileName, errno, strerror(errno));
    }
    
    if ((flock(sLogFd, LOCK_UN)) < 0 ){
    	DEBUG("flock LOCK_UN error!\n");
 	}
	
#endif
}


void
LogUserClose()
{
#ifdef _LOG_USER
	if (sLogFd > 2)
	{
		close(sLogFd);
		sLogFd = -1;
	}
	
#endif
}

// 获取更新
void
GetDynaUserList()
{
#ifdef _LOG_USER
	if (strlen(sUserListFileName) < 1)
	{
		return;
	}
	
	ReadUserList(sUserListFileName, vecUsersInList);
#endif
}

