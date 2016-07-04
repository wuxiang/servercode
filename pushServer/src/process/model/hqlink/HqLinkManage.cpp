#include "HqLinkManage.h"
#include "HqLinkNode.h"
#include "../../../util/util.h"
#include "../../../util/log/log.h"
#include "../../../util/time/time_util.h"
#include "../../../util/net/socket_util.h"
#include "../../../controller/runtime.h"

CHqLinkManage::CHqLinkManage()
	: CDynaNodeManage<HqLinkNode>(GetMaxLinkedHq())
{
}


CHqLinkManage::~CHqLinkManage()
{
}


// ÒÆ³ýËÀÁ´
void
CHqLinkManage::RemoveDeadLinkHq()
{
	HqLinkNode *pLinkNode = GetFirst();
	while(NULL != pLinkNode)
	{
		if (pLinkNode->IsDead())
		{
			RemoveInvalidNode(pLinkNode->GetNodeIndex());
		}
		pLinkNode = GetNext();
	}
}

