#ifndef _MAIN_PROCESS_H
#define _MAIN_PROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

bool ExecuteGuideServer(void);				// 开始执行指标计算服务
void ExitServerInstance();					// 执行退出时的资源清理

#ifdef __cplusplus
}
#endif

#endif  /* _MAIN_PROCESS_H */
