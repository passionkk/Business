#ifndef _PullStreamModule_H_
#define _PullStreamModule_H_

/************************************************************************/
/*
	�ļ�˵��	��	��ȡrtmp,hls,httpֱ�����㲥��
	����		��	����FFMPEG ������
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

	// ��ӹ�ע������ݵĺ���ָ��
	void Subscribe(DataHandle dataHandle, void *pUserData);

	// ȡ����ע������ݵĺ���ָ��
	void UnSubscribe(DataHandle dataHandle);

	// ��ӹ�ע������ݵ�ʵ������
	void Subscribe(RtmpDataIface *pDataInstance, void *pUserData);

	// ȡ����ע������ݵ�ʵ������
	void UnSubscribe(RtmpDataIface *pDataInstance);

	// ����
	bool	OpenStream(std::string strUrl, bool bGetVideo = true, bool bGetAudio = true, bool bIsReconnect = false);

	// �ر���
	bool	CloseStream(bool bIsReconnect = false);

	// ������ʱ�ò���
	// Seek 
	int		Seek(int nMillSec);
	// ������Ƶ�������
	void	SetVideoParam(int nDstWidth, int nDstHeight, PixelFmt eDstPixFmt);
	// ������Ƶ�������
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
	
	int				m_nReconnectInterval;//�������ʱ��

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