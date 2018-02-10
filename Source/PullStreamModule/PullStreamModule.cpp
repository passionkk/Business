#include "stdafx.h"
#include "PullStreamModule.h"


bool /*PullStreamModule::*/SortByPts(AVPacket* pkt1, AVPacket* pkt2)
{
	return pkt1->pts < pkt2->pts;
}

//从extradata里解析ADTS结构
int AAC_Decode_Extradata(ADTSContext *pADTS, unsigned char *pExtradata, int nBufSize)
{
	int nAot, nAotext, nSamFreIndex, nChannelConfig;
	unsigned char *p = pExtradata;

	if (!pADTS || !pExtradata || nBufSize < 2)
	{
		return -1;
	}
	nAot = (p[0] >> 3) & 0x1f;
	if (nAot == 31)
	{
		nAotext = (p[0] << 3 | (p[1] >> 5)) & 0x3f;
		nAot = 32 + nAotext;
		nSamFreIndex = (p[1] >> 1) & 0x0f;

		if (nSamFreIndex == 0x0f)
		{
			nChannelConfig = ((p[4] << 3) | (p[5] >> 5)) & 0x0f;
		}
		else
		{
			nChannelConfig = ((p[1] << 3) | (p[2] >> 5)) & 0x0f;
		}
	}
	else
	{
		nSamFreIndex = ((p[0] << 1) | p[1] >> 7) & 0x0f;
		if (nSamFreIndex == 0x0f)
		{
			nChannelConfig = (p[4] >> 3) & 0x0f;
		}
		else
		{
			nChannelConfig = (p[1] >> 3) & 0x0f;
		}
	}

#ifdef AOT_PROFILE_CTRL  //网络上找的代码，此句不明所以
	if (nAot < 2) 
		nAot= 2;
#endif  
	pADTS->nObjectType = nAot - 1;
	pADTS->nSampleRateIndex = nSamFreIndex;
	pADTS->nChannelConf = nChannelConfig;
	pADTS->nWriteADTS = 1;

	return 0;
}

//根据ADTSContext 计算ADTS头信息，插入每一个AAC帧前面
int AAC_Set_ADTS_Head(ADTSContext *pADTSCtx, unsigned char *pBuf, int nDataSize)
{
	unsigned char byte;

	if (nDataSize < ADTS_HEADER_SIZE)
	{
		return -1;
	}

	pBuf[0] = 0xff;
	pBuf[1] = 0xf1;
	byte = 0;
	byte |= (pADTSCtx->nObjectType & 0x03) << 6;
	byte |= (pADTSCtx->nSampleRateIndex & 0x0f) << 2;
	byte |= (pADTSCtx->nChannelConf & 0x07) >> 2;
	pBuf[2] = byte;
	byte = 0;
	byte |= (pADTSCtx->nChannelConf & 0x07) << 6;
	byte |= (ADTS_HEADER_SIZE + nDataSize) >> 11;
	pBuf[3] = byte;
	byte = 0;
	byte |= (ADTS_HEADER_SIZE + nDataSize) >> 3;
	pBuf[4] = byte;
	byte = 0;
	byte |= ((ADTS_HEADER_SIZE + nDataSize) & 0x7) << 5;
	byte |= (0x7ff >> 6) & 0x1f;
	pBuf[5] = byte;
	byte = 0;
	byte |= (0x7ff & 0x3f) << 2;
	pBuf[6] = byte;

	return 0;
}
//

PullStreamModule::PullStreamModule()
	: m_strUrl("")
	, m_nSrcWidth(0)
	, m_nSrcHeigth(0)
	, m_eSrcPixFmt(0)
	, m_nDstWidth(0)
	, m_nDstHeight(0)
	, m_eDstPixFmt(0)
	, m_nSrcChannels(0)
	, m_nSrcSmapleRate(0)
	, m_eSrcSamFmt(0)
	, m_nDstChannels(0)
	, m_nDstSmapleRate(0)
	, m_eDstSamFmt(0)
	, m_pFmtCtx(NULL)
	, m_bOpenStream(false)
	, m_nVidStreamIndex(-1)
	, m_nAudStreamIndex(-1)
	, m_bGetAudio(true)
	, m_bGetVideo(true)
	, m_nReconnectInterval(5)
	, m_pVFilterContext(NULL)
	, m_pAFilterContext(NULL)
	, m_bAutoReconnect(true)
{
	m_dequeVideoPkt.clear();
	m_dequeAudioPkt.clear();
	m_DataHandles.clear();
	m_DataInstances.clear();
	memset(m_sADTSHeader, 0, ADTS_HEADER_SIZE);
}


