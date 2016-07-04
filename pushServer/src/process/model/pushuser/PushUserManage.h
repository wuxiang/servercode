#ifndef _INCLUDE_PUSH_USER_MANAGE_H
#define _INCLUDE_PUSH_USER_MANAGE_H

#include "../../../util/util.h"
#include "../../data_type.h"

class CAndroidPushUserManage;
class CIosPushUserManage;
class CWp7PushUserManage;
class CWin8PushUserManage;

class CPushUserManage
{
private:
	CPushUserManage();
	~CPushUserManage();
	
private:
	// Androidƽ̨�û���Ϣ
	static CAndroidPushUserManage *m_pAndroidUser;
	// Iosƽ̨�û���Ϣ
	static CIosPushUserManage *m_pIosUser;
	// Wp7ƽ̨�û���Ϣ
	static CWp7PushUserManage *m_pWp7User;
	// Win8ƽ̨�û���Ϣ
	static CWin8PushUserManage *m_pWin8User;
	
private:
	// ��ʼ����Դ
	static const int CreateRes();
	
public:	
	// ��ʼ��
	static bool Initial();
	// �ͷ���Դ
	static void Release();
	// ��ȡָ��ƽ̨���û�����
	template<typename T>
	static T* GetPlatFormUserManage(const enum PlatFormClassifiedCodeEnum PlatformClassified)
	{
		void* pOut = NULL;
		switch(PlatformClassified)
		{
			case PFCC_WP7:
				pOut = m_pWp7User;
				break;
			
			case PFCC_ANDROID:
				pOut = m_pAndroidUser;
				break;
				
			case PFCC_IOS:
				pOut = m_pIosUser;
				break;
				
			case PFCC_WIN8:
				pOut = m_pWin8User;
				break;
				
			default:
				break;
		}
		
		return (T*)pOut;
	}
	
	// ����ƽ̨��¼
	static void RearrangeUserNode();
	// ͬ����¼�����ݿ�
	static int SyncLatestToDb();
	// ��ӡ���µ��û���Ϣռ�����
	static void PrintLatesUserInfo();
	// ��λ��Ծ�ڴ��û�����
	static int SetMemActiveUserDead(const char*, const unsigned char);
};

#endif	/* _INCLUDE_PUSH_USER_MANAGE_H */

