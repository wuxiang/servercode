#ifndef _STRING_UTIL_H
#define _STRING_UTIL_H


#ifdef __cplusplus
extern "C" {
#endif

#include    "../common/common_types.h"
#include <ctype.h>

char* TrimRude(char*);                              // 去除字符串前后端的空格（修改字符串本身）
char* RtrimRude(char*);								// 去除字符串右端的空格(修改字符串本身)
char* Ltrim(const char*);                           // 返回去除左端空格后的字符串

char* ToUppers(char *, const char*);       			// 返回转换为大写后的字符串
char* ToLower(char *, const char*);       			// 返回转换为小写后的字符串


char* Itoa(char *, int);                  			// 整型数值转为字符串
char* Ltoa(char *, long);                 			// 数值转为字符串

char* Lltoa(char *, LLONG);               			// 长整型数值转为字符串
char* Ldtoa(char *, LONG_DOUBLE, int);    			// long double型数值转为字符串                    
char* FixedLdtoa(char *, LONG_DOUBLE, 				// long double型数值转为字符串
					int, int);

LLONG           Atoll(const char*);                 // 字符串转为长整型数值
LONG_DOUBLE     Atold(const char*);                 // 字符串转为long double型数值

char* Xtoa(char *, int);                  			// 整型数值转为十六进制形式的字符串
int             Atox(const char*);                  // 转换十六进制形式的字符串为整型数值

char*           SkipWhiteSpaces(char **ptr);       	// 跳过空字符


BOOL            IsEmptyString(const char*);         // 返回字符串是否是空字符串
BOOL            IsNumeric(const char*);             // 校验字符串是否是有效的数值型字符串
BOOL            IsXNumeric(const char*);            // 校验字符串是否是有效的十六进制数值型字符串
int             ParseBoolString(const char*);       // 解析BOOL型字符串

const char*     SwitchEmptyString(const char*, 
					const char*);   				// 若字符串为空则返回指定的值

char*       StrCopy(char*, const char*, int);      	// 拷贝并返回字符串
char*       StrCat(char*, const char*, 				// 连接并返回字符串
						const char*);       


int             StringIsNumberType(const char *,    // 校验字符串是否是有效的数值型字符串
                        const int,const int);
                        
int StrNoCaseCmp(char const *, char const *,		// 忽略大小写比较函数
				unsigned long);

#ifdef __cplusplus
}
#endif

#endif  /* _STRING_UTIL_H */