PullStreamModule::~PullStreamModule()
{

}

static int g_Gop = 0;

void PullStreamModule::run()
{
	if (m_pFmtCtx != NULL)
	{
		AVPacket packet;
		AVPacket* pPkt = &packet;
		av_init_packet(pPkt);
		int nRecvRet = 0;

		AVRational videoTimebase = { 1, 25 };
		AVRational audioTimebase = { 1, 48000 };
		AVRational outTimebase = { 1, 90000 };
		int64_t iVStartTime = 0;
		int64_t iAStartTime = 0;
		int64_t iPktPts = 0;

		if (m_nVidStreamIndex >= 0)
		{
			videoTimebase = m_pFmtCtx->streams[m_nVidStreamIndex]->time_base;
			if (AV_NOPTS_VALUE != m_pFmtCtx->streams[m_nVidStreamIndex]->start_time)
				iVStartTime = m_pFmtCtx->streams[m_nVidStreamIndex]->start_time;
		}
		if (m_nAudStreamIndex >= 0)
		{
			audioTimebase = m_pFmtCtx->streams[m_nAudStreamIndex]->time_base;
			if (AV_NOPTS_VALUE != m_pFmtCtx->streams[m_nAudStreamIndex]->start_time)
				iAStartTime = m_pFmtCtx->streams[m_nAudStreamIndex]->start_time;
		}
		int nReadFrame = 0;
		int nGetKetPacket = 0;
		while (!m_bStop)
		{
			nReadFrame = av_read_frame(m_pFmtCtx, &packet);
			if (nReadFrame  < 0)
			{
				//end of file or error need reconnect
				CloseStream(true);
				if (m_bAutoReconnect)
				{
					Poco::Thread::sleep(m_nReconnectInterval * 1000);
					OpenStream(m_strUrl, m_bGetVideo, m_bGetAudio, m_bAutoReconnect, true);
				}
				else
					break;
			}
			else
			{
				if (pPkt->stream_index == m_nVidStreamIndex && m_bGetVideo)
				{
					iPktPts = pPkt->pts == AV_NOPTS_VALUE ? pPkt->dts : pPkt->pts;
					pPkt->pts = (iPktPts - iVStartTime) * av_q2d(videoTimebase) * 1000;//ms
					pPkt->dts = (pPkt->dts - iVStartTime) * av_q2d(videoTimebase) * 1000;//ms

					if (pPkt->pts >= 0 && pPkt->dts >= 0)
					{
						AVPacket* pSavePkt = av_packet_clone(pPkt);

						if (m_pVFilterContext != NULL)
							av_bitstream_filter_filter(m_pVFilterContext, m_pFmtCtx->streams[m_nVidStreamIndex]->codec, NULL,
							&pSavePkt->data, &pSavePkt->size, pPkt->data, pPkt->size, pPkt->flags & AV_PKT_FLAG_KEY);

						Poco::Mutex::ScopedLock lock(m_mutexVideoDeque);
						m_dequeVideoPkt.push_back(pSavePkt);
					}
				}
				else if (pPkt->stream_index == m_nAudStreamIndex && m_bGetAudio)
				{ 
					iPktPts = pPkt->pts == AV_NOPTS_VALUE ? pPkt->dts : pPkt->pts;
					pPkt->pts = (iPktPts - iAStartTime) * av_q2d(audioTimebase) * 1000;//ms
					pPkt->dts = (pPkt->dts - iAStartTime) * av_q2d(audioTimebase) * 1000;//ms

					if (pPkt->pts >= 0 && pPkt->dts >= 0)
					{
						AVPacket* pSavePkt = av_packet_alloc();
						av_new_packet(pSavePkt, pPkt->size + ADTS_HEADER_SIZE);
						if (m_pFmtCtx->streams[m_nAudStreamIndex]->codec->codec_id == AV_CODEC_ID_AAC)
							pSavePkt->size = pPkt->size + ADTS_HEADER_SIZE;
						else
							pSavePkt->size = pPkt->size;
						pSavePkt->pts = pPkt->pts;
						if (pSavePkt->dts < 0)
							pSavePkt->dts = pSavePkt->pts;

						if (m_pFmtCtx->streams[m_nAudStreamIndex]->codec->codec_id == AV_CODEC_ID_AAC)
						{
							AAC_Set_ADTS_Head(&m_stADTSCtx, m_sADTSHeader, pPkt->size);
							memcpy(pSavePkt->data, m_sADTSHeader, ADTS_HEADER_SIZE);
							memcpy(pSavePkt->data + ADTS_HEADER_SIZE, pPkt->data, pPkt->size);
						}
						else
						{
							memcpy(pSavePkt->data, pPkt->data, pPkt->size);
						}
						Poco::Mutex::ScopedLock lock(m_mutexAudioDeque);
						m_dequeAudioPkt.push_back(pSavePkt);
					}
				}
				av_packet_unref(pPkt);
			}
		}
		m_bStopRun = true;
	}
}

