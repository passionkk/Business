#if WIN32
#include "stdafx.h"
#endif
#include <Poco/Clock.h>
#include <assert.h>

#include "Mp3Muxer.h"

static int ReadAudioDataProxy(void *pTSMuxer,
    uint8_t *pBuf, int iBufSize)
{
    Mp3Muxer *pMuxer = (Mp3Muxer *)pTSMuxer;
    return pMuxer->ReadAVData(pBuf, iBufSize, Audio);
}

static int ReadVideoDataProxy(void *pTSMuxer,
    uint8_t *pBuf, int iBufSize)
{
    Mp3Muxer *pMuxer = (Mp3Muxer *)pTSMuxer;
    return pMuxer->ReadAVData(pBuf, iBufSize, Video);
}

static int WriteTSDataProxy(void *pTSMuxer,
    uint8_t *pBuf, int iBufSize)
{
    Mp3Muxer *pMuxer = (Mp3Muxer *)pTSMuxer;
    return pMuxer->OnDataHandle(pBuf, iBufSize);
}

Mp3Muxer::Mp3Muxer()
: Poco::Runnable()
, m_pInFormatCtxA(NULL)
, m_pInFormatCtxV(NULL)
, m_pOutFormatCtx(NULL)
, m_pAVIOCtxA(NULL)
, m_pAVIOCtxV(NULL)
, m_pAVIOCtxOut(NULL)
, m_eAFormatType(unkonwn)
, m_eVFormatType(unkonwn)
, m_bRoutineStop(true)
, m_bFCtxAOpened(false)
, m_bFCtxVOpened(false)
, m_bTerminateConsume(true)
, m_iBufferSize(188 * 200)
, m_iCurTimeduration(0)
{

}

Mp3Muxer::~Mp3Muxer()
{
    Stop();
    Destroy();
}

void Mp3Muxer::Subscribe(DataHandle dataHandle, void *pUserData)
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

void Mp3Muxer::UnSubscribe(DataHandle dataHandle)
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

void Mp3Muxer::Subscribe(Mp3DataIface *pDataInstance, void *pUserData)
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

void Mp3Muxer::UnSubscribe(Mp3DataIface *pDataInstance)
{
    Poco::Mutex::ScopedLock lock(m_CallbackMutex);
	for (std::vector<std::pair<Mp3DataIface *, void*> >::iterator it =
        m_DataInstances.begin(); it != m_DataInstances.end(); ++it)
    {
        if ((*it).first == pDataInstance)
        {
            m_DataInstances.erase(it);
            break;
        }
    }
}

bool Mp3Muxer::ImportAVPacket(
    MediaType eMediaType,
    FormatType eFormatType,
    uint8_t *pData,
    int iDataSize,
    int64_t iTimestamp,
	int64_t iTimestampDTS)
{
    if (m_bRoutineStop || (NULL == pData) || iDataSize <= 0 ||
        (Audio == eMediaType && none == m_eAFormatType) ||
        (Video == eMediaType && none == m_eVFormatType))
    {
        return false;
    }
    AVPacketData * pAVPacketData = new AVPacketData;
    pAVPacketData->pData = new uint8_t[iDataSize];
    memcpy(pAVPacketData->pData, pData, iDataSize);
    pAVPacketData->iDataSize = iDataSize;
    pAVPacketData->iOffset = 0;
    pAVPacketData->iTimestamp = iTimestamp;
	pAVPacketData->iTimestampDTS = iTimestampDTS;
    pAVPacketData->eFormatType = eFormatType;
    pAVPacketData->iProbeOffset = 0;

    Poco::Mutex::ScopedLock lock(m_DataMutex);
    bool bEmpty = m_AudioPacketDatas.empty() ||
        m_VideoPacketDatas.empty();
    if (Audio == eMediaType)
    {
        m_AudioPacketDatas.push_back(pAVPacketData);
    }
    else if (Video == eMediaType)
    {
        m_VideoPacketDatas.push_back(pAVPacketData);
    }
    if (bEmpty)
    {
        m_ConsumeEvent.set();
    }
    return true;
}

