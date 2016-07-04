#ifndef _PROCESS_CONFIG_H
#define _PROCESS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SINGLE_REQUEST							768*1024		// 单次最大请求数据量
#define MAX_SINGLE_RESPONSE							1024*1024		// 单次最大响应数据量
                                	                        				                            	                        				
#define DEFAULT_LISTEN_BACKLOG						5				// Listen的默认backlog

#define NET_THREAD_TIME_INTERVAL					20				// 网络处理线程缄默时间(单位:毫秒)
#define CALC_THREAD_TIME_INTERVAL					20				// 计算线程缄默时间(单位:毫秒)
#define DYNA_HQTHREAD_TIME_INTERVAL					20				// 请求行情动态数据线程缄默时间(单位:毫秒)
#define DYNA_NEWS_THD_TIME_INTERVAL					20				// 请求新闻公告动态数据线程缄默时间(单位:毫秒)
#define SEND_OUTER_THREAD_TIME_INTERVAL				20				// 发送线程空闲缄默时间(单位:毫秒)

#define JUDGE_INITIAL_INTERVAL						15				// 计算初始化时间间隔(单位:秒)
#define GET_STATCI_HQ_DATA_INTERVAL					3*60			// 取行情静态数据变化时间(单位:秒)

#define HQ_LINK_ALIVE_TIME							90				// 行情链接存活时间(单位:秒)
#define CHECK_HQ_LINK_ALIVE_TIME					2*60			// 扫描行情链接死链时间(单位:秒)

#define S_USER_ALIVE_TIME							90				// 短用户存活时间(单位:秒 超过此界限标识为非活跃)
#define CHECK_USER_ALIVE_TIME						77				// 扫描用户是否存活时间(单位:秒)
#define S_USER_DEFAULT_DEAD_TIME					1440			// 短用户默认死亡时间(单位:分钟 超过此界限标识为死亡 #!!不能小于90秒!!#)
#define L_USER_DEFAULT_DEAD_TIME					14400			// 长用户默认死亡时间(单位:分钟 超过此界限标识为死亡)
#define USER_DEFAULT_CHECK_START					211000			// 默认用户状态状态开始时间(HHMMSS)
#define USER_DEFAULT_CHECK_END						235959			// 默认用户状态状态结束时间(HHMMSS)

#define EARLY_WARNING_TYPE_NUM						5				// 预警种类

#ifdef __cplusplus
}
#endif

#endif  /* _PROCESS_CONFIG_H */
