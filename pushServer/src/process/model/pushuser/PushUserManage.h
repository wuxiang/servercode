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
	// Android平台用户信息
	static CAndroidPushUserManage *m_pAndroidUser;
	// Ios平台用户信息
	static CIosPushUserManage *m_pIosUser;
	// Wp7平台用户信息
	static CWp7PushUserManage *m_pWp7User;
	// Win8平台用户信息
	static CWin8PushUserManage *m_pWin8User;
	
private:
	// 初始化资源
	static const int CreateRes();
	
public:	
	// 初始化
	static bool Initial();
	// 释放资源
	static void Release();
	// 获取指定平台的用户管理
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
	
	// 整理平台记录
	static void RearrangeUserNode();
	// 同步记录至数据库
	static int SyncLatestToDb();
	// 打印最新的用户信息占用情况
	static void PrintLatesUserInfo();
	// 置位活跃内存用户死亡
	static int SetMemActiveUserDead(const char*, const unsigned char);
};

#endif	/* _INCLUDE_PUSH_USER_MANAGE_H */

