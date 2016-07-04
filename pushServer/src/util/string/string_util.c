#include "string_util.h"
#include "../common/common_lib.h"
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

/// <summary>
/// ȥ���ַ����Ҷ˵Ŀո�
/// </summary>
/// <param name="pStr"></param>
/// <returns>char*</returns>
char*
RtrimRude(char* pStr) {
    int endPos = strlen(pStr) - 1;

    while (endPos >= 0 && ISSPACE(pStr[endPos])) {
        endPos--;
    }

    pStr[++endPos] = '\0';
    return (char*) pStr;
}

// ����ȥ����˿ո����ַ���
char*
Ltrim(const char* pStr) {
    int beginPos = 0;

    while (ISSPACE(pStr[beginPos])) {
        beginPos++;
    }
    return (char*) (pStr + beginPos);
}

// ȥ���ַ���ǰ��˵Ŀո��޸��ַ�������
char*
TrimRude(char* pStr) {
    return Ltrim(RtrimRude(pStr));
}

/// <summary>
/// ����ת��Ϊ��д����ַ���
/// </summary>
/// <param name="buf">�������</param>
/// <param name="s">����</param>
/// <returns>char*</returns>
char*
ToUppers(char *buf, const char *s) {
    char        *ptr = (char*) NULL;
    int         j = 0;

    if (NULL == s || NULL == buf
    	|| DIM(buf) < strlen(s)) {
        return (char*) NULL;
    }
    
    ptr = buf;
    while (s[j] != '\0') {
        ptr[j] = TOUPPER(s[j]);
        j++;
    }
    ptr[j] = '\0';

    return ptr;
}

/// <summary>
/// ����ת��ΪСд����ַ���
/// </summary>
/// <param name="buf">�������</param>
/// <param name="s">����</param>
/// <returns>char*</returns>
char*
ToLower(char *buf, const char *s) {
    char        *ptr = (char*) NULL;
    int         j = 0;

    if (NULL == s || NULL == buf
    	|| DIM(buf) < strlen(s)) {
        return (char*) NULL;
    }

    ptr = buf;
    while (s[j] != '\0') {
        ptr[j] = TOLOWER(s[j]);
        j++;
    }
    ptr[j] = '\0';

    return ptr;
}

/// <summary>
/// ������ֵתΪ�ַ���
/// </summary>
/// <param name="buf">�������</param>
/// <param name="num">����</param>
/// <returns>char*</returns>
char*
Itoa(char *buf, int num) {
	if (NULL == buf) {
        return (char*) NULL;
    }
    
    sprintf(buf, "%d", num);
    return buf;
}

/// <summary>
/// ��������ֵתΪ�ַ���
/// </summary>
/// <param name="buf">�������</param>
/// <param name="num">����</param>
/// <returns>char*</returns>
char*
Ltoa(char *buf, long num) {
	if (NULL == buf) {
        return (char*) NULL;
    }
    
    sprintf(buf, "%ld", num);
    return buf;
}

/// <summary>
/// LLONG����ֵתΪ�ַ���
/// </summary>
/// <param name="buf">�������</param>
/// <param name="num">����</param>
/// <returns>char*</returns>
char*
Lltoa(char *buf, LLONG num) {
	if (NULL == buf) {
        return (char*) NULL;
    }
    
    sprintf(buf, "%lld", num);
    return buf;
}

/// <summary>
/// long double����ֵתΪ�ַ��� 
/// </summary>
/// <param name="buf"></param>
/// <param name="num"></param>
/// <param name="scale"></param>
/// <returns>char*</returns>
char*
Ldtoa(char *buf, LONG_DOUBLE num, int scale) {
	if (NULL == buf ) {
        return (char*) NULL;
    }
    
	sprintf(buf, "%.*Lf", scale, num + LDTOA_ROUND_WINDAGE);
    return buf;
}

/// <summary>
/// long double����ֵתΪ�ַ��� 
/// </summary>
/// <param name="buf"></param>
/// <param name="num"></param>
/// <param name="scale"></param>
/// <returns>char*</returns>
char*
FixedLdtoa(char *buf, LONG_DOUBLE num, int precision, int scale) {
	if (NULL == buf ) {
        return (char*) NULL;
    }
    
	snprintf(buf, precision + 1, "%.*Lf", scale, num + LDTOA_ROUND_WINDAGE);
    return buf;
}

/// <summary>
/// ת���ַ���ΪLLONG
/// </summary>
/// <param name="str"></param>
/// <returns>LLONG</returns>
LLONG
Atoll(const char *str) {
    LLONG value = 0;

    sscanf(str, "%lld", &value);
    return value;
}

/// <summary>
/// ת���ַ���Ϊlong double
/// </summary>
/// <param name="str"></param>
/// <returns>LONG_DOUBLE</returns>
LONG_DOUBLE
Atold(const char *str) {
    LONG_DOUBLE value = 0.0L;

    sscanf(str, "%Lf", &value);
    return value;
}

/// <summary>
/// ת��ʮ��������ʽ���ַ���Ϊ������ֵ
/// </summary>
/// <param name="str"></param>
/// <returns>int</returns>
int
Atox(const char *str) {
    int value = 0;
    if (NULL == str) {
    	return value;
    }

    sscanf(str, "%xd", &value);
    return value;
}

