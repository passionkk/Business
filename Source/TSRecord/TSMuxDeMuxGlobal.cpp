#include "TSMuxDeMuxGlobal.h"

#ifdef WIN32
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#else
#endif

bool g_bGlobalInitSuccess = InitEnv();

bool InitEnv()
{
    static bool bInit = false;
    if (!bInit)
    {
        bInit = true;
        av_register_all();
    }
    return bInit;
}
