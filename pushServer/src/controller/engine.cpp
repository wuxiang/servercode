#include "engine.h"
#include "../util/log/log.h"
#include "../util/util.h"
#include "../util/common/common_types.h"
#include "../controller/runtime.h"
#include "../util/process/process_manager.h"
#include "../util/process/daemon.h"
#include "../process/process.h"
#include "../process/model/loguser/Loguser.h"


// �ڲ���������
static void     _Startup();                             /* �������س��� */
static void     _Shutdown(int);                         /* �ر����س��� */
static void     _OnStartup();                           /* �������� */
static void     _OnShutdown();                          /* ��ֹ���� */
static BOOL     _InitSystem();                          /* ϵͳ��ʼ�� */
/// -----------------------------------				/


int
StartEngine() {
	int childPid = 0;

    if ((childPid = TurnIntoDaemon(-1)) < 0) {
        // �����ػ�����ʧ��
        FATAL("TurnIntoDaemon failed!");
        return -1;
    } else if (childPid > 0) {
        // �ȴ�ϵͳ����״̬�趨
        usleep(100000);
        return childPid;
    } else {
        // �ػ����̷���
        _Startup();
        exit(0);
    }
}

static void
_Startup() {
    printf("System Starting ...");

    // ��ʼ���������
    if (!_InitSystem()) {
        printf("_Startup failed!");
    }

    // ע��SIG_ABORT�źţ���ִ����ֹǰ���������
    signal(SIGABRT, _Shutdown);

    // ִ������ʱ����
    _OnStartup();

    // ��ʼ����־
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

	// ��ʼ�������û���־
	LogUserInit("log_user_list.txt");

    // ִ��ָ��������
    if (!ExecuteGuideServer()) {
    	printf("Push Server Execute Failed.\n");
    }

    // �ر�����
    _Shutdown(0);
}

static void
_Shutdown(int s) {
    printf("Close main controller.");

    // ִ����ֹʱ����
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
    // ��ʼϵͳʱ��
	UpdateNowTime();
}

static void
_OnShutdown() {
	ExitServerInstance();
	ReleaseLog();
	LogUserClose();
    SetSystemStatus(STOP);
}
