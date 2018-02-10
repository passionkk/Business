#include "stdafx.h"
#include "FlvRecord.h"

FlvRecord::FlvRecord()
	: RecordBase()
	, m_bStop(true)
	, m_i64StartRecord(0)
	, m_bEnableVideo(true)
	, m_bEnableAudio(true)
	, m_bFirstPacket(true)
	, m_nHeaderDataLen(1024)
{
	m_pHeaderData = new unsigned char[m_nHeaderDataLen];
	m_FlvMuxer.Subscribe(this, NULL);
}


FlvRecord::~FlvRecord()
{
	if (m_pHeaderData != nullptr)
		delete[] m_pHeaderData;
	m_pHeaderData = NULL;
}

bool FlvRecord::ImportAVPacket(MediaType eMediaType, FormatType eFormatType, uint8_t *pData, int iDataSize, uint64_t iTimestamp, uint64_t iTimestampDTS)
{
	if ((eMediaType == Audio && m_bEnableAudio) || (eMediaType == Video && m_bEnableVideo))
	{
#if WIN32
		//TRACE(L"[FlvRecord::ImportAVPacket]eMediaType is %d, iTimestamp is %lld, iTimestampDTS is %lld.\n", eMediaType, iTimestamp, iTimestampDTS);
#endif
		return m_FlvMuxer.ImportAVPacket(eMediaType, eFormatType, pData, iDataSize, iTimestamp, iTimestampDTS);// iTimestampDTS);
	}
	return false;
}

bool FlvRecord::Start(FormatType eAFormatType, FormatType eVFormatType, const std::string strAbFilePath)
{
	bool bRet = false;

	if (!m_FlvMuxer.IsRunning() && m_thread.isRunning())
		Stop();

	if (!m_thread.isRunning())
	{
		if (eAFormatType == none)
			m_bEnableAudio = false;
		if (eVFormatType == none)
			m_bEnableVideo = false;
		bRet = m_FlvMuxer.Start(eAFormatType, eVFormatType);
		if (bRet)
		{
			m_strAbFilePath = strAbFilePath;
			m_bStop = false;
			m_bFirstPacket = true;
			m_thread.start(*this);
			m_RecordState = RS_Record;
		}
	}
	return bRet;
}

void FlvRecord::Stop()
{
	m_RecordState = RS_Stop;
	if (m_bStop)
	{
		return;
	}
	m_bStop = true;
	m_FlvMuxer.Stop();
	m_thread.join();
	m_nPausedTime = 0;
	m_nRecordedDuration = 0;
	m_nRecordedSize = 0;
	m_i64StartRecord = 0;
	Destroy();
}

bool FlvRecord::Pause()
{
	bool bRet = true;
	if (m_RecordState == RS_Record)
	{
		m_RecordState = RS_Pause;
		m_clkPauseOnce.update();
	}
	else
	{
		bRet = false;
	}
	return bRet;
}

bool FlvRecord::Resume()
{
	bool bRet = true;
	if (m_RecordState == RS_Pause)
	{
		m_RecordState = RS_Record;
		Poco::Clock clkTmp;
		clkTmp.update();
		int64_t i64Diff = clkTmp - m_clkPauseOnce;
		m_nPausedTime += static_cast<int>(i64Diff / 1000);
	}
	else
	{
		bRet = false;
	}
	return bRet;
}

bool FlvRecord::IsRunning() const
{
	return m_thread.isRunning() && m_FlvMuxer.IsRunning();
}

void FlvRecord::DataHandle(void *pUserData, const uint8_t *pData, uint32_t iLen, uint64_t iPts/*, uint64_t iDts*/)
{
	if (m_RecordState == RS_Record)
	{
		FlvPacketData *pTSPacketData = new FlvPacketData;
		pTSPacketData->pData = new uint8_t[iLen];
		pTSPacketData->iDataSize = iLen;
		pTSPacketData->iOffset = 0;
		pTSPacketData->iTimeCode = iPts;
		//pTSPacketData->iTimeCodeDTS = iDts;
		memcpy(pTSPacketData->pData, pData, iLen);
		{
			Poco::Mutex::ScopedLock lock(m_DataMutex);
			m_FlvPacketDatas.push_back(pTSPacketData);
		}
	}
}

