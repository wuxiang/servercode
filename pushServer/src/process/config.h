#ifndef _PROCESS_CONFIG_H
#define _PROCESS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SINGLE_REQUEST							768*1024		// �����������������
#define MAX_SINGLE_RESPONSE							1024*1024		// ���������Ӧ������
                                	                        				                            	                        				
#define DEFAULT_LISTEN_BACKLOG						5				// Listen��Ĭ��backlog

#define NET_THREAD_TIME_INTERVAL					20				// ���紦���̼߳�Ĭʱ��(��λ:����)
#define CALC_THREAD_TIME_INTERVAL					20				// �����̼߳�Ĭʱ��(��λ:����)
#define DYNA_HQTHREAD_TIME_INTERVAL					20				// �������鶯̬�����̼߳�Ĭʱ��(��λ:����)
#define DYNA_NEWS_THD_TIME_INTERVAL					20				// �������Ź��涯̬�����̼߳�Ĭʱ��(��λ:����)
#define SEND_OUTER_THREAD_TIME_INTERVAL				20				// �����߳̿��м�Ĭʱ��(��λ:����)

#define JUDGE_INITIAL_INTERVAL						15				// �����ʼ��ʱ����(��λ:��)
#define GET_STATCI_HQ_DATA_INTERVAL					3*60			// ȡ���龲̬���ݱ仯ʱ��(��λ:��)

#define HQ_LINK_ALIVE_TIME							90				// �������Ӵ��ʱ��(��λ:��)
#define CHECK_HQ_LINK_ALIVE_TIME					2*60			// ɨ��������������ʱ��(��λ:��)

#define S_USER_ALIVE_TIME							90				// ���û����ʱ��(��λ:�� �����˽��ޱ�ʶΪ�ǻ�Ծ)
#define CHECK_USER_ALIVE_TIME						77				// ɨ���û��Ƿ���ʱ��(��λ:��)
#define S_USER_DEFAULT_DEAD_TIME					1440			// ���û�Ĭ������ʱ��(��λ:���� �����˽��ޱ�ʶΪ���� #!!����С��90��!!#)
#define L_USER_DEFAULT_DEAD_TIME					14400			// ���û�Ĭ������ʱ��(��λ:���� �����˽��ޱ�ʶΪ����)
#define USER_DEFAULT_CHECK_START					211000			// Ĭ���û�״̬״̬��ʼʱ��(HHMMSS)
#define USER_DEFAULT_CHECK_END						235959			// Ĭ���û�״̬״̬����ʱ��(HHMMSS)

#define EARLY_WARNING_TYPE_NUM						5				// Ԥ������

#ifdef __cplusplus
}
#endif

#endif  /* _PROCESS_CONFIG_H */
