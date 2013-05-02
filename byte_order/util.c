/*
 * Some basic routines, called by dotsd.c
 *
 * (c) Columbia University, 2004-2006, All Rights Reserved.
 * Author: Weibin Zhao
 */

#include "mod_dots.h"

/*
 * Get the next token from "buf", starting from index "ptr"
 * where "buf" MUST be ending with NULL
 * Return: the next token (NULL if no more)
 *         change "ptr" when returns
 */
char *get_token(char *buf, int *ptr)
{
    int   i = *ptr;
    char *start;

    /* find the starting position */
    while (buf[i] != '\0') {
        if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n') {
            i++;
        } else {
            break;
        }
    }
    start = &buf[i];

    /* mark the ending position as NULL */
    while (buf[i] != '\0') {
        if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n') {
            buf[i] = '\0';
            i++;
            break;
        } else {
            i++;
        }
    }
    *ptr = i;
    if (*start == '\0') return NULL;
    return start;
}

/*
 * Read a line from socket
 */
static pthread_key_t  rl_key;
static pthread_once_t rl_once = PTHREAD_ONCE_INIT;

static void readline_destructor(void *ptr)
{
    free(ptr);
}

static void readline_once(void)
{
    pthread_key_create(&rl_key, readline_destructor);
}

typedef struct {
    int   rl_cnt;
    char *rl_bufptr;
    char  rl_buf[MAXLINE];
} Rline;

static ssize_t my_read(Rline *tsd, int fd, char *ptr)
{
    if (tsd->rl_cnt <= 0) {
again:
        if ( (tsd->rl_cnt = read(fd, tsd->rl_buf, MAXLINE)) < 0) {
            if (errno == EINTR)
                goto again;
            return(-1);
        } else if (tsd->rl_cnt == 0)
            return(0);
        tsd->rl_bufptr = tsd->rl_buf;
    }

    tsd->rl_cnt--;
    *ptr = *tsd->rl_bufptr++;
    return(1);
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
    int     n, rc;
    char    c, *ptr;
    Rline   *tsd;

    pthread_once(&rl_once, readline_once);
    if ((tsd = pthread_getspecific(rl_key)) == NULL) {
        tsd = calloc(1, sizeof(Rline));
        pthread_setspecific(rl_key, tsd);
    }
    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        if ( (rc = my_read(tsd, fd, &c)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break;  /* newline is stored, like fgets() */
        } else if (rc == 0) {
            if (n == 1)
                return(0);  /* EOF, no data read */
            else
                break;      /* EOF, some data was read */
        } else
            return(-1);     /* error, errno set by read() */
    }

    *ptr = 0;   /* null terminate like fgets() */
    return(n);
}

/*-----------------------------------------------------------
 * Write integer/long in binary format (network byte order)
 *-----------------------------------------------------------*/

/*
 * See P.49 of C programming
 * Get "n" bits from integer "x", starting from position "p"
 * e.g., getbits(x, 31, 8) -- highest byte
 *       getbits(x,  7, 8) -- lowest  byte
 */
#define getbits(x, p, n) ((x >> (p+1-n)) & ~(~0 << n))

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

bool isBigEndian()
{
	char type = __BYTE_ORDER_INIT;

	M_JUDGE_TYPE  jp;
	jp.ll = 0x12345678;
	type = (jp.c[0] == 0x12 ? __BYTE_ORDER_BIG : __BYTE_ORDER_LITTLE);

	return  (type == __BYTE_ORDER_BIG) ? true : false;
}

uint64_t htonll(uint64_t n)
{
	if (isBigEndian())
	{
		return n;
	}

    return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
//#if __BYTE_ORDER == __BIG_ENDIAN
//    return n;
//#else
//    return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
//#endif
}

uint64_t ntohll(uint64_t n)
{
	if (isBigEndian())
	{
		return n;
	}

    return (((uint64_t)ntohl(n)) << 32) + ntohl(n >> 32);
//#if __BYTE_ORDER == __BIG_ENDIAN
//    return n;
//#else
//    return (((uint64_t)ntohl(n)) << 32) + ntohl(n >> 32);
//#endif
}

