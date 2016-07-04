#ifndef _INCLUDE_SSL_COMMON_H
#define _INCLUDE_SSL_COMMON_H

#include <assert.h>
#include "openssl/pem.h"
#include "openssl/rsa.h"
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"

typedef struct SSL_Connection{
    /* SSL Vars */
    SSL_CTX         *ctx;
    SSL             *ssl;
    SSL_METHOD      *meth;
    
    SSL_Connection()
    {
    	ctx = NULL;
    	ssl = NULL;
    	meth = NULL;
    }
};

class CSslCommon
{
public:
	// 初始化运行环境
	static void Initial();
	// 释放欲行环境
	static void Release();
};


#endif	/* _INCLUDE_SSL_COMMON_H */

