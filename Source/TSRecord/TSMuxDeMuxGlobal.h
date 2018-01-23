#ifndef _TSMuxDeMuxGlobal_H_
#define _TSMuxDeMuxGlobal_H_

extern "C"
{
#include <libavformat/avformat.h> 
}

enum EventType
{
    Data = 1,
};

enum TSMediaType
{
    Video = 0,
    Audio = 1,
};

enum FormatType
{
    none = -2, // 当前流不存在
    unkonwn = -1, // 不指定AVInputFormat，FFmpeg内部探测流编码格式
    g711a = 19,
    g711u = 20,
    aaclc = 42,
    h264 = 96,
    h265 = 265,
};

bool InitEnv();

#endif // _TSMuxDeMuxGlobal_H_