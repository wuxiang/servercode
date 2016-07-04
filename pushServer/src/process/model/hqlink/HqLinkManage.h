#ifndef _INCLUDE_HQLINK_MANAGE_H
#define _INCLUDE_HQLINK_MANAGE_H

#include "../template/DynaNodeManage.h"
class HqLinkNode;

class CHqLinkManage : public CDynaNodeManage<HqLinkNode>
{
public:
	CHqLinkManage();
	~CHqLinkManage();
	
	void RemoveDeadLinkHq();							// “∆≥˝À¿¡¥
};

#endif	/* _INCLUDE_HQLINK_MANAGE_H */