#if 1
/*
 * Write an integer "x" to pointer "*buf" in binary format
 * "x" can be written as 1-4 bytes
 * Pointer "*buf" is automatically increased after writing
 */
int write_int(char **buf, int x, int size)
{
    int   t = htonl(x);     /* convert to network byte order */
    char *p = (char *)&t;
    if (size > 4 || size < 1) {
        printf("Allowed size is 1-4\n");
        return 0;
    }
    memcpy(*buf, p+4-size, size);
    *buf += size;
    return size;
}
#endif

#if 0
/*
 * Another implementation of write_int (write high byte first)
 */
int write_int(char **buf, int x, int size)
{
    int i;
    if (size > 4 || size < 1) {
        printf("Allowed size is 1-4\n");
        return 0;
    }
    for (i=size; i>=1; i--) *(*buf)++ = getbits(x, i*8-1, 8);
    return size;
}
#endif

#if 1
/*
 * Write a 64-bit long integer
 */
int write_long(char **buf, uint64_t x)
{
    uint64_t t = htonll(x);     /* convert to network byte order */
    memcpy(*buf, (char *)&t, 8);
    *buf += 8;
    return 8;
}
#endif

#if 0
/*
 * Another implementation of write_long (write high byte first)
 */
int write_long(char **buf, uint64_t x)
{
    int i;
    for (i=8; i>=1; i--) *(*buf)++ = getbits(x, i*8-1, 8);
    return 8;
}
#endif

/*-----------------------------------------------------------
 * Read integer/long in binary format (network byte order)
 *-----------------------------------------------------------*/

#if 1
/*
 * Read an integer from pointer "*buf"
 * The size of the integer can 1-4
 * Pointer "*buf" is automatically increased after reading
 */
int read_int(char **buf, int size)
{
    int x = 0;
    char *p = (char *)&x;
    if (size > 4 || size < 1) {
        printf("Allowed size is 1-4\n");
        return 0;
    }
    memcpy(p+4-size, *buf, size);
    *buf += size;
    return ntohl(x); /* converted to host byte order */
}
#endif

#if 0
/*
 * Another implementation of read_int
 */
int read_int(char **buf, int size)
{
    int x = 0;
    if (size > 4 || size < 1) {
        printf("Allowed size is 1-4\n");
        return 0;
    }
    while (size-- > 0) x = (x << 8) + (*(*buf)++ & 0xff);
    return x;
}
#endif

#if 1
/*
 * Read a 64-bit long integer
 */
uint64_t read_long(char **buf)
{
    uint64_t x = 0;
    memcpy((char *)&x, *buf, 8);
    *buf += 8;
    return ntohll(x); /* converted to host byte order */
}
#endif

#if 0
/*
 * Another implementation of read_long (read high byte first)
 */
uint64_t read_long(char **buf)
{
    uint64_t x = 0;
    int i;
    for (i=0; i<8; i++) x = (x << 8) + (*(*buf)++ & 0xff);
    return x;
}
#endif

void parse_url(char *url, char *host, int *port, char *path)
{
    char *p;

    if (url == NULL || host == NULL || port == NULL || path == NULL) return;
    p = strstr(url, "://");     /* has scheme? */
    if (p == NULL) {
        strcpy(host, url);      /* no scheme */
    } else {
        strcpy(host, p+3);      /* skip the scheme and :// */
    }

    p = strchr(host, '/');      /* has path? */
    if (p == NULL) {
        path[0] = '\0';         /* no path */
    } else {
        *p = '\0';              /* remove path from host */
        strcpy(path, p+1);      /* skip host:port and / */
    }

    p = strchr(host, ':');      /* has port? */
    if (p == NULL) {
        *port = 0;              /* no port */
    } else {
        *p = '\0';              /* remove port from host */
        *port = atoi(p+1);      /* get port */
    }
}
