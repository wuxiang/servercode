#include "version.h"
#include "../util/util.h"
#include "../util/common/common_types.h"
#include "../util/common/common_cmd.h"
#include "../util/common/common_env.h"
#include "../util/clib/getopt.h"
#include "../util/log/log.h"

/*
 * 提示信息定义
 */
static const char *const _version_usage[] = {
    "PushServerAs5_64_V2.4.02 2013/04/03 \n",
    (char *) NULL,
};
/* -------------------------           */

int
Version(int argc, char* argv[]) {
    static const char       short_options[] = "v:t";
    static struct option    long_options[] = {
        { "verbose",    0,  NULL,   'v' },
        { "test",    0,  NULL, 't' },
        { 0, 0, 0, 0 }
    };

    int     option_index = 0;
    int     c = 0;
 	bool    verbose     = false; 	
   
    opterr = 0;
    while ((c = getopt_long(argc, argv, short_options,
            long_options, &option_index)) != EOF) {
        switch (c) {
            case 'v':
                verbose = true;
                break;
            
            case 't':
            	break;

            case '?':
            default:
                Usage(_version_usage);
                return -1;
        }
    }
    
    // 显示信息
    Usage(_version_usage);
        
    return 0;
}

