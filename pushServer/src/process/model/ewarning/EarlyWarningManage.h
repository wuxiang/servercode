#ifndef _INCLUDE_EWARNING_MANAGE_H
#define _INCLUDE_EWARNING_MANAGE_H

#include "../../../util/util.h"

class CPlatformEWarnManage;
class CEarlyWarningConditionRecord;

class CEwarningManage
{
private:
	CEwarningManage();
	~CEwarningManage();
	
private:
	// Android平台用户历史记录信息
	static CPlatformEWarnManage *m_pAndroidEWarn;
	// Ios平台用户历史记录信息
	static CPlatformEWarnManage *m_pIosEWarn;
	// Wp7平台用户历史记录信息
	static CPlatformEWarnManage *m_pWp7EWarn;
	// Win8平台用户历史记录信息
	static CPlatformEWarnManage *m_pWin8EWarn;
	
private:
	// 初始化资源
	static const int CreateRes();
	
public:
	// 初始化
	static bool InitialEWarn();
	// 释放资源
	static void ReleaseEWarn();
	// 获取指定平台的预警记录管理
	static CPlatformEWarnManage *GetPlatFormEWarn(const int);
	// 更新预警记录占用标识
	static int UpdateSetEwarnOccupy(const unsigned char, const unsigned int);
	// 整理无效记录
	static void RearrangeNode();
};

#endif	/* _INCLUDE_EWARNING_MANAGE_H */