bool PullStreamModule::OpenStream(std::string strUrl, bool bGetVideo, bool bGetAudio, bool bAutoReconnect, bool bIsReconnect)
{
	bool bRet = true;
	if (!bIsReconnect)
	{
		if (m_bOpenStream == true)
			return false;
		m_bOpenStream = false;
		m_bAutoReconnect = bAutoReconnect;
	}
	if (m_pFmtCtx == NULL)
		m_pFmtCtx = avformat_alloc_context();
	AVFormatContext* pFormatCtx = m_pFmtCtx;
	if (pFormatCtx)
	{
		if (avformat_open_input(&pFormatCtx, strUrl.c_str(), NULL, NULL) < 0)
		{
			bRet = false;
		}
		if (bRet && avformat_find_stream_info(pFormatCtx, NULL) < 0)
		{
			bRet = false;
		}
		else if (bRet)
		{
			int nStreamArray[AVMEDIA_TYPE_NB];
			memset(nStreamArray, -1, sizeof(nStreamArray));
			for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
			{
				AVStream* pStream = pFormatCtx->streams[i];
				enum AVMediaType eStreamType = pStream->codecpar->codec_type;
				if (eStreamType >= 0 && nStreamArray[eStreamType] == -1)
					nStreamArray[eStreamType] = i;
			}
			if (nStreamArray[AVMEDIA_TYPE_VIDEO] >= 0)
			{
				int nStreamIndex = m_nVidStreamIndex = nStreamArray[AVMEDIA_TYPE_VIDEO];
				AVCodecContext* pCodecCtx = pFormatCtx->streams[nStreamIndex]->codec;
				if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[nStreamIndex]->codecpar) < 0)
				{
					avcodec_free_context(&pCodecCtx);
					bRet = false;
				}
				else
				{
					AVStream* pVideoStream = pFormatCtx->streams[nStreamIndex];
					m_stVAFileInfo.bHasVideo = true;
					m_stVAFileInfo.sVideoInfo.eCodecID = pCodecCtx->codec_id;
					m_stVAFileInfo.sVideoInfo.ePixelFormat = pCodecCtx->pix_fmt;
					m_stVAFileInfo.sVideoInfo.nWidth = pCodecCtx->width;
					m_stVAFileInfo.sVideoInfo.nHeight = pCodecCtx->height;
					m_stVAFileInfo.sVideoInfo.eFmtType = GetFormatTypeByAVCodecID(pCodecCtx->codec_id);

					av_codec_set_pkt_timebase(pCodecCtx, pFormatCtx->streams[nStreamIndex]->time_base);

					AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
					AVDictionary* pOpts = NULL;
					av_dict_set(&pOpts, "refcounted_frames", "1", 0);
					if (avcodec_open2(pCodecCtx, pCodec, &pOpts) < 0)
					{
						bRet = false;
					}

					int64_t nValue = 0;
					if (pCodecCtx && pCodecCtx->codec && pCodecCtx->codec->priv_class && bRet) {
						const AVOption *opt = NULL;
						while (opt = av_opt_next(pCodecCtx->priv_data, opt)) {
							if (opt->flags) 
								continue;
							if (strcmp(opt->name, "is_avc") == 0)
							{
								if (av_opt_get_int(pCodecCtx->priv_data, opt->name, 0, &nValue) < 0)
								{
									printf("get is_avc error.\n");
								}	
								break;
							}
						}
					}

					if (m_pVFilterContext == NULL && bRet && nValue == 1)
					{
						switch (m_pFmtCtx->streams[m_nVidStreamIndex]->codec->codec_id)
						{
						case AV_CODEC_ID_H264:
							m_pVFilterContext = av_bitstream_filter_init("h264_mp4toannexb");
							break;
						case AV_CODEC_ID_HEVC:
							m_pVFilterContext = av_bitstream_filter_init("hevc_mp4toannexb");
							break;
						default:
							break;
						}
					}
				}
			}
			else
			{
				m_stVAFileInfo.bHasVideo = false;
			}

			if (nStreamArray[AVMEDIA_TYPE_AUDIO] >= 0)
			{
				int nStreamIndex = m_nAudStreamIndex = nStreamArray[AVMEDIA_TYPE_AUDIO];
				AVCodecContext* pCodecCtx = pFormatCtx->streams[nStreamIndex]->codec;
				if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[nStreamIndex]->codecpar) < 0)
				{
					avcodec_free_context(&pCodecCtx);
					bRet = false;
				}
				char szFourcc[AV_FOURCC_MAX_STRING_SIZE] = { 0 };
				m_stVAFileInfo.bHasAudio = true;
				m_stVAFileInfo.sAudioInfo.eCodecID = pCodecCtx->codec_id;
				m_stVAFileInfo.sAudioInfo.eSampleFormat = pCodecCtx->sample_fmt;
				m_stVAFileInfo.sAudioInfo.nBitRate = pCodecCtx->bit_rate;
				m_stVAFileInfo.sAudioInfo.nChannels = pCodecCtx->channels;
				m_stVAFileInfo.sAudioInfo.nSampleRate = pCodecCtx->sample_rate;
				m_stVAFileInfo.sAudioInfo.nBitsPerSample = av_get_bytes_per_sample(pCodecCtx->sample_fmt);
				m_stVAFileInfo.sAudioInfo.eFmtType = GetFormatTypeByAVCodecID(pCodecCtx->codec_id);

				av_codec_set_pkt_timebase(pCodecCtx, pFormatCtx->streams[nStreamIndex]->time_base);

				AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
				AVDictionary* pOpts = NULL;
				av_dict_set(&pOpts, "refcounted_frames", "1", 0);
				if (avcodec_open2(pCodecCtx, pCodec, &pOpts) < 0)
				{
					bRet = false;
				}
				if (m_pFmtCtx->streams[m_nAudStreamIndex]->codec->codec_id == AV_CODEC_ID_AAC && bRet)
				{
					AAC_Decode_Extradata(&m_stADTSCtx, (unsigned char*)pCodecCtx->extradata, pCodecCtx->extradata_size);
				}
			}
			else
			{
				m_stVAFileInfo.bHasAudio = false;
			}
		}
		if (!m_stVAFileInfo.bHasAudio && !m_stVAFileInfo.bHasVideo)
		{
			bRet = false;
			avformat_close_input(&pFormatCtx);
		}

		if (bRet)
		{
			if(!bIsReconnect)
			{
				m_bGetVideo = bGetVideo;
				m_bGetAudio = bGetAudio;
				m_strUrl = strUrl;
				
				m_bOpenStream = true;
				m_bStop = false;
				m_thread.start(*this);

				m_VDataThread.start(PullStreamModule::HandleVData, (void*)this);
				m_ADataThread.start(PullStreamModule::HandleAData, (void*)this);
			}
		}
	}
	else
	{
		bRet = false;
	}
	return bRet;
}

