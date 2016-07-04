#ifndef _COMMON_LIB_H
#define _COMMON_LIB_H


#ifdef __cplusplus
extern "C" {
#endif

#include    <stdlib.h>
#include    <string.h>
#include    <math.h>

/*
 * 常量定义
 */
#define     LDTOA_ROUND_WINDAGE     0.00000001          /* 用于Ldtoa函数的舍入精度偏差 */
#define     LDTOA_ROUND_OFFSET      0.50000001          /* 用于Ldtoa函数的舍入精度偏差 */
/* -------------------------           */

/*
 * 宏函数定义
 */

/* bzero */
#ifndef bzero
# define    bzero(s,len)    memset((s), 0, (len))
#endif


/* MIN & MAX */
#undef  MIN
#define     MIN(a,b)        ((a) < (b) ? (a) : (b))
#undef  MAX
#define     MAX(a,b)        ((a) > (b) ? (a) : (b))

#undef  ABS
#define     ABS(x)          ((x) < 0 ? -(x) : (x))

#undef  XOR
#define     XOR(e,f)        (((e) && !(f)) || (!(e) && (f)))

#undef  ROUND
#define     ROUND(f)        ((long)(f + LDTOA_ROUND_OFFSET))
#undef  LROUND
#define     LROUND(f)       ((long)(f + LDTOA_ROUND_OFFSET))
#undef  LLROUND
#define     LLROUND(f)      ((long long)(f + LDTOA_ROUND_OFFSET))

#undef  FLOOR
#define     FLOOR(f)        ((long)(f + LDTOA_ROUND_WINDAGE))
#undef  CEIL
#define     CEIL(f)         ((f) - (long)(f) > 0.00001 ? (long)(f) + 1 : (long)(f))


/*
 * 字符处理
 */

#undef  ISASCII
#define     ISASCII(c)      isascii(c)

#ifdef isupper
# define    ISUPPER(c)      isupper(c)
#else
# define    ISUPPER(c)      ('A' <= (c) && (c) <= 'Z')
#endif

#ifdef islower
# define    ISLOWER(c)      islower(c)
#else
# define    ISLOWER(c)      ('a' <= (c) && (c) <= 'z')
#endif

#ifdef isdigit
# define    ISDIGIT(c)      isdigit(c)
#else
# define    ISDIGIT(c)      ('0' <= (c) && (c) <= '9')
#endif

#ifdef isalpha
# define    ISALPHA(c)      isalpha(c)
#else
# define    ISALPHA(c)      (ISUPPER(c) || ISLOWER(c))
#endif

#ifdef isalnum
# define    ISALNUM(c)      isalnum(c)
#else
# define    ISALNUM(c)      (ISDIGIT(c) || ISALPHA(c))
#endif

#ifdef isxdigit
# define    ISXDIGIT(c)     isxdigit(c)
#else
# define    ISXDIGIT(c)     (ISDIGIT(c) || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))
#endif

#ifdef isblank
# define    ISBLANK(c)      isblank(c)
#else
# define    ISBLANK(c)      ((c) == ' ' || (c) == '\t')
#endif

#ifdef isspace
# define    ISSPACE(c)      isspace(c)
#else
# define    ISSPACE(c)      ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
#endif

#ifdef toupper
# define    TOUPPER(c)      toupper(c)
#else
# define    TOUPPER(c)      (ISLOWER(c) ? 'A' + ((c) - 'a') : (c))
#endif

#ifdef tolower
# define    TOLOWER(c)      tolower(c)
#else
# define    TOLOWER(c)      (ISUPPER(c) ? 'a' + ((c) - 'A') : (c))
#endif
/* -------------------------           */



/* 计算数组长度 */
#define     DIM(x)                          (sizeof((x)) / sizeof((x)[0]))


/* 若x小于0则返回0 */
#define     TRIM_ZERO(x)                    ((x) < 0 ? 0 : (x))


/* 若x小于等于0则返回v */
#define     SWITCH_SMALLNESS_NUMBER(x,v)    ((x) <= 0 ? (v) : (x))

/* 若x为空则返回v */
#define     SWITCH_NULL(x,v)                ((void*) (x) == (void*) 0 ? (v) : (x))

// 位操作
#ifndef BitSet
#define BitSet(x,n)							(x |= (1<<n))		// 置位
#endif

#ifndef BitUnSet
#define BitUnSet(x,n)						(x &= (~(1<<n)))	// 反置位
#endif

#ifndef IsBitSet
#define IsBitSet(x,n)						(x &(1<<n))			// 是否置位
#endif

#ifndef BitRevers
#define BitRevers(x,n)						(x ^= (1<<n))		// 翻转位
#endif
/// -----------------------			/


#ifdef __cplusplus
}
#endif

#endif  /* _COMMON_LIB_H */
