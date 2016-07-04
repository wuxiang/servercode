#include "common_file.h"
#include "../util.h"
#include "../string/string_util.h"
#include "common_lib.h"

///////////////////宏及常量//////////////////
#ifndef DIR_SEPARATOR
    #define DIR_SEPARATOR '/'							
#endif

#ifndef DIR_SEPARATOR_2
    #define IS_DIR_SEPARATOR(ch) ((ch) == DIR_SEPARATOR)
#else
    #define IS_DIR_SEPARATOR(ch) \
            (((ch) == DIR_SEPARATOR) || ((ch) == DIR_SEPARATOR_2))
#endif

// -------------------------           //



/// <summary>
/// 检查文件是否存在
/// </summary>
/// <param name="filePath"></param>
/// <returns>BOOL</returns>
BOOL
IsFileExist(const char *filePath) {
    return access(filePath, F_OK) < 0 ? FALSE : TRUE;
}

/// <summary>
/// 返回文件基础名称
/// </summary>
/// <param name="filePath"></param>
/// <returns>const char*</returns>
const char*
GetBaseFileName(const char *filePath) {
    const char *base = filePath;

#if defined (HAVE_DOS_BASED_FILE_SYSTEM)
    if (ISALPHA(filePath[0]) && filePath[1] == ':') {
        filePath += 2;
    }
#endif

    for (base = filePath; *filePath; filePath++) {
        if (IS_DIR_SEPARATOR (*filePath)) {
            base = filePath + 1;
        }
    }
    return base;
}

/// <summary>
/// 返回文件基础路径
/// </summary>
/// <param name="buf"></param>
/// <param name="filePath"></param>
/// <returns>const char*</returns>
const char*
GetBaseFilePath(char *buf, const char *filePath) {
    const char *o = filePath;
    const char *base = filePath;

#if defined (HAVE_DOS_BASED_FILE_SYSTEM)
    if (ISALPHA(filePath[0]) && filePath[1] == ':') {
        filePath += 2;
    }
#endif

    for (base = filePath; *filePath; filePath++) {
        if (IS_DIR_SEPARATOR (*filePath)) {
            base = filePath;
        }
    }

    strncpy(buf, o, base - o);
    buf[base - o] = '\0';

    return buf;
}

/// <summary>
/// 返回文件长度
/// </summary>
/// <param name="filePath"></param>
/// <returns>long</returns>
long
GetFileLength(const char *filePath) {
    struct stat     f_stat;

    if (stat(filePath, &f_stat) < 0) {
        return -1;
    }
    return (long) f_stat.st_size;
}

/// <summary>
/// 返回文件修改时间
/// </summary>
/// <param name="tm"></param>
/// <param name="filePath"></param>
/// <returns>const struct tm*</returns>
const struct tm*
GetFileTime(struct tm *tm, const char *filePath) {
    struct stat         f_stat;

    if (stat(filePath, &f_stat) < 0) {
        return (const struct tm*) NULL;
    }

    memcpy(tm, localtime(&f_stat.st_mtime), sizeof(struct tm));
    return tm;
}

/// <summary>
/// 返回文件修改时间
/// </summary>
/// <param name="filePath"></param>
/// <returns>const time_t</returns>
const time_t
GetFileModiTime(const char *filePath) {
	struct stat         f_stat;

    if (stat(filePath, &f_stat) < 0) {
        return (time_t) 0;
    }
    return f_stat.st_mtime;
}

/// <summary>
/// 检查文件类型是否是目录
/// </summary>
/// <param name="filePath"></param>
/// <returns>BOOL</returns>
BOOL
IsDir(const char *filePath) {
    struct stat     buf;

    if (lstat(filePath, &buf) < 0) {
        return FALSE;
    }

    return S_ISDIR(buf.st_mode) ? TRUE: FALSE;
}