/// <summary>
/// ������ֵתΪʮ��������ʽ���ַ���
/// </summary>
/// <param name="buf"></param>
/// <param name="num"></param>
/// <returns>char*</returns>
char*
Xtoa(char *buf, int num) {
	if (NULL == buf ) {
        return (char*) NULL;
    }
    
    sprintf(buf, "%x", num);
    return buf;
}

// �����ַ����Ƿ��ǿ��ַ���
BOOL
IsEmptyString(const char *s) {
    int i = 0;

    if (s == NULL) {
        return TRUE;
    }

    while (s[i] != '\0') {
        if (!ISSPACE(s[i])) {
            return FALSE;
        }
        i++;
    }
    return TRUE;
}

// �������ַ�
char*
SkipWhiteSpaces(char **ptr) {
    while (ISSPACE(**ptr)) {
        (*ptr) ++;
    }
    return *ptr;
}

// У���ַ����Ƿ�����Ч����ֵ���ַ���
BOOL
IsNumeric(const char *s) {
    char    *ptr        = (char*) s;
    BOOL    isDecimal   = FALSE;

    if (! ptr) {
        return FALSE;
    }

    SkipWhiteSpaces(&ptr);
    if (*ptr == '\0') {
        return FALSE;
    }

    if (*ptr == '-' || *ptr == '+') {
        ptr++;
    }

    while (*ptr != '\0') {
        if (! ISDIGIT(*ptr)) {
            if (*ptr == '.' && ! isDecimal) {
                isDecimal = TRUE;
            } else if (ISSPACE(*ptr)) {
                while (*(++ptr) != '\0') {
                    if (! ISSPACE(*ptr)) {
                        return FALSE;
                    }
                }
                return TRUE;
            } else {
                return FALSE;
            }
        }
        ptr++;
    }
    return TRUE;
}

// У���ַ����Ƿ�����Ч��ʮ��������ֵ���ַ���
BOOL
IsXNumeric(const char *s) {
    char    *ptr    = (char*) s;

    if (! ptr) {
        return FALSE;
    }

    SkipWhiteSpaces(&ptr);
    if (*ptr == '\0') {
        return FALSE;
    }

    if (*ptr == '-' || *ptr == '+') {
        ptr++;
    }

    if (*(ptr++) != '0' || *(ptr++) != 'x') {
        return FALSE;
    }

    while (*ptr != '\0') {
        if (! ISXDIGIT(*ptr)) {
            if (ISSPACE(*ptr)) {
                while (*(++ptr) != '\0') {
                    if (! ISSPACE(*ptr)) {
                        return FALSE;
                    }
                }
                return TRUE;
            }
            return FALSE;
        }
        ptr++;
    }
    return TRUE;
}

// ����BOOL���ַ���
int
ParseBoolString(const char *s) {
    char    *ptr = (char*) s;
    char    buf[8] = { 0 };
    int     value = 0;

    if (s) {
        RtrimRude(StrCopy(buf, SkipWhiteSpaces(&ptr), 7));

        if (buf[1] == '\0' && (buf[0] == '0' || buf[0] == '1')) {
            return buf[0] == '1';
        }

        if ((value = strncasecmp(buf, "true", 4)) == 0
                || strncasecmp(buf, "false", 5) == 0) {
            return value == 0;
        }
    }
    return -1;
}

// ���ַ���Ϊ���򷵻�ָ����ֵ
const char*
SwitchEmptyString(const char *s, const char *v) {
    return IsEmptyString(s) ? v : s;
}

// У���ַ����Ƿ�����Ч����ֵ���ַ���
int
StringIsNumberType(const char *Buffer,const int Length,const int Decimal) {
    int i = 0;
    int len = Length;
    int dec = Decimal;
    int reallen = strlen(Buffer);

    if( NULL == Buffer ) {
        return -1;
    }

    if (reallen < 1) {
        printf("empty segment��\n");
        return -1;
    }

    if (len < reallen) {
        return -1;
    }

    if (dec > 0) {
        if (Buffer[reallen - dec - 1] != '.') {
            return -1;
        }
    }

    while (i < reallen) {
        if(Buffer[i] == '-' && i == 0) {
            i++;
            continue;
        }

        if (Buffer[i] == '.' || Buffer[i] == ',') {
            if((i + dec + 1) != reallen) {
                return -1;
            }
        } else if ((Buffer[i] < '0') || (Buffer[i] > '9')) {
            return -1;
        }

        i++;
    }
    return 0;
}

// �����������ַ���
char*
StrCopy(char *buf, const char *str, int length) {
	if (NULL == buf || NULL == str) {
		return (char*)NULL;
	}
	
    strncpy(buf, str, length);
    return buf;
}

// ���Ӳ������ַ���
char*
StrCatT(char *buf, const char *str, const char *s2) {
	if (NULL == buf || NULL == str) {
		return (char*)NULL;
	}
	
    sprintf(buf, "%s%s", str, s2);
    return buf;
}

int 
StrNoCaseCmp(char const *str1, char const *str2, unsigned long len)
{
    int c1 = 0, c2 = 0;
    while (len--)
    {
        c1 = tolower(*str1++);
        c2 = tolower(*str2++);

        if (c1 == 0 || c1 != c2)
            break;
    }

    return c1 - c2;
}

