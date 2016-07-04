#include "SSLCommon.h"

// 初始化运行环境
void 
CSslCommon::Initial()
{
    /* Load the error strings for SSL & CRYPTO APIs */
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    
    /* Load encryption & hashing algorithms for the SSL program */
    SSL_library_init();
}

// 释放欲行环境
void 
CSslCommon::Release()
{
	ERR_free_strings();
	EVP_cleanup();
}
