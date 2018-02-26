#include "stdafx.h"
#include "Mp3Record.h"

Mp3Record::Mp3Record()
	: RecordBase()
	, m_bStop(true)
	, m_i64StartRecord(0)
	, m_bEnableVideo(true)
	, m_bEnableAudio(true)
	, m_bFirstPacket(true)
	, m_nHeaderDataLen(1024)
{
	m_pHeaderData = new unsigned char[m_nHeaderDataLen];
	m_mp3Muxer.Subscribe(this, NULL);
}


Mp3Record::~Mp3Record()
{
	if (m_pHeaderData != nullptr)
		delete[] m_pHeaderData;
	m_pHeaderData = NULL;
}

bool Mp3Record::ImportAVPacket(MediaType eMediaType, FormatType eFormatType, uint8_t *pData, int iDataSize, int64_t iTimestamp, int64_t iTimestampDTS)
{
	if ((eMediaType == Audio && m_bEnableAudio) || (eMediaType == Video && m_bEnableVideo))
	{
#if WIN32
		//TRACE(L"[Mp3Record::ImportAVPacket]eMediaType is %d, iTimestamp is %lld, iTimestampDTS is %lld.\n", eMediaType, iTimestamp, iTimestampDTS);
#endif
		return m_mp3Muxer.ImportAVPacket(eMediaType, eFormatType, pData, iDataSize, iTimestamp, iTimestampDTS);// iTimestampDTS);
	}
	return false;
}

bool Mp3Record::Start(FormatType eAFormatType, FormatType eVFormatType, const std::string strAbFilePath)
{
	bool bRet = false;

	if (!m_mp3Muxer.IsRunning() && m_thread.isRunning())
		Stop();

	if (!m_thread.isRunning())
	{
		if (eAFormatType == none)
			m_bEnableAudio = false;
		if (eVFormatType == none)
			m_bEnableVideo = false;
		bRet = m_mp3Muxer.Start(eAFormatType, eVFormatType);
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

void Mp3Record::Stop()
{
	m_RecordState = RS_Stop;
	if (m_bStop)
	{
		return;
	}
	m_bStop = true;
	m_mp3Muxer.Stop();
	m_thread.join();
	m_nPausedTime = 0;
	m_nRecordedDuration = 0;
	m_nRecordedSize = 0;
	m_i64StartRecord = 0;
	Destroy();
}

bool Mp3Record::Pause()
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

bool Mp3Record::Resume()
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

bool Mp3Record::IsRunning() const
{
	return m_thread.isRunning() && m_mp3Muxer.IsRunning();
}

void Mp3Record::DataHandle(void *pUserData, const uint8_t *pData, uint32_t iLen, int64_t iPts)
{
	if (m_RecordState == RS_Record)
	{
		Mp3PacketData *pTSPacketData = new Mp3PacketData;
		pTSPacketData->pData = new uint8_t[iLen];
		pTSPacketData->iDataSize = iLen;
		pTSPacketData->iOffset = 0;
		pTSPacketData->iTimeCode = iPts;
		memcpy(pTSPacketData->pData, pData, iLen);
		{
			Poco::Mutex::ScopedLock lock(m_DataMutex);
			m_mp3PacketDatas.push_back(pTSPacketData);
		}
	}
}

void Mp3Record::run()
{
	bool bNeedSplit = true;
	bool bEmpty = false;
	Mp3PacketData *pPacket = NULL;
	Poco::FileStream flvFStream;
	bool bCloseFile = false;
	m_i64StartRecord = 0;
	int64_t i64LastPacketTime = 0;
	while (!m_bStop)
	{
		m_DataMutex.lock();
		bEmpty = m_mp3PacketDatas.empty();
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
				//WriteHeader(flvFStream);
#if WIN32
				TRACE(L"[FlvRecord::run]write Header.\n");
#endif
			}
		}
		Poco::Mutex::ScopedLock lock(m_DataMutex);
		pPacket = m_mp3PacketDatas.front();
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
		m_mp3PacketDatas.pop_front();
	}
	if(!bCloseFile)
		flvFStream.close();
}

int Mp3Record::WriteHeader(Poco::FileStream& mp3Stream)
{
	int nRet = -1;
	if (m_pHeaderData && m_nHeaderDataLen > 0)
	{
		mp3Stream.write((const char*)m_pHeaderData, m_nHeaderDataLen);
		nRet = 0;
	}
	return nRet;
}

int Mp3Record::WriteTailer(Poco::FileStream& flvFStream)
{
	int nRet = 0;
	return nRet;
}

void Mp3Record::Destroy()
{
	Poco::Mutex::ScopedLock lock(m_DataMutex);
	for (int i = 0; i < m_mp3PacketDatas.size(); ++i)
	{
		delete[] m_mp3PacketDatas[i]->pData;
		delete m_mp3PacketDatas[i];
	}
	m_mp3PacketDatas.clear();
}

std::string Mp3Record::GenerateFilePath()
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
		strRet += ".mp3";
		}
	return strRet;
}