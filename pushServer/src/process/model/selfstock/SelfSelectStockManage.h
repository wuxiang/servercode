#ifndef _INCLUDE_SELF_SELECT_STOCK_MANAGE_H
#define _INCLUDE_SELF_SELECT_STOCK_MANAGE_H

class CSelfSelectStock;

class CSelfSelectStockManage
{
private:
	CSelfSelectStockManage();
	~CSelfSelectStockManage();
	
private:
	static CSelfSelectStock *m_pAndroidSelfStkManage;
	static CSelfSelectStock *m_pIosSelfStkManage;
	static CSelfSelectStock *m_pWp7SelfStkManage;
	static CSelfSelectStock *m_pWin8SelfStkManage;
	
private:
	// 初始化资源
	static const int CreateRes();
	
public:
	// 初始化
	static bool InitialSelfStk();
	// 释放资源
	static void ReleaseSelfStk();
	// 获取指定平台的记录管理
	static CSelfSelectStock* GetPlatformManage(const int);
	// 重置
	static void ResetConfigMark();
};

#endif		/* _INCLUDE_SELF_SELECT_STOCK_MANAGE_H */
