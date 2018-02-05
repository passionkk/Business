#include "CommonDef.h"

FFmpegInit* FFmpegInit::m_pInstance = NULL;

FormatType GetFormatTypeByAVCodecID(AVCodecID codecID)
{
	FormatType fmt = none;
	switch (codecID)
	{
	case AV_CODEC_ID_H264:
		fmt = h264;
		break;
	case AV_CODEC_ID_HEVC:
		fmt = h265;
		break;
	case AV_CODEC_ID_AAC:
		fmt = aaclc;
		break;
	case AV_CODEC_ID_PCM_ALAW:
		fmt = g711a;
		break;
	case AV_CODEC_ID_PCM_MULAW:
		fmt = g711u;
		break;
	case AV_CODEC_ID_OPUS:
		fmt = opus;
		break;
	default:
		break;
	}
	return fmt;
}

FFmpegInit::FFmpegInit()
{

}

FFmpegInit::~FFmpegInit()
{

}

FFmpegInit * FFmpegInit::Initialize()
{
	return FFmpegInit::GetInstance();
}

void FFmpegInit::Uninitialize()
{
	if (m_pInstance != NULL)
	{
		m_pInstance->UnInit();
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

FFmpegInit * FFmpegInit::GetInstance()
{
	if (m_pInstance == NULL)
	{
		m_pInstance = new FFmpegInit;
		m_pInstance->Init();
	}

	return m_pInstance;
}

void FFmpegInit::Init()
{
	av_register_all();
	avcodec_register_all();
	avformat_network_init();
}

void FFmpegInit::UnInit()
{
	avformat_network_deinit();

}