#ifndef _STRING_UTIL_H
#define _STRING_UTIL_H


#ifdef __cplusplus
extern "C" {
#endif

#include    "../common/common_types.h"
#include <ctype.h>

char* TrimRude(char*);                              // ȥ���ַ���ǰ��˵Ŀո��޸��ַ�������
char* RtrimRude(char*);								// ȥ���ַ����Ҷ˵Ŀո�(�޸��ַ�������)
char* Ltrim(const char*);                           // ����ȥ����˿ո����ַ���

char* ToUppers(char *, const char*);       			// ����ת��Ϊ��д����ַ���
char* ToLower(char *, const char*);       			// ����ת��ΪСд����ַ���


char* Itoa(char *, int);                  			// ������ֵתΪ�ַ���
char* Ltoa(char *, long);                 			// ��ֵתΪ�ַ���

char* Lltoa(char *, LLONG);               			// ��������ֵתΪ�ַ���
char* Ldtoa(char *, LONG_DOUBLE, int);    			// long double����ֵתΪ�ַ���                    
char* FixedLdtoa(char *, LONG_DOUBLE, 				// long double����ֵתΪ�ַ���
					int, int);

LLONG           Atoll(const char*);                 // �ַ���תΪ��������ֵ
LONG_DOUBLE     Atold(const char*);                 // �ַ���תΪlong double����ֵ

char* Xtoa(char *, int);                  			// ������ֵתΪʮ��������ʽ���ַ���
int             Atox(const char*);                  // ת��ʮ��������ʽ���ַ���Ϊ������ֵ

char*           SkipWhiteSpaces(char **ptr);       	// �������ַ�


BOOL            IsEmptyString(const char*);         // �����ַ����Ƿ��ǿ��ַ���
BOOL            IsNumeric(const char*);             // У���ַ����Ƿ�����Ч����ֵ���ַ���
BOOL            IsXNumeric(const char*);            // У���ַ����Ƿ�����Ч��ʮ��������ֵ���ַ���
int             ParseBoolString(const char*);       // ����BOOL���ַ���

const char*     SwitchEmptyString(const char*, 
					const char*);   				// ���ַ���Ϊ���򷵻�ָ����ֵ

char*       StrCopy(char*, const char*, int);      	// �����������ַ���
char*       StrCat(char*, const char*, 				// ���Ӳ������ַ���
						const char*);       


int             StringIsNumberType(const char *,    // У���ַ����Ƿ�����Ч����ֵ���ַ���
                        const int,const int);
                        
int StrNoCaseCmp(char const *, char const *,		// ���Դ�Сд�ȽϺ���
				unsigned long);

#ifdef __cplusplus
}
#endif

#endif  /* _STRING_UTIL_H */
