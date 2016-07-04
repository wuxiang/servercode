#ifndef _COMMON_TYPES_H
#define _COMMON_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

// NULL 定义
#ifndef NULL
#define NULL            ((void *) 0)
#endif
// -------------------------           /


// BOOL 类型定义
#ifndef  BOOL
#define BOOL            int
#endif

#ifndef  TRUE
#define TRUE            1
#endif

#ifndef  FALSE
#define FALSE           0
#endif
// -------------------------           /


// long long 类型定义
#undef  LLONG
#define LLONG           long long
// -------------------------           /


// long double 类型定义
#undef  LONG_DOUBLE
#define LONG_DOUBLE   	long double
// -------------------------           /

// INT64 类型定义
#ifndef INT64
	#if defined(WIN32)
		typedef unsigned __int64 UINT64;
		typedef __int64 INT64;
	#else
		typedef unsigned long long UINT64;
		typedef long long INT64;
	#endif
#endif
// -------------------------           /

// DWORD类型定义
#ifdef _USE_SQL
#include <sqltypes.h>
#else
typedef unsigned int   DWORD;
#endif
// -------------------------           /

// BYTE类型定义
#ifndef BYTE
typedef unsigned char   BYTE;
#endif
// -------------------------           /

// WORD类型定义
#ifndef WORD
typedef unsigned short  WORD;
#endif
// -------------------------           /

// UINT类型定义
#ifndef UINT
typedef unsigned int    UINT;
#endif
// -------------------------           /

// byte类型定义
#ifndef byte
typedef unsigned char   byte;
#endif
// -------------------------           /

// SOCKET类型定义
#ifndef SOCKET
typedef int             SOCKET;
#endif
// -------------------------           /

// LONG类型定义
#ifndef LONG
typedef long            LONG;
#endif
// -------------------------           /

#ifndef LOWORD
#define LOWORD(l)           ((WORD)((DWORD)(l) & 0xffff))
#endif

#ifndef HIWORD
#define HIWORD(l)           ((WORD)((DWORD)(l) >> 16))
#endif

#ifndef LOBYTE
#define LOBYTE(w)           ((BYTE)((DWORD)(w) & 0xff))
#endif

#ifndef HIBYTE
#define HIBYTE(w)           ((BYTE)((DWORD)(w) >> 8))
#endif

#ifndef FLOATZERO
#define FLOATZERO        0.0001f
#endif

#ifndef FLOATINVALID   
#define FLOATINVALID     -1.0f
#endif

// 32位time_t类型定义
#ifndef TIME_T32
typedef unsigned int            TIME_T32;
#endif
// -------------------------           /

// 常用缓存长度定义
#define MAX_BLOCK_SIZE          8192            /* 最大文本块长度 */
#define MAX_LINE_SIZE           1024            /* 最大单行文本长度 */
#define MAX_PATH_LEN            256             /* 最大路径长度 */
// -------------------------           /


#ifdef __cplusplus
}
#endif

#endif  /* _COMMON_TYPES_H */
