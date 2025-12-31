#if defined(_WIN32)
#define T_SHARED_POOL_NAME "SH_TestShared"
#define T_SHARED_POOL2_NAME "SH_TestShared1"
#define T_SHARED_POOL_SIZE 0x50000
#elif defined(__OS2__)
#define T_SHARED_POOL_NAME "\\SHAREMEM\\SHTEST"
#define T_SHARED_POOL2_NAME NULL
#define T_SHARED_POOL_SIZE 0
#elif defined(UNIX)
#include <sys/types.h>
#include <sys/ipc.h>
#define T_SHARED_POOL_NAME (char *)ftok("tclient.c", 'a')
#define T_SHARED_POOL2_NAME (char *)ftok("tclient.c", 'b')
#define T_SHARED_POOL_SIZE 4096
#endif