bool Mp3Muxer::Start(
    FormatType eAFormatType,
    FormatType eVFormatType,
    int iAudioReadSize /*= 32768*/,
    int iVideoReadSize /*= 32768*/)
{
    if (!CheckAudioType(eAFormatType) ||
        !CheckVideoType(eVFormatType) ||
        (none == eAFormatType &&
         none == eVFormatType))
    {
        return false;
    }
    if (!m_RoutineThread.isRunning())
    {
#if WIN32
        int iTrueReadASize = min(iAudioReadSize, (int)m_iBufferSize);
        int iTrueReadVSize = min(iVideoReadSize, (int)m_iBufferSize);
#else
        int iTrueReadASize = std::min(iAudioReadSize, (int)m_iBufferSize);
        int iTrueReadVSize = std::min(iVideoReadSize, (int)m_iBufferSize);
#endif
        m_eAFormatType = eAFormatType;
        m_eVFormatType = eVFormatType;

        if (none != m_eAFormatType)
        {
            m_pInFormatCtxA = avformat_alloc_context();

            uint8_t * pBuffer = (uint8_t *)av_malloc(iTrueReadASize);
            m_pAVIOCtxA = avio_alloc_context(pBuffer,
                iTrueReadASize, 0, this, ReadAudioDataProxy, NULL, NULL);
            m_pInFormatCtxA->pb = m_pAVIOCtxA;
        }

        if (none != m_eVFormatType)
        {
            m_pInFormatCtxV = avformat_alloc_context();

            uint8_t * pBuffer = (uint8_t *)av_malloc(iTrueReadVSize);
            m_pAVIOCtxV = avio_alloc_context(pBuffer,
                iTrueReadVSize, 0, this, ReadVideoDataProxy, NULL, NULL);
            m_pInFormatCtxV->pb = m_pAVIOCtxV;
        }

        avformat_alloc_output_context2(&m_pOutFormatCtx, NULL, "mp3", NULL);
        uint8_t * pBuffer = (uint8_t *)av_malloc(m_iBufferSize);

        m_pAVIOCtxOut = avio_alloc_context(pBuffer, m_iBufferSize,
            AVIO_FLAG_WRITE, this, NULL, WriteTSDataProxy, NULL);

        m_pOutFormatCtx->pb = m_pAVIOCtxOut;
        m_pOutFormatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
        m_pOutFormatCtx->flags |= AVFMT_FLAG_FLUSH_PACKETS;
        m_pOutFormatCtx->flags |= AVFMT_ALLOW_FLUSH;

        m_bRoutineStop = false;
        m_bTerminateConsume = false;
        m_RoutineThread.start(*this);
        return true;
    }
    return false;
}

void Mp3Muxer::Stop()
{
    if (!m_bRoutineStop)
    {
        m_bRoutineStop = true;
        m_bTerminateConsume = true;
        m_ConsumeEvent.set();
        m_RoutineThread.join();
    }    
}

bool Mp3Muxer::IsBusy()
{
    bool bEmpty = false;
    {
        Poco::Mutex::ScopedLock lock(m_DataMutex);
        bEmpty = m_AudioPacketDatas.empty() || m_VideoPacketDatas.empty();
    }
    return !bEmpty && m_RoutineThread.isRunning();
}

bool Mp3Muxer::IsRunning() const
{
    return m_RoutineThread.isRunning();
}

AVFormatContext* Mp3Muxer::GetOutputFmtCtx()
{
	if (m_pOutFormatCtx)
		return m_pOutFormatCtx;
	else
		return nullptr;
}

