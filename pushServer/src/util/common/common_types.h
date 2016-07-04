#ifndef _COMMON_TYPES_H
#define _COMMON_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

// NULL ����
#ifndef NULL
#define NULL            ((void *) 0)
#endif
// -------------------------           /


// BOOL ���Ͷ���
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


// long long ���Ͷ���
#undef  LLONG
#define LLONG           long long
// -------------------------           /


// long double ���Ͷ���
#undef  LONG_DOUBLE
#define LONG_DOUBLE   	long double
// -------------------------           /

// INT64 ���Ͷ���
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

// DWORD���Ͷ���
#ifdef _USE_SQL
#include <sqltypes.h>
#else
typedef unsigned int   DWORD;
#endif
// -------------------------           /

// BYTE���Ͷ���
#ifndef BYTE
typedef unsigned char   BYTE;
#endif
// -------------------------           /

// WORD���Ͷ���
#ifndef WORD
typedef unsigned short  WORD;
#endif
// -------------------------           /

// UINT���Ͷ���
#ifndef UINT
typedef unsigned int    UINT;
#endif
// -------------------------           /

// byte���Ͷ���
#ifndef byte
typedef unsigned char   byte;
#endif
// -------------------------           /

// SOCKET���Ͷ���
#ifndef SOCKET
typedef int             SOCKET;
#endif
// -------------------------           /

// LONG���Ͷ���
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

// 32λtime_t���Ͷ���
#ifndef TIME_T32
typedef unsigned int            TIME_T32;
#endif
// -------------------------           /

// ���û��泤�ȶ���
#define MAX_BLOCK_SIZE          8192            /* ����ı��鳤�� */
#define MAX_LINE_SIZE           1024            /* ������ı����� */
#define MAX_PATH_LEN            256             /* ���·������ */
// -------------------------           /


#ifdef __cplusplus
}
#endif

#endif  /* _COMMON_TYPES_H */