int	PullStreamModule::Seek(int nMillSec)
{
	int nRet = 0;
	
	return nRet;
}

bool PullStreamModule::CloseStream(bool bIsReconnect)
{
	bool bRet = true;

	if (!bIsReconnect)
	{
		m_bStop = true;
		m_thread.join();
		m_VDataThread.join();
		m_ADataThread.join();

		m_bOpenStream = false;
	}

	if (m_pFmtCtx != NULL)
	{
		if (m_stVAFileInfo.bHasAudio && m_nAudStreamIndex >= 0)
		{
			avcodec_close(m_pFmtCtx->streams[m_nAudStreamIndex]->codec);
			avcodec_free_context(&m_pFmtCtx->streams[m_nAudStreamIndex]->codec);
		}
		if (m_stVAFileInfo.bHasAudio && m_nVidStreamIndex >= 0)
		{
			avcodec_close(m_pFmtCtx->streams[m_nVidStreamIndex]->codec);
			avcodec_free_context(&m_pFmtCtx->streams[m_nVidStreamIndex]->codec);
		}
		avformat_close_input(&m_pFmtCtx);
		m_pFmtCtx = NULL;
		if (m_pVFilterContext != NULL)
		{
			av_bitstream_filter_close(m_pVFilterContext);
			m_pVFilterContext = NULL;
		}
		if (m_pAFilterContext != NULL)
		{
			av_bitstream_filter_close(m_pAFilterContext);
			m_pAFilterContext = NULL;
		}
		
	}

	return bRet;
}