/// <summary>
/// 创建目录
/// </summary>
/// <param name="dir"></param>
/// <returns>BOOL</returns>
BOOL
MkDir(const char *dir) {
    char    cmd[256];

    #define _CMD_MKDIR   "mkdir -p %s"

    if (!IsDir(dir)) {
        sprintf(cmd, _CMD_MKDIR, dir);
        system(cmd);

        return IsDir(dir);
    }
    return TRUE;
}

/// <summary>
/// 移动文件
/// </summary>
/// <param name="source"></param>
/// <param name="target"></param>
/// <returns>BOOL</returns>
BOOL
MvFile(const char *source, const char *target) {
    char    *index = (char*) NULL;
    char    cmd[256];
    char    targetDir[256];
    char    sourceDir[256];
    char    filePath[256];
    int     result = 0;

#if _OS_HPUX
    #define _CMD_MVFILE \
            "find \"%s\" -type f -name \"%s\" -print | xargs -i mv -f {} \"%s\""
#else
    #define _CMD_MVFILE \
            "find \"%s\" -maxdepth 1 -type f -name \"%s\" -print | xargs -i mv -f {} \"%s\""
#endif

    // 检查目的目录是否存在，若不存在则创建之
    if (!IsFileExist(target)) {
        index = (char*) strrchr(target, '/');
        if (index) {
            bzero(targetDir, sizeof(targetDir));
            strncpy(targetDir, target, index - target + 1);

            if (!MkDir(targetDir)) {
                return FALSE;
            }
        }
    }

    index = (char*) strrchr(source, '/');
    if (!index) {
        return FALSE;
    }

    bzero(sourceDir, sizeof(sourceDir));
    bzero(filePath, sizeof(filePath));

    strncpy(sourceDir, source, index - source + 1);
    strcpy(filePath, index + 1);

    sprintf(cmd, _CMD_MVFILE, sourceDir, filePath, target);

    result = system(cmd);

    if (result < 0) {
        return FALSE;
    }
    return TRUE;
}

/// <summary>
/// 移动文件夹
/// </summary>
/// <param name="source"></param>
/// <param name="target"></param>
/// <returns>BOOL</returns>
BOOL
MvDir(const char *source, const char *target) {
    char    *index = (char*) NULL;
    char    cmd[256];
    char    tmp[256];
    char    root[128];
    char    dir[128];
    int     result = 0;

#if _OS_HPUX
    #define _CMD_MVDIR  \
            "find \"%s\" -type d -name \"%s\" -print | xargs -i mv -f {} \"%s\""
#else
    #define _CMD_MVDIR  \
            "find \"%s\" -maxdepth 1 -type d -name \"%s\" -print | xargs -i mv -f {} \"%s\""
#endif

    // 检查目的目录是否存在，若不存在则创建之
    if (!IsDir(target)) {
        strcpy(tmp, target);
        RtrimRude(tmp);
        if (tmp[strlen(tmp) - 1] == '/') {
            tmp[strlen(tmp) - 1] = '\0';
        }

        index = strrchr(tmp, '/');
        if (!index) {
            return FALSE;
        }

        bzero(root, sizeof(root));
        strncpy(root, tmp, index - tmp + 1);

        if (!MkDir(root)) {
            return FALSE;
        }
    }

    strcpy(tmp, source);
    TrimRude(tmp);
    if (tmp[strlen(tmp) - 1] == '/') {
        tmp[strlen(tmp) - 1] = '\0';
    }

    index = strrchr(tmp, '/');
    if (!index) {
        return FALSE;
    }

    bzero(root, sizeof(root));
    bzero(dir, sizeof(dir));

    strncpy(root, tmp, index - tmp + 1);
    strcpy(dir, index + 1);

    sprintf(cmd, _CMD_MVDIR, root, dir, target);

    result = system(cmd);

    if (result < 0) {
        return FALSE;
    }
    return TRUE;
}

