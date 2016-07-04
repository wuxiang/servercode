#ifndef _COMMON_FILE_H
#define _COMMON_FILE_H


#ifdef __cplusplus
extern "C" {
#endif

#include    "common_types.h"
#include    "../util.h"

/*
 * ��������
 */
#define     F_MODE_RW       (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
/* -------------------------           */


/*
 * ����ԭ��
 */
BOOL        IsFileExist(const char*);                               /* ����ļ��Ƿ���� */
const char* GetBaseFileName(const char*);                           /* �����ļ��������� */
const char* GetBaseFilePath(char*, const char*);                    /* �����ļ�����·�� */

long        GetFileLength(const char*);                             /* �����ļ����� */
const struct tm*
            GetFileTime(struct tm*, const char *);                  /* �����ļ��޸�ʱ�� */
const char* GetFormattedFileTime(const char *);                     /* �����ļ��޸�ʱ���ַ��� */
const time_t GetFileModiTime(const char*);							/* �����ļ����޸�ʱ�� */

BOOL        IsDir(const char*);                                     /* ����ļ������Ƿ���Ŀ¼ */
BOOL        MkDir(const char*);                                     /* ����Ŀ¼ */
BOOL        MvFile(const char*, const char*);                       /* �ƶ��ļ� */
BOOL        MvDir(const char*, const char*);                        /* �ƶ��ļ��� */
BOOL        RmFile(const char*);                                    /* ɾ���ļ� */
BOOL        RmDir(const char*);                                     /* ɾ���ļ��� */
BOOL        LnFile(const char*, const char*);                       /* �����ļ�Ӳ���� */

BOOL        OpenFile(int *pFd, const char*, int);                   /* ���ļ� */
void        CloseFile(int *pFd);                                    /* �ر��ļ� */

BOOL        TruncWriteToFile(const char*, const void*, long);       /* �Ը��ǵķ�ʽд���ļ�������д���ض��ļ� */
BOOL        TruncWriteToOpenedFile(int *pFd, const char*,
                    const void*, long);
BOOL        AppendToFile(const char*, const void*, long);           /* ��׷�ӵķ�ʽд���ļ� */
BOOL        AppendToOpenedFile(int *pFd, const char*,
                    const void*, long);

#ifdef __cplusplus
}
#endif

#endif  /* _COMMON_FILE_H */