//可能暂时用不到
void PullStreamModule::SetVideoParam(int nDstWidth, int nDstHeight, PixelFmt eDstPixFmt)
{
	m_nDstWidth = nDstWidth;
	m_nDstHeight = nDstHeight;
	m_eDstPixFmt = eDstPixFmt;
}

void PullStreamModule::SetAudioParam(int nDstChannels, int nDstSampleRate, SampleFmt eDstSamFmt)
{
	m_nDstChannels = nDstChannels;
	m_nDstSmapleRate = nDstSampleRate;
	m_eDstSamFmt = eDstSamFmt;
}

void PullStreamModule::HandleVData(void* pParam)
{
	PullStreamModule* pThis = (PullStreamModule*)pParam;
	if (pParam != NULL)
	{
		pThis->OnHandleVData();
	}
}

void PullStreamModule::OnHandleVData()
{
	int64_t iLastPts = ULLONG_MAX;
	while (!m_bStop)
	{
		if (m_dequeVideoPkt.empty())
		{
			Poco::Thread::sleep(1);
		}
		else
		{
			Poco::Mutex::ScopedLock lock(m_mutexVideoDeque);
			
			AVPacket* pPkt = m_dequeVideoPkt.front();
#if WIN32
			//TRACE(L"[PullStreamModule::OnHandleVData]V pkt pts : %lld, size : %d.\n", pPkt->pts, pPkt->size);
#endif
			Poco::Mutex::ScopedLock lockCallback(m_CallbackMutex);
			for (int i = 0; i < m_DataHandles.size(); ++i)
			{
				try
				{
					if (m_DataHandles[i].first)
					{
						m_DataHandles[i].first(m_DataHandles[i].second, Video, 
											   m_stVAFileInfo.sVideoInfo.eFmtType,
											   pPkt->data, pPkt->size, pPkt->pts, pPkt->dts);
					}
				}
				catch (...)
				{
				}
			}
			for (int i = 0; i < m_DataInstances.size(); ++i)
			{
				try
				{
					if (m_DataInstances[i].first)
					{
						m_DataInstances[i].first->DataHandle(m_DataInstances[i].second, Video, 
															 m_stVAFileInfo.sVideoInfo.eFmtType,
															 pPkt->data, pPkt->size, pPkt->pts, pPkt->dts);
					}
				}
				catch (...)
				{
				}
			}
			m_dequeVideoPkt.pop_front();
			av_packet_unref(pPkt);
			av_packet_free(&pPkt);
		}
	}
}

