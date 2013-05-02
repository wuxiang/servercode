#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

//extern unsigned long long  htonll(const unsigned long long src);
//extern unsigned long long  ntohll(const unsigned long long src);

enum M_BYTE_ORDER
{
	__BYTE_ORDER_INIT = 0,
	__BYTE_ORDER_BIG,
	__BYTE_ORDER_LITTLE,
};

typedef union M_JUDGE_TYPE{
	unsigned  long int ll;
	unsigned char  c[4];
}M_JUDGE_TYPE;

void swapChar(unsigned char*  a, unsigned char* b)
{
	unsigned char tmp = *a;
	*a = *b;
	*b = tmp;
}

//unsigned long long  m_convert(const unsigned long long src)
unsigned long long  m_convert(const unsigned int src)
{
	char type = __BYTE_ORDER_INIT;

	M_JUDGE_TYPE  jp;
	jp.ll = 0x12345678;
	type = (jp.c[0] == 0x12 ? __BYTE_ORDER_BIG : __BYTE_ORDER_LITTLE);

	if (type == __BYTE_ORDER_BIG)
	{
		return src;
	}

	return ((((uint32_t)(src) & 0xff000000) >> 24) |
			(((uint32_t)(src) & 0xff0000) >> 8)    |
			(((uint32_t)(src) & 0xff00) << 8)      |
			(((uint32_t)(src) & 0xff) << 24)
		   );

}

//unsigned long long  htonll(const unsigned long long src)
unsigned long long  htonll(const unsigned int src)
{
	return m_convert(src);
}

//unsigned long long ntohll(const unsigned long long src)
unsigned long long ntohll(const unsigned int src)
{
	return m_convert(src);
}


int main()
{
	unsigned int  val = 123456;
	unsigned int exl = htonll(val);
	unsigned int sys = htonl(val);

	printf("htonll: %d\t%d\t%d\n", exl, sys, val);

	printf("%d\t%d\n", ntohl(exl), ntohll(exl));
	//val = ntohll(exl);
	//printf("ntohll:%d\n", val);
	return 0;
}