/// <summary>
/// 删除文件
/// </summary>
/// <param name="path"></param>
/// <returns>BOOL</returns>
BOOL
RmFile(const char *path) {
    char    *index = (char*) NULL;
    char    cmd[256];
    char    dir[128];
    char    filePath[256];
    int     result = 0;

#if _OS_HPUX
    #define _CMD_RMFILE \
            "find \"%s\" -type f -name \"%s\" -print | xargs -i rm -f {}"
#else
    #define _CMD_RMFILE \
            "find \"%s\" -maxdepth 1 -type f -name \"%s\" -print | xargs -i rm -f {}"
#endif

    index = (char*) strrchr(path, '/');
    if (!index) {
        return FALSE;
    }

    bzero(dir, sizeof(dir));
    bzero(filePath, sizeof(filePath));

    strncpy(dir, path, index - path + 1);
    strcpy(filePath, index + 1);

    sprintf(cmd, _CMD_RMFILE, dir, filePath);

    result = system(cmd);

    if (result != 0) {
        return FALSE;
    }
    return TRUE;
}

/// <summary>
/// 删除文件夹
/// </summary>
/// <param name="path"></param>
/// <returns>BOOL</returns>
BOOL
RmDir(const char *path) {
    char    *index = (char*) NULL;
    char    cmd[256];
    char    tmp[256];
    char    root[128];
    char    dir[128];
    int     result = 0;

#if _OS_HPUX
    #define _CMD_RMDIR  \
            "find \"%s\" -type d -name \"%s\" -print | xargs -i rm -fr {}"
#else
    #define _CMD_RMDIR  \
            "find \"%s\" -maxdepth 1 -type d -name \"%s\" -print | xargs -i rm -fr {}"
#endif

    strcpy(tmp, path);
    TrimRude(tmp);
    if (tmp[strlen(tmp) - 1] == '/') {
        tmp[strlen(tmp) - 1] = '\0';
    }

    index = strrchr(tmp, '/');
    if (!index) {
        return FALSE;
    }

    bzero(root, sizeof(root));
    bzero(dir, sizeof(dir));

    strncpy(root, tmp, index - tmp + 1);
    strcpy(dir, index + 1);

    sprintf(cmd, _CMD_RMDIR, root, dir);

    result = system(cmd);

    if (result != 0) {
        return FALSE;
    }
    return TRUE;
}

/// <summary>
/// 建立文件硬链接
/// </summary>
/// <param name="source"></param>
/// <param name="target"></param>
/// <returns>BOOL</returns>
BOOL
LnFile(const char *source, const char *target) {
    char    *index = (char*) NULL;
    char    cmd[256];
    char    targetDir[256];
    char    sourceDir[256];
    char    filePath[256];
    int     result = 0;

#if _OS_HPUX
    #define _CMD_LNFILE \
            "find \"%s\" -type f -name \"%s\" -print | xargs -i ln -f {} \"%s\""
#else
    #define _CMD_LNFILE \
            "find \"%s\" -maxdepth 1 -type f -name \"%s\" -print | xargs -i ln -f {} \"%s\""
#endif

    // 检查目的目录是否存在，若不存在则创建之
    if (!IsFileExist(target)) {
        index = (char*) strrchr(target, '/');
        if (index) {
            bzero(targetDir, sizeof(targetDir));
            strncpy(targetDir, target, index - target + 1);

            if (!MkDir(targetDir)) {
                return FALSE;
            }
        }
    }

    index = (char*) strrchr(source, '/');
    if (!index) {
        return FALSE;
    }

    bzero(sourceDir, sizeof(sourceDir));
    bzero(filePath, sizeof(filePath));

    strncpy(sourceDir, source, index - source + 1);
    strcpy(filePath, index + 1);

    sprintf(cmd, _CMD_LNFILE, sourceDir, filePath, target);

    result = system(cmd);

    if (result != 0) {
        return FALSE;
    }
    return TRUE;
}