void PullStreamModule::HandleAData(void* pParam)
{
	PullStreamModule* pThis = (PullStreamModule*)pParam;
	if (pParam != NULL)
	{
		pThis->OnHandleAData();
	}
}

void PullStreamModule::OnHandleAData()
{
	while (!m_bStop)
	{
		if (m_dequeAudioPkt.empty())
		{
			Poco::Thread::sleep(1);
		}
		else
		{
			Poco::Mutex::ScopedLock lock(m_mutexAudioDeque);
			AVPacket* pPkt = m_dequeAudioPkt.front();
#if WIN32
			//TRACE(L"[PullStreamModule::OnHandleAData]A pkt pts : %lld, size : %d.\n", pPkt->pts, pPkt->size);
#endif
			Poco::Mutex::ScopedLock lockCallback(m_CallbackMutex);
			for (int i = 0; i < m_DataHandles.size(); ++i)
			{
				try
				{
					if (m_DataHandles[i].first)
					{
						m_DataHandles[i].first(m_DataHandles[i].second, Audio, 
											   m_stVAFileInfo.sAudioInfo.eFmtType,
											   pPkt->data, pPkt->size, pPkt->pts, pPkt->dts);
					}
				}
				catch (...)
				{
				}
			}
			for (int i = 0; i < m_DataInstances.size(); ++i)
			{
				try
				{
					if (m_DataInstances[i].first)
					{
						m_DataInstances[i].first->DataHandle(m_DataInstances[i].second, Audio,
															 m_stVAFileInfo.sAudioInfo.eFmtType,
															 pPkt->data, pPkt->size, pPkt->pts, pPkt->dts);
					}
				}
				catch (...)
				{
				}
			}
			m_dequeAudioPkt.pop_front();
			av_packet_unref(pPkt);
			av_packet_free(&pPkt);
		}
	}
}

void PullStreamModule::Subscribe(DataHandle dataHandle, void *pUserData)
{
	Poco::Mutex::ScopedLock lock(m_CallbackMutex);
	bool bNotFind = true;
	for (int i = 0; i < m_DataHandles.size(); ++i)
	{
		if (m_DataHandles[i].first == dataHandle)
		{
			bNotFind = false;
			break;
		}
	}
	if (bNotFind)
	{
		m_DataHandles.push_back(std::make_pair(dataHandle, pUserData));
	}
}

void PullStreamModule::UnSubscribe(DataHandle dataHandle)
{
	Poco::Mutex::ScopedLock lock(m_CallbackMutex);
	for (std::vector<std::pair<DataHandle, void*> >::iterator it =
		 m_DataHandles.begin(); it != m_DataHandles.end(); ++it)
	{
		if ((*it).first == dataHandle)
		{
			m_DataHandles.erase(it);
			break;
		}
	}
}

void PullStreamModule::Subscribe(RtmpDataIface *pDataInstance, void *pUserData)
{
	Poco::Mutex::ScopedLock lock(m_CallbackMutex);
	bool bNotFind = true;
	for (int i = 0; i < m_DataInstances.size(); ++i)
	{
		if (m_DataInstances[i].first == pDataInstance)
		{
			bNotFind = false;
			break;
		}
	}
	if (bNotFind)
	{
		m_DataInstances.push_back(std::make_pair(pDataInstance, pUserData));
	}
}

void PullStreamModule::UnSubscribe(RtmpDataIface *pDataInstance)
{
	Poco::Mutex::ScopedLock lock(m_CallbackMutex);
	for (std::vector<std::pair<RtmpDataIface *, void*> >::iterator it =
		 m_DataInstances.begin(); it != m_DataInstances.end(); ++it)
	{
		if ((*it).first == pDataInstance)
		{
			m_DataInstances.erase(it);
			break;
		}
	}
}
