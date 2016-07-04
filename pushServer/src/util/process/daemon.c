#include "daemon.h"
#include "../util.h"
#include "../log/log.h"


/// <summary>
/// 
/// </summary>
/// <param name="sender"></param>
/// <param name="e"></param>
/// <returns></returns>
int 
TurnIntoDaemon(int servfd)
{
	int childpid = 0;
    int fd = 0;
    int fdtablesize = 0;

    if ((childpid = fork()) < 0) {
        FATAL("failed to fork first child");
        return -1;
    } else if (childpid > 0) {
        return childpid;
    }

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    /*
     * dissociate from process group
     */
    if (setsid() < 0) {
        FATAL("failed to become process group leader");
        exit(1);
    }

    /*
     * lose controlling terminal
     */
    if ((fd = open("/dev/tty", O_RDWR)) != -1) {
#ifdef TIOCSCTTY
        ioctl(fd, TIOCNOTTY, NULL);
#endif
        close(fd);
    }

    /*
     * close any open file descriptors
     */
    for (fd = 3, fdtablesize = getdtablesize(); fd < fdtablesize; fd++) {
        if (fd != servfd) {
            close(fd);
        }
    }

    /*
     * 重定向stdin, stdout, stderr
     */
    if ((fd = open("/dev/null", O_RDWR)) != -1) {
        dup2(fd, STDIN_FILENO);

        /* 为输出控制台日志，注释该段代码
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        */

        if (fd > 2) {
            close(fd);
        }
    }

    /*
     * set working directory to allow filesystems to be unmounted
     */
    /* chdir("/"); */

    return 0;
}
