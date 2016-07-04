#ifndef _PROCESS_MANAGER_H
#define _PROCESS_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

int         Abort(int);                         /* 中止指定的进程 */
int         AbortG(int);                        /* 中止指定的进程组 */


#ifdef __cplusplus
}
#endif

#endif  /* _PROCESS_MANAGER_H */
