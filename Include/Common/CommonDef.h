#ifndef _CommonDef_H_
#define _CommonDef_H_

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
	opus  = 21,	//可能不对
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

extern "C"
{
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/avutil.h"
}

#ifdef WIN32
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#else
#endif

struct ADTSContext
{
	int nWriteADTS;
	int nObjectType;
	int nSampleRateIndex;
	int nChannelConf;
};

struct AV_FRAME_BUF_S
{
	MediaType eType;
	FormatType nFormatType;
	char* pData;
	int nDataLen;
	int nTimeStamp;
};

struct stVideoStreamInfo			//视频属性
{
	int						nWidth;			//图像宽度
	int						nHeight;		//图像高度
	int						nGopSize;		//编码时帧结构
	__int64					nBitRate;		//视频码率
	float					fFrameRate;		//视频帧率
	AVCodecID				eCodecID;		//视频编码方式
	FormatType				eFmtType;		//兼容eCodecID
	AVPixelFormat			ePixelFormat;	//视频颜色空间格式
	int						nDen;			//
	int						nNum;			//
	int						nFrameCount;	//总帧数

	stVideoStreamInfo()
	{
		nWidth = -1;
		nHeight = -1;
		nGopSize = 12;
		nBitRate = -1;
		fFrameRate = 0.0f;
		eCodecID = AV_CODEC_ID_NONE;
		ePixelFormat = AV_PIX_FMT_NONE;
		nDen = 1;
		nNum = 0;
		nFrameCount = 0;
	}

	stVideoStreamInfo& operator = (const stVideoStreamInfo& other)
	{
		if (this == &other)
			return *this;

		nWidth = other.nWidth;
		nHeight = other.nHeight;
		nGopSize = other.nGopSize;
		nBitRate = other.nBitRate;
		fFrameRate = other.fFrameRate;
		eCodecID = other.eCodecID;
		ePixelFormat = other.ePixelFormat;
		nDen = other.nDen;
		nNum = other.nNum;
		nFrameCount = other.nFrameCount;
		return *this;
	}
};

struct stAudioStreamInfo			//音频属性
{
	int						nChannels;		//声道数目
	int						nSampleRate;	//音频采样率
	int						nBitsPerSample;
	__int64					nBitRate;		//音频码率
	AVCodecID				eCodecID;		//视频编码方式
	FormatType				eFmtType;		//兼容eCodecID
	AVSampleFormat			eSampleFormat;	//音频采样格式

	stAudioStreamInfo()
	{
		nChannels = 2;
		nSampleRate = 48000;
		nBitsPerSample = 0;
		nBitRate = -1;
		eCodecID = AV_CODEC_ID_NONE;
		eSampleFormat = AV_SAMPLE_FMT_NONE;
	}

	stAudioStreamInfo& operator = (const stAudioStreamInfo& other)
	{
		if (this == &other)
			return *this;

		nChannels = other.nChannels;
		nSampleRate = other.nSampleRate;
		nBitRate = other.nBitRate;
		eCodecID = other.eCodecID;
		eSampleFormat = other.eSampleFormat;
		nBitsPerSample = other.nBitsPerSample;
		return *this;
	}
};

struct stVAInfo
{
	__int64					nDuration;		//文件时长,单位微秒

	bool					bHasVideo;		//是否有视频
	stVideoStreamInfo		sVideoInfo;		//文件流中视频属性

	bool					bHasAudio;		//是否有音频
	stAudioStreamInfo		sAudioInfo;		//文件流中音频属性

	stVAInfo()
	{
		nDuration = -1;
		bHasVideo = false;
		bHasAudio = false;
	}

	stVAInfo& operator = (const stVAInfo& other)
	{
		if (this == &other)
			return *this;

		nDuration = other.nDuration;
		bHasVideo = other.bHasVideo;
		sVideoInfo = other.sVideoInfo;
		bHasAudio = other.bHasAudio;
		sAudioInfo = other.sAudioInfo;

		return *this;
	}
};

FormatType GetFormatTypeByAVCodecID(AVCodecID codecID);

//初始化和反初始化 FFmpeg库
class FFmpegInit
{
public:
	FFmpegInit();
	~FFmpegInit();

	static FFmpegInit *Initialize();
	static void Uninitialize();
	static FFmpegInit *GetInstance();

private:
	void Init();
	void UnInit();
public:
	static FFmpegInit* m_pInstance;
};

#endif