int Mp3Muxer::ReadAVData(uint8_t *pBuf, int iBufSize, MediaType eType)
{
    int iReadSize = 0;
    do
    {
        if (m_bTerminateConsume)
        {
            iReadSize = -1; // 终止消费数据
            break;
        }
        AVPacketData *pAVPacketData = NULL;
        {
            Poco::Mutex::ScopedLock lock(m_DataMutex);

            std::deque<AVPacketData*> *pAVPacketDataDeque = (Audio == eType ?
                &m_AudioPacketDatas : &m_VideoPacketDatas);

            if (!pAVPacketDataDeque->empty())
            {
                for (int i = 0; i < pAVPacketDataDeque->size(); ++i)
                {
                    if ((*pAVPacketDataDeque)[i]->iProbeOffset <
                        (*pAVPacketDataDeque)[i]->iDataSize)
                    {
                        pAVPacketData = (*pAVPacketDataDeque)[i];
#if WIN32
                        iReadSize = min(pAVPacketData->iDataSize -
                            pAVPacketData->iProbeOffset, iBufSize);
#else
                        iReadSize = std::min(pAVPacketData->iDataSize -
                            pAVPacketData->iProbeOffset, iBufSize);
#endif
                        pAVPacketData->iProbeOffset += iReadSize;
                        break;
                    }
                }
            }
        }
        if (pAVPacketData)
        {
            memcpy(pBuf, pAVPacketData->pData +
                pAVPacketData->iProbeOffset - iReadSize, iReadSize);
            break;
        }
        else if (!m_bTerminateConsume)
        {
            // 等待数据
            try
            {
                m_ConsumeEvent.tryWait(10);
            }
            catch (...)
            {
            }
        }
    } while (true);

    return iReadSize;
}

int Mp3Muxer::OnDataHandle(uint8_t *pBuf, int iBufSize)
{
    Poco::Mutex::ScopedLock lock(m_CallbackMutex);
    for (int i = 0; i < m_DataHandles.size(); ++i)
    {
        try
        {
            if (m_DataHandles[i].first)
            {
                m_DataHandles[i].first(m_DataHandles[i].second,
                    pBuf, iBufSize, m_iCurTimeduration);
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
                m_DataInstances[i].first->DataHandle(
                    m_DataInstances[i].second, pBuf, iBufSize,
                    m_iCurTimeduration);
            }
        }
        catch (...)
        {
        }
    }
	return iBufSize;
}

