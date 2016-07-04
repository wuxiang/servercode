#ifndef _INCLUDE_CODE_CONVERT_H
#define _INCLUDE_CODE_CONVERT_H

#include <iconv.h>

class CCodeConverter
{
private:
	iconv_t m_iconvFd;
public:
	CCodeConverter(const char *,const char *);
	~CCodeConverter();
	
	int Convert(char *, const int, char *, const int);
};

#endif	/* _INCLUDE_CODE_CONVERT_H */

