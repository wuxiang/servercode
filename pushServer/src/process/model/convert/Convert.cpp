#include "Convert.h"
#include "../../../util/util.h"

CCodeConverter::CCodeConverter(const char *from_charset,const char *to_charset)
	: m_iconvFd((iconv_t)-1)
{
	m_iconvFd = iconv_open(to_charset, from_charset);
}

CCodeConverter::~CCodeConverter()
{
	if (m_iconvFd > (iconv_t)-1)
	{
		iconv_close(m_iconvFd);
	}
}

int 
CCodeConverter::Convert(char *inbuf, const int inlen, char *outbuf, const int outlen)
{
	char **pin = &inbuf;
	char **pout = &outbuf;	
	memset(outbuf, 0, outlen);
	
	return iconv(m_iconvFd, pin, (size_t *)&inlen, pout, (size_t *)&outlen);
}