void Mp3Muxer::run()
{
    std::string sError;
    int iAudioInIndex, iVideoInIndex;
    int iAudioOutIndex, iVideoOutIndex;
    bool bHasAudio = false;
    bool bHasVideo = false;

    do 
    {
        if (m_eAFormatType != none)
        {
            if (avformat_open_input(&m_pInFormatCtxA, "nothing",
                GetAVInputFormat(m_eAFormatType), NULL) < 0)
            {
                sError = "Could not open Audio source";
                break;
            }
            bHasAudio = true;
            m_bFCtxAOpened = true;
            if (avformat_find_stream_info(m_pInFormatCtxA, 0) < 0)
            {
                sError = "Failed to retrieve input audio stream information";
                break;
            }
            if ((iAudioInIndex = av_find_best_stream(m_pInFormatCtxA,
                AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0)) < 0)
            {
                sError = "Couldn't find a audio stream";
                break;
            }
            AVRational rational;
            rational.num = 1;
            rational.den = 90000;
            m_pInFormatCtxA->streams[iAudioInIndex]->time_base = rational;

            AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
            if (NULL == pCodecCtx)
            {
                sError = "Could not allocate AVCodecContext";
                break;
            }
            if (avcodec_parameters_to_context(pCodecCtx,
                m_pInFormatCtxA->streams[iAudioInIndex]->codecpar) < 0)
            {
                sError = "Copy codecpar to context failed in audio";
                break;
            }

            AVStream *outAStream = avformat_new_stream(m_pOutFormatCtx,
                pCodecCtx->codec);
            if (!outAStream)
            {
                sError = "Failed allocating output audio stream";
                break;
            }
            iAudioOutIndex = outAStream->index;

            if (avcodec_parameters_from_context(outAStream->codecpar,
                pCodecCtx) < 0)
            {
                sError = "Failed to copy codepar from"
                    " audio stream codec context";
                break;
            }
            avcodec_free_context(&pCodecCtx);
        }        

        if (m_eVFormatType != none)
        {
            if (avformat_open_input(&m_pInFormatCtxV, "nothing",
                GetAVInputFormat(m_eVFormatType), NULL) < 0)
            {
                sError = "Could not open Video source";
                break;
            }
            bHasVideo = true;

            m_bFCtxVOpened = true;
            if (avformat_find_stream_info(m_pInFormatCtxV, 0) < 0)
            {
                sError = "Failed to retrieve input video stream information";
                break;
            }
            if ((iVideoInIndex = av_find_best_stream(m_pInFormatCtxV,
                AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0)
            {
                sError = "Couldn't find a video stream";
                break;
            }
            AVRational rational;
            rational.num = 1;
            rational.den = 90000;
            m_pInFormatCtxV->streams[iVideoInIndex]->time_base = rational;

            AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
            if (NULL == pCodecCtx)
            {
                sError = "Could not allocate AVCodecContext";
                break;
            }
            if (avcodec_parameters_to_context(pCodecCtx,
                m_pInFormatCtxV->streams[iVideoInIndex]->codecpar) < 0)
            {
                sError = "Copy codecpar to context failed in video";
                break;
            }
            AVStream *outVStream = avformat_new_stream(m_pOutFormatCtx,
                pCodecCtx->codec);
            if (!outVStream)
            {
                sError = "Failed allocating output video stream";
                break;
            }
            iVideoOutIndex = outVStream->index;

            if (avcodec_parameters_from_context(outVStream->codecpar,
                pCodecCtx) < 0)
            {
                sError = "Failed to copy codecpar from"
                    " video stream codec context";
                break;
            }
            avcodec_free_context(&pCodecCtx);
        }

        if (!bHasAudio && !bHasVideo)
        {
            sError = "No audio option and video option, "
                "check call Start() parameter";
            break;
        }

        if (avformat_write_header(m_pOutFormatCtx, NULL) < 0)
        {
            sError = "Error occurred when opening output file";
            break;
        }

        int64_t iAFirstTimestamp = LLONG_MAX;
        int64_t iVFirstTimestamp = LLONG_MAX;
        int64_t iBaseTimestamp = LLONG_MAX;
        {
            Poco::Mutex::ScopedLock lock(m_DataMutex);
            if (bHasAudio)
            {
                iAFirstTimestamp = m_AudioPacketDatas.front()->iTimestamp;
            }
            if (bHasVideo)
            {
                iVFirstTimestamp = m_VideoPacketDatas.front()->iTimestamp;
            }
        }
#ifdef WIN32
        iBaseTimestamp = min(iAFirstTimestamp, iVFirstTimestamp);
		TRACE(L"[MP3Muxer::run]iAFirstTimestamp is %I64u\n iVFirstTimestamp is %I64u\n iBaseTimestamp is %I64u.\n",
			  iAFirstTimestamp, iVFirstTimestamp, iBaseTimestamp);
#else
        iBaseTimestamp = std::min(iAFirstTimestamp, iVFirstTimestamp);
#endif
        Poco::Clock now;
        int64_t iNowTimestamp = now.raw();
        while (!m_bRoutineStop)
        {
            bool bHasData = false;
            {
                Poco::Mutex::ScopedLock lock(m_DataMutex);
                bHasData = (!bHasAudio || !m_AudioPacketDatas.empty()) &&
                    (!bHasVideo || !m_VideoPacketDatas.empty());
            }
            if (!bHasData)
            {
                try
                {
                    m_ConsumeEvent.tryWait(10);
                }
                catch (...)
                {
                }
                continue;
            }

            int64_t iAPts = 0;
            int64_t iVPts = 0;
			int64_t iVDts = 0;
            {
                Poco::Mutex::ScopedLock lock(m_DataMutex);
                if (bHasAudio)
				{
					iAPts = m_AudioPacketDatas.front()->iTimestamp - iBaseTimestamp;
				}
                if (bHasVideo)
                {
					iVPts = m_VideoPacketDatas.front()->iTimestamp - iBaseTimestamp;
					iVDts = m_VideoPacketDatas.front()->iTimestampDTS - iBaseTimestamp;
			    }
            }

            AVFormatContext * pInFormatCtx = NULL;
            AVStream * pInStream = NULL;
            AVStream * pOutStream = NULL;
            int64_t iCurPts = 0;
			int64_t iCurDts = 0;
            int iOutStreamIndex = 0;
            std::deque<AVPacketData *> * pAVPacketDataDeque = NULL;
            MediaType eType = Audio;

            AVPacket pkt;
            av_init_packet(&pkt);

            if ((bHasVideo && bHasAudio && (iVPts <= iAPts || iVDts <= iAPts)) || // AV流都存在时
                (bHasVideo && !bHasAudio)) // 视频流存在、音频流不存在时
            {
                iOutStreamIndex = iVideoOutIndex;
                pInFormatCtx = m_pInFormatCtxV;
                pInStream = pInFormatCtx->streams[iVideoInIndex];
                pOutStream = m_pOutFormatCtx->streams[iVideoOutIndex];
                iCurPts = iVPts;
				iCurDts = iVDts;
                pAVPacketDataDeque = &m_VideoPacketDatas;
                eType = Video;
            }
            else // 音频流存在、视频流不存在
            {
                iOutStreamIndex = iAudioOutIndex;
                pInFormatCtx = m_pInFormatCtxA;
                pInStream = pInFormatCtx->streams[iAudioInIndex];
                pOutStream = m_pOutFormatCtx->streams[iAudioOutIndex];
                iCurPts = iAPts;
                pAVPacketDataDeque = &m_AudioPacketDatas;
                eType = Audio;
            }

            ReadAVFrame(&pkt, iBaseTimestamp, iCurPts, pAVPacketDataDeque);

            SetAVPacketFlag(&pkt, eType);

            pkt.pts = int64_t(iCurPts * 1.0 /
                AV_TIME_BASE / av_q2d(pInStream->time_base));
			
			//Convert PTS/DTS  
			pkt.pts = av_rescale_q_rnd(pkt.pts,
									   pInStream->time_base, pOutStream->time_base,
									   (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

			if (eType == Audio)
			{
				pkt.dts = pkt.pts;
			}
			else
			{
				pkt.dts = int64_t(iCurDts * 1.0 /
							  AV_TIME_BASE / av_q2d(pInStream->time_base));
				pkt.dts = av_rescale_q_rnd(pkt.dts,
							   pInStream->time_base, pOutStream->time_base,
							   (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			}

			//pkt.dts = pkt.pts;
			
            pkt.stream_index = iOutStreamIndex;

            //Write
            now.update();
            m_iCurTimeduration = now.raw() - iNowTimestamp;
            m_iCurTimeduration /= 1000;
            if (av_write_frame(m_pOutFormatCtx, &pkt) < 0)
			{
                sError = "Error muxing packet";
                break;
            }
            if (av_write_frame(m_pOutFormatCtx, NULL) < 0)
			{
                sError = "Error muxing packet";
                break;
            }
			// free AVPacket
			if (pkt.buf)
			{
				free(pkt.buf);
			}
			else if (pkt.data)
			{
				free(pkt.data);
			}
			av_packet_unref(&pkt);
        }

        av_write_trailer(m_pOutFormatCtx);
    } while (false);

    m_bRoutineStop = true;

    Destroy();

    if (!sError.empty())
    {
        printf("%s\n", sError.c_str());
    }
}

bool Mp3Muxer::CheckAudioType(FormatType eAType)
{
    return (mp3 == eAType || none == eAType || unkonwn == eAType);
}

bool Mp3Muxer::CheckVideoType(FormatType eVType)
{
    return (png == eVType || none == eVType || unkonwn == eVType);
}

void Mp3Muxer::Destroy()
{
    if (m_pInFormatCtxA)
    {
        if (m_bFCtxAOpened)
        {
            m_bFCtxAOpened = false;
            avformat_close_input(&m_pInFormatCtxA);
        }
        else
        {
            avformat_free_context(m_pInFormatCtxA);
        }
        m_pInFormatCtxA = NULL;
    }
    if (m_pInFormatCtxV)
    {
        if (m_bFCtxVOpened)
        {
            m_bFCtxVOpened = false;
            avformat_close_input(&m_pInFormatCtxV);
        }
        else
        {
            avformat_free_context(m_pInFormatCtxV);
        }
        m_pInFormatCtxV = NULL;
    }
    if (m_pAVIOCtxA)
    {
        av_freep(&m_pAVIOCtxA->buffer);
        av_freep(&m_pAVIOCtxA);
        m_pAVIOCtxA = NULL;
    }
    if (m_pAVIOCtxV)
    {
        av_freep(&m_pAVIOCtxV->buffer);
        av_freep(&m_pAVIOCtxV);
        m_pAVIOCtxV = NULL;
    }
    if (m_pOutFormatCtx && !(m_pOutFormatCtx->flags & AVFMT_NOFILE))
    {
        avformat_free_context(m_pOutFormatCtx);
        m_pOutFormatCtx = NULL;
    }
    if (m_pAVIOCtxOut)
    {
        av_freep(&m_pAVIOCtxOut->buffer);
        av_freep(&m_pAVIOCtxOut);
        m_pAVIOCtxOut = NULL;
    }
    Poco::Mutex::ScopedLock lock(m_DataMutex);
    for (int i = 0; i < m_AudioPacketDatas.size(); ++i)
    {
        delete[] m_AudioPacketDatas[i]->pData;
        delete m_AudioPacketDatas[i];
    }
    m_AudioPacketDatas.clear();
    for (int i = 0; i < m_VideoPacketDatas.size(); ++i)
    {
        delete[] m_VideoPacketDatas[i]->pData;
        delete m_VideoPacketDatas[i];
    }
    m_VideoPacketDatas.clear();
}

AVInputFormat *Mp3Muxer::GetAVInputFormat(FormatType eType)
{
    AVInputFormat * pInputFormat = NULL;
    switch (eType)
    {
    case g711a:
        pInputFormat = av_find_input_format("alaw");
        break;
    case g711u:
        pInputFormat = av_find_input_format("mulaw");
        break;
    case aaclc:
        pInputFormat = av_find_input_format("aac");
        break;
	case mp3:
		pInputFormat = av_find_input_format("mp3");
		break;
	case png:
		pInputFormat = av_find_input_format("image2");
		break;
    case h264:
        pInputFormat = av_find_input_format("h264");
        break;
    case h265:
        pInputFormat = av_find_input_format("hevc");
        break;
    default:
        break;
    }
    return pInputFormat;
}

void Mp3Muxer::ReadAVFrame(
    AVPacket *pkt,
    int64_t iReferenceTimestamp,
    int64_t iCurPts,
    std::deque<AVPacketData *> *pAVPacketDataDeque)
{
    pkt->data = NULL;
    pkt->size = 0;
    pkt->flags = 0;
    while (!m_bTerminateConsume)
    {
        AVPacketData * pAVPacketData = NULL;
        bool bHasVideoData = true;
        {
            Poco::Mutex::ScopedLock lock(m_DataMutex);
            if (!pAVPacketDataDeque->empty())
            {
                pAVPacketData = pAVPacketDataDeque->front();
            }
        }
        if (!pAVPacketData)
        {
            try
            {
                m_ConsumeEvent.tryWait(10);
            }
            catch (...)
            {
            }
            continue;
        }

        if (iCurPts == pAVPacketData->iTimestamp - iReferenceTimestamp)
        {
            pkt->data = (uint8_t*)realloc(pkt->data,
						pAVPacketData->iDataSize + pkt->size);
            memcpy(pkt->data + pkt->size,
                pAVPacketData->pData, pAVPacketData->iDataSize);
			pkt->size += (pAVPacketData->iDataSize);
            delete[] pAVPacketData->pData;
            delete pAVPacketData;
            Poco::Mutex::ScopedLock lock(m_DataMutex);
            pAVPacketDataDeque->pop_front();
        }
        else
        {
            break;
        }
    }
}

void Mp3Muxer::SetAVPacketFlag(AVPacket *pkt, MediaType eType)
{
    pkt->flags |= AV_PKT_FLAG_KEY;
}

