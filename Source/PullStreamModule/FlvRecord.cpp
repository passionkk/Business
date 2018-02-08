#include "stdafx.h"
#include "FlvRecord.h"

FlvRecord::FlvRecord()
	: RecordBase()
	, m_bStop(true)
	, m_i64StartRecord(0)
	, m_bEnableVideo(true)
	, m_bEnableAudio(true)
{
	m_TSMuxer.Subscribe(this, NULL);
}


FlvRecord::~FlvRecord()
{
}

bool FlvRecord::ImportAVPacket(MediaType eMediaType, FormatType eFormatType, uint8_t *pData, int iDataSize, uint64_t iTimestamp, uint64_t iTimestampDTS)
{
	if ((eMediaType == Audio && m_bEnableAudio) || (eMediaType == Video && m_bEnableVideo))
	{
#if WIN32
		//TRACE(L"[FlvRecord::ImportAVPacket]eMediaType is %d, iTimestamp is %lld, iTimestampDTS is %lld.\n", eMediaType, iTimestamp, iTimestampDTS);
#endif
		return m_TSMuxer.ImportAVPacket(eMediaType, eFormatType, pData, iDataSize, iTimestamp, iTimestampDTS);// iTimestampDTS);
	}
	return false;
}

bool FlvRecord::Start(FormatType eAFormatType, FormatType eVFormatType, const std::string strAbFilePath)
{
	bool bRet = false;

	if (!m_TSMuxer.IsRunning() && m_thread.isRunning())
		Stop();

	if (!m_thread.isRunning())
	{
		if (eAFormatType == none)
			m_bEnableAudio = false;
		if (eVFormatType == none)
			m_bEnableVideo = false;
		bRet = m_TSMuxer.Start(eAFormatType, eVFormatType);
		if (bRet)
		{
			m_strAbFilePath = strAbFilePath;
			m_bStop = false;
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
	m_TSMuxer.Stop();
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
	return m_thread.isRunning() && m_TSMuxer.IsRunning();
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
			m_TSPacketDatas.push_back(pTSPacketData);
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
		bEmpty = m_TSPacketDatas.empty();
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
		}
		Poco::Mutex::ScopedLock lock(m_DataMutex);
		pPacket = m_TSPacketDatas.front();
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
		m_TSPacketDatas.pop_front();
	}
	if(!bCloseFile)
		flvFStream.close();
}

int FlvRecord::WriteHeader(Poco::FileStream& flvFStream)
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
	for (int i = 0; i < m_TSPacketDatas.size(); ++i)
	{
		delete[] m_TSPacketDatas[i]->pData;
		delete m_TSPacketDatas[i];
	}
	m_TSPacketDatas.clear();
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