void FlvRecord::run()
{
	bool bNeedSplit = true;
	bool bEmpty = false;
	FlvPacketData *pPacket = NULL;
	Poco::FileStream flvFStream;
	bool bCloseFile = false;
	m_i64StartRecord = 0;
	uint64_t i64LastPacketTime = 0;
	while (!m_bStop)
	{
		m_DataMutex.lock();
		bEmpty = m_FlvPacketDatas.empty();
		m_DataMutex.unlock();
		if (bEmpty && !m_bStop)
		{
			Poco::Thread::sleep(1);
			continue;
		}
		if (bNeedSplit)
		{
			std::string strFilePath = GenerateFilePath();
			flvFStream.open(strFilePath, std::ios::in);
			bNeedSplit = false;
			m_nRecordedSize = 0;
			m_i64StartRecord = i64LastPacketTime;
			m_nPausedTime = 0;
			m_nRecordedDuration = 0;
			bCloseFile = false;
			if (!m_bFirstPacket)
			{
				WriteHeader(flvFStream);
#if WIN32
				TRACE(L"[FlvRecord::run]write Header.\n");
#endif
			}
		}
		Poco::Mutex::ScopedLock lock(m_DataMutex);
		pPacket = m_FlvPacketDatas.front();
		if (m_bFirstPacket)
		{
			m_bFirstPacket = false;
			if (m_nHeaderDataLen < pPacket->iDataSize)
			{
				if (m_pHeaderData != nullptr)
				{
					delete[] m_pHeaderData;
				}
				m_nHeaderDataLen = pPacket->iDataSize;
				m_pHeaderData = new unsigned char[m_nHeaderDataLen];
			}
			m_nHeaderDataLen = pPacket->iDataSize;
			memcpy(m_pHeaderData, pPacket->pData, m_nHeaderDataLen);
		}
		flvFStream.write((const char*)pPacket->pData, pPacket->iDataSize);
		m_nRecordedSize += pPacket->iDataSize;
		if (m_nSplitType == 0)
		{
			m_nRecordedDuration = pPacket->iTimeCode - m_i64StartRecord - m_nPausedTime;
			if (m_nRecordedDuration >= m_nSplitDuration)
			{
				bNeedSplit = true;
				flvFStream.close();
				bCloseFile = true;
			}
		}
		else if (m_nSplitType == 1)
		{
			if (m_nRecordedSize >= m_nSplitSize)
			{
				bNeedSplit = true;
				flvFStream.close();
				bCloseFile = true;
			}
		}
		if (m_i64StartRecord == 0)
		{
			m_i64StartRecord = pPacket->iTimeCode;
		}
		i64LastPacketTime = pPacket->iTimeCode;

		delete[] pPacket->pData;
		delete pPacket;
		m_FlvPacketDatas.pop_front();
	}
	if(!bCloseFile)
		flvFStream.close();
}

int FlvRecord::WriteHeader(Poco::FileStream& flvFStream, uint64_t uintPts)
{
	int nRet = 0;
	//FLV Header
	//FLV
	flvFStream.write("FLV", 3);
	//Version
	flvFStream.write("1", 1);
	//Flags  第6位标识存在音频tag，第8位标识存在视频tag，其余为0
	unsigned char ch = (FLV_HEADER_FLAG_HASAUDIO * m_bEnableAudio + FLV_HEADER_FLAG_HASVIDEO * m_bEnableVideo);
	std::string strWrite = "";
	strWrite = ch;
	flvFStream.write(strWrite.c_str(), 1);
	//HeaderSize 4个字节，总是9
	flvFStream.write("0009", 4);
	
	//FLV Body
	//Previous Tag Size 前一个Tag的长度
	flvFStream.write("0000", 4);
	
	//MetaData
	/* write meta_tag */
	ch = FLV_TAG_TYPE_META; // tag type META
	strWrite.clear();
	strWrite = ch;
	flvFStream.write(strWrite.c_str(), 1);

	//flv->metadata_size_pos = avio_tell(pb);
	flvFStream.write("000", 3);// size of data part (sum of all parts below)

	//avio_wb24(pb, ts);          // timestamp
	//avio_wb32(pb, 0);           // reserved

	//CodecHeader

	return nRet;
}

int FlvRecord::WriteHeader(Poco::FileStream& flvStream)
{
	int nRet = -1;
	if (m_pHeaderData && m_nHeaderDataLen > 0)
	{
		flvStream.write((const char*)m_pHeaderData, m_nHeaderDataLen);
		nRet = 0;
	}
	return nRet;
}

int FlvRecord::WriteTailer(Poco::FileStream& flvFStream)
{
	int nRet = 0;
	return nRet;
}

void FlvRecord::Destroy()
{
	Poco::Mutex::ScopedLock lock(m_DataMutex);
	for (int i = 0; i < m_FlvPacketDatas.size(); ++i)
	{
		delete[] m_FlvPacketDatas[i]->pData;
		delete m_FlvPacketDatas[i];
	}
	m_FlvPacketDatas.clear();
}

std::string FlvRecord::GenerateFilePath()
{
	std::string strRet = "";
	if (!m_strAbFilePath.empty())
	{
		size_t nIndex = m_strAbFilePath.find_last_of('.');
		if (nIndex == std::string::npos)
		{
			strRet = m_strAbFilePath;
		}
		else
		{
			strRet = m_strAbFilePath.substr(0, nIndex);
		}
		time_t timeNow;
		tm* localTime;
		char buf[128] = { 0 };
		timeNow = time(NULL);
		localTime = localtime(&timeNow); //转为本地时间
		strftime(buf, 64, "%Y_%m_%d_%H_%M_%S", localTime);
		strRet += "_";
		strRet += buf;
		strRet += ".flv";
		}
	return strRet;
}