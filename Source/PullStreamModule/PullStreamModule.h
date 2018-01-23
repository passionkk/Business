#ifndef _PullStreamModule_H_
#define _PullStreamModule_H_

/************************************************************************/
/*
	�ļ�˵��	��	��ȡrtmp,hls,httpֱ�����㲥��
	����		��	����FFMPEG ������
*/
/************************************************************************/

#include "../../Include/Common/CommonDef.h"
#include <string>
#include <poco/Thread.h>

extern "C"
{
#include <libavformat/avformat.h> 
#include <libavcodec/avcodec.h>
}

#ifdef WIN32
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#else
#endif

class PullStreamModule : 
	public Poco::Runnable
{
public:
	PullStreamModule();
	~PullStreamModule();

public:
	//��ʼ���ͷ���ʼ�� FFmpeg��
	static void	InitFFmpegModule();
	static void UninitFFmpegModule();

	bool	OpenStream(std::string strUrl, bool bGetVideo = true, bool bGetAudio = true);
	void	SetVideoParam(int nDstWidth, int nDstHeight, PixelFmt eDstPixFmt);
	void	SetAudioParam(int nDstChannels, int nDstSampleRate, SampleFmt eDstSamFmt);
	bool	CloseStream();

	void	run();

protected:
	std::string		m_strUrl;
	//Dst Video Info
	int				m_nDstWidth;
	int				m_nDstHeight;
	int				m_eDstPixFmt;
	//Dst Audio Info
	int				m_nDstChannels;
	int				m_nDstSmapleRate;
	int				m_eDstSamFmt;
	
	bool			m_bStop;
	Poco::Thread	m_thread;
};

#endif