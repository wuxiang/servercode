#include "ErrorMsg.h"
#include <string>
#include <map>

using namespace std;

map<int, string>::value_type map_init_data[] =
{
	map<int, string>::value_type(1,			"请求数据格式存在错误！"),
	map<int, string>::value_type(2, 		"预警记录数达到最大！"),
	map<int, string>::value_type(3, 		"您的预警数已经达到上限，请您删除部分预警后再添加！"),
	map<int, string>::value_type(4, 		"添加当前加预警记录发生未知错误！"),
	map<int, string>::value_type(5, 		"所选择的证券目前暂时不支持预警功能！"),
		
	map<int, string>::value_type(6, 		"接收的验证信息不完整"),
	map<int, string>::value_type(7, 		"用户名或密码错误!"),
	map<int, string>::value_type(8, 		"客户端标识不匹配!"),
	map<int, string>::value_type(9, 		""),
	map<int, string>::value_type(10, 		""),
		
	map<int, string>::value_type(11,		"尚未设定预警记录,不能删除！"),
	map<int, string>::value_type(12, 		"指定ID的预警记录不存在！"),
	map<int, string>::value_type(13, 		"删除指定ID的预警记录时发生未知错误！"),
	map<int, string>::value_type(14, 		"预警服务器资源紧张，请稍后重试"),
	map<int, string>::value_type(15, 		""),
		
	map<int, string>::value_type(16, 		"推送令牌不能为空，请确定后再传送！"),
	map<int, string>::value_type(17, 		""),
	map<int, string>::value_type(18, 		""),
	map<int, string>::value_type(19, 		""),
	map<int, string>::value_type(20, 		""),
		
	map<int, string>::value_type(21,		"添加的记录应该已经存在，但未找到匹配的原有记录"),
	map<int, string>::value_type(22, 		"服务器正在初始化，请稍后重试！"),
	map<int, string>::value_type(23, 		""),
	map<int, string>::value_type(24, 		""),	// 已经被占用，应客户端要求返回空字符串	
	map<int, string>::value_type(25, 		""),
		
	map<int, string>::value_type(26, 		"解析子类型包头出错"),
	map<int, string>::value_type(27, 		"解析用户上传的标识信息出错"),
	map<int, string>::value_type(28, 		"添加用户管理出错，已达到设定的最大处理能力"),
	map<int, string>::value_type(29, 		"分配到处理线程出错"),
	map<int, string>::value_type(30, 		"定位到子类型包头指定的长度失败"),
	
	map<int, string>::value_type(31, 		"服务端尚未实现此接口，请等待升级！"),
	map<int, string>::value_type(32, 		"传输的平台类型尚未实现，请等待升级！"),
	map<int, string>::value_type(33, 		"回执信息存在错误，请修正后重新请求！"),
	map<int, string>::value_type(34, 		"目前用户状态异常，请确认或重新注册！"),
	map<int, string>::value_type(35, 		"获取公告出现异常，请确定后重试!"),
		
	map<int, string>::value_type(36, 		"自选股服务器异常，请稍后重试!"),
	map<int, string>::value_type(37, 		"所选自选股服务器目前尚未支持，请等待升级!"),
	map<int, string>::value_type(38, 		"添加自选股失败，请确定上传的数据是否正确!"),
	map<int, string>::value_type(39, 		"删除自选股失败，请确定上传的数据是否正确!")
};

const map<int, string> sPushErrorMsgMap(map_init_data, map_init_data + 39);

// 获取推送定义的错误消息
const char* 
GetPushErrorMsg(const int ErrorCode)
{
	map<int, string>::const_iterator FindIter = sPushErrorMsgMap.find(ErrorCode);
	if (FindIter != sPushErrorMsgMap.end())
	{
		return FindIter->second.c_str();
	}
	
	return NULL;
}


