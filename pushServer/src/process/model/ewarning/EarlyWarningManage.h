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
	// Androidƽ̨�û���ʷ��¼��Ϣ
	static CPlatformEWarnManage *m_pAndroidEWarn;
	// Iosƽ̨�û���ʷ��¼��Ϣ
	static CPlatformEWarnManage *m_pIosEWarn;
	// Wp7ƽ̨�û���ʷ��¼��Ϣ
	static CPlatformEWarnManage *m_pWp7EWarn;
	// Win8ƽ̨�û���ʷ��¼��Ϣ
	static CPlatformEWarnManage *m_pWin8EWarn;
	
private:
	// ��ʼ����Դ
	static const int CreateRes();
	
public:
	// ��ʼ��
	static bool InitialEWarn();
	// �ͷ���Դ
	static void ReleaseEWarn();
	// ��ȡָ��ƽ̨��Ԥ����¼����
	static CPlatformEWarnManage *GetPlatFormEWarn(const int);
	// ����Ԥ����¼ռ�ñ�ʶ
	static int UpdateSetEwarnOccupy(const unsigned char, const unsigned int);
	// ������Ч��¼
	static void RearrangeNode();
};

#endif	/* _INCLUDE_EWARNING_MANAGE_H */

