#ifndef _CommonDefine_H_
#define _CommonDefine_H_

enum EventType
{
	Data = 1,
};

enum Record_State
{
	RS_Stop = 0,
	RS_Record = 1,
	RS_Pause = 2,
	RS_Count,
};

enum MediaType
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

enum PixelFmt
{
	PF_NONE = 0,
	PF_RGBA = 1,
	PF_BGRA = 2,
	PF_YUV420 = 3,
	PF_YUV420P = 4,
	PF_YUV422 = 5,
	PF_YUV422P = 6,
};

enum SampleFmt
{
	SF_NONE	= 0,
};

//初始化和反初始化 FFmpeg库
void	InitFFmpegModule();
void	UninitFFmpegModule();

#endif