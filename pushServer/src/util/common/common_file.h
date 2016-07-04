#ifndef _COMMON_FILE_H
#define _COMMON_FILE_H


#ifdef __cplusplus
extern "C" {
#endif

#include    "common_types.h"
#include    "../util.h"

/*
 * 常量定义
 */
#define     F_MODE_RW       (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
/* -------------------------           */


/*
 * 函数原型
 */
BOOL        IsFileExist(const char*);                               /* 检查文件是否存在 */
const char* GetBaseFileName(const char*);                           /* 返回文件基础名称 */
const char* GetBaseFilePath(char*, const char*);                    /* 返回文件基础路径 */

long        GetFileLength(const char*);                             /* 返回文件长度 */
const struct tm*
            GetFileTime(struct tm*, const char *);                  /* 返回文件修改时间 */
const char* GetFormattedFileTime(const char *);                     /* 返回文件修改时间字符串 */
const time_t GetFileModiTime(const char*);							/* 返回文件的修改时间 */

BOOL        IsDir(const char*);                                     /* 检查文件类型是否是目录 */
BOOL        MkDir(const char*);                                     /* 创建目录 */
BOOL        MvFile(const char*, const char*);                       /* 移动文件 */
BOOL        MvDir(const char*, const char*);                        /* 移动文件夹 */
BOOL        RmFile(const char*);                                    /* 删除文件 */
BOOL        RmDir(const char*);                                     /* 删除文件夹 */
BOOL        LnFile(const char*, const char*);                       /* 建立文件硬链接 */

BOOL        OpenFile(int *pFd, const char*, int);                   /* 打开文件 */
void        CloseFile(int *pFd);                                    /* 关闭文件 */

BOOL        TruncWriteToFile(const char*, const void*, long);       /* 以覆盖的方式写入文件，并在写入后截断文件 */
BOOL        TruncWriteToOpenedFile(int *pFd, const char*,
                    const void*, long);
BOOL        AppendToFile(const char*, const void*, long);           /* 以追加的方式写入文件 */
BOOL        AppendToOpenedFile(int *pFd, const char*,
                    const void*, long);

#ifdef __cplusplus
}
#endif

#endif  /* _COMMON_FILE_H */