/// <summary>
/// 打开文件
/// </summary>
/// <param name="pFd"></param>
/// <param name="filePath"></param>
/// <param name="oflag"></param>
/// <returns>BOOL</returns>
BOOL
OpenFile(int *pFd, const char *filePath, int oflag) {
    if (! pFd) {
        return FALSE;
    }

    if ((*pFd = open(filePath, oflag, F_MODE_RW)) < 0) {
        printf("open file fail! - file: [%s]; error: [%d:%s]",
                SWITCH_NULL(filePath, ""), errno, strerror(errno));
        return FALSE;
    }
    return TRUE;
}

/// <summary>
/// 打开文件
/// </summary>
/// <param name="关闭文件"></param>
/// <returns>void</returns>
void
CloseFile(int *pFd) {
    if (pFd && *pFd > 0) {
        close(*pFd);
        *pFd = -1;
    }
}

/// <summary>
/// 以覆盖的方式写入文件，并在写入后截断文件
/// </summary>
/// <param name="关闭文件"></param>
/// <returns>BOOL</returns>
BOOL
TruncWriteToFile(const char *filePath, const void *buf, long size) {
    return TruncWriteToOpenedFile((int*) NULL, filePath, buf, size);
}

/// <summary>
/// 以覆盖的方式写入文件，并在写入后截断文件
/// </summary>
/// <param name="pFd"></param>
/// <param name="filePath"></param>
/// <param name="buf"></param>
/// <param name="size"></param>
/// <returns>BOOL</returns>
BOOL
TruncWriteToOpenedFile(int *pFd, const char *filePath, const void *buf, long size) {
    int     fp = -1;

    if (! pFd) {
        pFd = &fp;
    }

    if (*pFd < 0) {
        if ((*pFd = open(filePath, O_WRONLY | O_CREAT, F_MODE_RW)) < 0) {
            printf("open file fail! - file: [%s]; error: [%d:%s]\n",
                    filePath, errno, strerror(errno));
            return FALSE;
        }
    } else {
        if (lseek(*pFd, 0, SEEK_SET) < 0) {
            printf("lseek file fail! - file: [%s]; error: [%d:%s]\n",
                    filePath, errno, strerror(errno));
            CloseFile(pFd);
            return FALSE;
        }
    }

    if (write(*pFd, buf, size) < 0) {
        printf("write to file fail! - file: [%s]; error: [%d:%s]\n",
                filePath, errno, strerror(errno));
        CloseFile(pFd);
        return FALSE;
    }

    if (ftruncate(*pFd, size) < 0) {
        printf("ftruncate file fail! - file: [%s]; error: [%d:%s]\n",
                filePath, errno, strerror(errno));
        CloseFile(pFd);
        return FALSE;
    }

    if (pFd == &fp) {
        CloseFile(pFd);
    }
    return TRUE;
}

/// <summary>
/// 以追加的方式写入文件
/// </summary>
/// <param name="filePath"></param>
/// <param name="buf"></param>
/// <param name="size"></param>
/// <returns>BOOL</returns>
BOOL
AppendToFile(const char *filePath, const void *buf, long size) {
    return AppendToOpenedFile((int*) NULL, filePath, buf, size);
}

/// <summary>
/// 以追加的方式写入文件
/// </summary>
/// <param name="pFd"></param>
/// <param name="filePath"></param>
/// <param name="buf"></param>
/// <param name="size"></param>
/// <returns>BOOL</returns>
BOOL
AppendToOpenedFile(int *pFd, const char *filePath, const void *buf, long size) {
    int     fp = -1;

    if (! pFd) {
        pFd = &fp;
    }

    if (*pFd < 0) {
        if ((*pFd = open(filePath, O_WRONLY | O_CREAT | O_APPEND, F_MODE_RW)) < 0) {
            printf("open file fail! - file: [%s]; error: [%d:%s]\n",
                    filePath, errno, strerror(errno));
            return FALSE;
        }
    }

    if (write(*pFd, buf, size) < 0) {
        printf("write to file fail! - file: [%s]; error: [%d:%s]\n",
                filePath, errno, strerror(errno));
        CloseFile(pFd);
        return FALSE;
    }

    if (pFd == &fp) {
        CloseFile(pFd);
    }
    return TRUE;
}
