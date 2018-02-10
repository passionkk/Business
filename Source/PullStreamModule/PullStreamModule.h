#ifndef _PullStreamModule_H_
#define _PullStreamModule_H_

/************************************************************************/
/*
	文件说明	：	拉取rtmp,hls,http直播，点播流
	功能		：	基于FFMPEG 拉流。
*/
/************************************************************************/

#include "../../Include/Common/CommonDef.h"
#include <poco/Thread.h>
#include <string>
#include <vector>
#include <deque>

#define ADTS_HEADER_SIZE 7

class RtmpDataIface
{
public:
	virtual ~RtmpDataIface() {}

	virtual void DataHandle(
		void *pUserData,
		MediaType eMediaType,
		FormatType eFormatType,
		uint8_t *pData,
		uint32_t iLen,
		uint64_t iPts /*ms*/,
		uint64_t iDts) = 0;

};

class PullStreamModule : 
	public Poco::Runnable
{
public:

	typedef void(*DataHandle)(
		void *pUserData,
		MediaType eMediaType,
		FormatType eFormatType,
		uint8_t *pData,
		uint32_t iLen,
		uint64_t iPts /*ms*/,
		uint64_t iDts);

	PullStreamModule();
	~PullStreamModule();

public:

	// 添加关注输出数据的函数指针
	void Subscribe(DataHandle dataHandle, void *pUserData);

	// 取消关注输出数据的函数指针
	void UnSubscribe(DataHandle dataHandle);

	// 添加关注输出数据的实例对象
	void Subscribe(RtmpDataIface *pDataInstance, void *pUserData);

	// 取消关注输出数据的实例对象
	void UnSubscribe(RtmpDataIface *pDataInstance);

	// 打开流
	bool	OpenStream(std::string strUrl, bool bGetVideo = true, bool bGetAudio = true, bool bAutoReconnect = true, bool bIsReconnect = false);

	// 关闭流
	bool	CloseStream(bool bIsReconnect = false);

	// 可能暂时用不到
	// Seek 
	int		Seek(int nMillSec);
	// 设置视频解码参数
	void	SetVideoParam(int nDstWidth, int nDstHeight, PixelFmt eDstPixFmt);
	// 设置音频解码参数
	void	SetAudioParam(int nDstChannels, int nDstSampleRate, SampleFmt eDstSamFmt);

	void	run();

	int	PacketQueuePut(AVPacket* pPacket);
	int	PacketQueueGet(AVPacket* pPacket, int *pSerial);
protected:
	static void	HandleVData(void* pParam);
	static void	HandleAData(void* pParam);
	void	OnHandleVData();
	void	OnHandleAData();
	//bool	SortByPts(AVPacket* pkt1, AVPacket* pkt2);
protected:
	std::string		m_strUrl;
	
	//Src Video Info
	int				m_nSrcWidth;
	int				m_nSrcHeigth;
	int				m_eSrcPixFmt;
	
	//Dst Video Info
	int				m_nDstWidth;
	int				m_nDstHeight;
	int				m_eDstPixFmt;

	//Src Audio Info
	int				m_nSrcChannels;
	int				m_nSrcSmapleRate;
	int				m_eSrcSamFmt;

	//Dst Audio Info
	int				m_nDstChannels;
	int				m_nDstSmapleRate;
	int				m_eDstSamFmt;
	
	bool			m_bGetAudio;
	bool			m_bGetVideo;
	
	bool			m_bAutoReconnect;
	int				m_nReconnectInterval;//重连间隔时长

	stVAInfo		m_stVAFileInfo;

	bool			m_bStop;
	bool			m_bStopRun;
	Poco::Thread	m_thread;
	Poco::Thread	m_VDataThread;
	Poco::Thread	m_ADataThread;

	bool			m_bOpenStream;
	int				m_nVidStreamIndex;
	int				m_nAudStreamIndex;
	AVFormatContext* m_pFmtCtx;

	std::deque<AVPacket*>	m_dequeVideoPkt;
	std::deque<AVPacket*>	m_dequeAudioPkt;
	Poco::Mutex			m_mutexVideoDeque;
	Poco::Mutex			m_mutexAudioDeque;

	Poco::Mutex			m_CallbackMutex;
	std::vector<std::pair<DataHandle, void*> > m_DataHandles;
	std::vector<std::pair<RtmpDataIface*, void*> > m_DataInstances;

	AVBitStreamFilterContext* m_pVFilterContext;
	AVBitStreamFilterContext* m_pAFilterContext;
	ADTSContext		m_stADTSCtx;
	unsigned char m_sADTSHeader[ADTS_HEADER_SIZE];
};

#endif