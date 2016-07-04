/// <summary>
/// 守护进程创建
/// </summary>
/// <creater>wpj</creater>
/// <version>1.0</version>
/// <revision>
///	<record part="" date="" description=""  />
/// </revision>
#ifndef _DAEMON_H
#define _DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

// 建立守护进程
int TurnIntoDaemon(int servfd);


#ifdef __cplusplus
}
#endif

#endif  /* _DAEMON_H */
