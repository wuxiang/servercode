#include "log.h"
#include "../common/common_types.h"
#include "../common/common_file.h"
#include "../time/time_util.h"
#include "../string/string_util.h"
#include "../util.h"

///////////////////�꼰����//////////////////
static const short MAX_LOG_INFO_SIZE = 8192;                // ��־���ݵ���󳤶�
// -------------------------           //

///////////////////��̬����//////////////////
static int sLogFd = STDOUT_FILENO;							// ��ǰ��־�ļ�������
static char sLogFileName[MAX_PATH_LEN] = "RunInfo.log";		// ��ǰ��־�ļ�����
static char sLine[MAX_LINE_SIZE + 1] = {0};					// ��־�ļ�������
static int sLogStatus = 0;									// ��־�ļ�״̬(sLogStatus=0:���� sLogStatus>0:�쳣)
static LOG_LEVEL* sLevel = NULL;							// ��־����
static char LogLevelDesc[10] = {"TRACE"};					// ��־����������
// -------------------------           //

///////////////////��������//////////////////
LOG_LEVEL* GetCurrentLogLevel();							//��ȡ��ǰ��־����
void ActionLogChain(const char *, int, const char *,		// ��¼��־
	const LOG_LEVEL *,const char *, int *);
// -------------------------           /

/// <summary>
/// ��ȡ��ǰ��־����
/// </summary>
/// <param>none</param>
/// <returns>LOG_LEVEL*</returns>
LOG_LEVEL*
GetCurrentLogLevel() {
	return GetLogLevel(LogLevelDesc);
}

/// <summary>
/// ��־�ǼǺ���
/// </summary>
/// <param name="srcFile">���ó���Ĵ����ļ�����</param>
/// <param name="srcLine">���ô��ڴ����ļ��е�����</param>
/// <param name="strFunction">��Դģ������</param>
/// <param name="level">��־����</param>
/// <param name="fmt">��־����, ֧�ֿɱ����</param>
/// <param name="...">�ɱ�����б�</param>
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

    // ��־��Ϣ����
    if (level->level < sLevel ->level){
    	return;
    }

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    // �Ǽ���־
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
/// ��ʼ����־
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

	// ����Ŀ¼
	if (!IsDir("./log"))
	{
		MkDir("./log");
	}

	// �γ�����
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
/// �ͷ���־ռ�õ���Դ
/// </summary>
/// <returns>void</returns>
void
ReleaseLog(){
	if (STDOUT_FILENO != sLogFd){
		CloseFile(&sLogFd);
	}
}
