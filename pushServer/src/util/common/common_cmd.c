#include    <stdio.h>
#include    "common_cmd.h"


/// <summary>
/// ��ʾ��ʾ��Ϣ
/// </summary>
/// <param name="msg"></param>
/// <returns>void</returns>
void
Usage(const char *const *msg) {
    fprintf(stderr, "\n");
    for (; *msg; msg++) {
        fprintf(stderr, *msg);
    }
    fprintf(stderr, "\n");
}
