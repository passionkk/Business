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
	none	= -2, // ��ǰ��������
	unkonwn = -1, // ��ָ��AVInputFormat��FFmpeg�ڲ�̽���������ʽ
	g711a	= 19,
	g711u	= 20,
	opus	= 21,	//���ܲ���
	mp3		= 41,	//�����ӵ�mp3
	aaclc	= 42,
	png		= 90,	//mp3�з������png
	h264	= 96,
	h265	= 265,
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
	#include "libavutil/opt.h"
}

#ifdef WIN32
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#else
#endif

struct ADTSContext
{
	void InitContext()
	{
		nWriteADTS = 0;
		nObjectType = 0;
		nSampleRateIndex = 0;
		nChannelConf = 0;
	}

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

struct stVideoStreamInfo			//��Ƶ����
{
	int						nWidth;			//ͼ����
	int						nHeight;		//ͼ��߶�
	int						nGopSize;		//����ʱ֡�ṹ
	__int64					nBitRate;		//��Ƶ����
	float					fFrameRate;		//��Ƶ֡��
	AVCodecID				eCodecID;		//��Ƶ���뷽ʽ
	FormatType				eFmtType;		//����eCodecID
	AVPixelFormat			ePixelFormat;	//��Ƶ��ɫ�ռ��ʽ
	int						nDen;			//
	int						nNum;			//
	int						nFrameCount;	//��֡��

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

struct stAudioStreamInfo			//��Ƶ����
{
	int						nChannels;		//������Ŀ
	int						nSampleRate;	//��Ƶ������
	int						nBitsPerSample;
	__int64					nBitRate;		//��Ƶ����
	AVCodecID				eCodecID;		//��Ƶ���뷽ʽ
	FormatType				eFmtType;		//����eCodecID
	AVSampleFormat			eSampleFormat;	//��Ƶ������ʽ

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
	__int64					nDuration;		//�ļ�ʱ��,��λ΢��

	bool					bHasVideo;		//�Ƿ�����Ƶ
	stVideoStreamInfo		sVideoInfo;		//�ļ�������Ƶ����

	bool					bHasAudio;		//�Ƿ�����Ƶ
	stAudioStreamInfo		sAudioInfo;		//�ļ�������Ƶ����

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

//��ʼ���ͷ���ʼ�� FFmpeg��
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