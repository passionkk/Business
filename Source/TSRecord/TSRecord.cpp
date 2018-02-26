#include "stdafx.h"
#include "TSRecord.h"
#include <Poco/FileStream.h>

TSRecord::TSRecord()
	: RecordBase()
	, m_bStop(true)
	, m_i64StartRecord(0)
	, m_bEnableVideo(true)
	, m_bEnableAudio(true)
{
	m_TSMuxer.Subscribe(this, NULL);
}


TSRecord::~TSRecord()
{
}

bool TSRecord::ImportAVPacket(MediaType eMediaType, FormatType eFormatType, uint8_t *pData, int iDataSize, int64_t iTimestamp, int64_t iTimestampDTS)
{
	if ((eMediaType == Audio && m_bEnableAudio) || (eMediaType == Video && m_bEnableVideo))
	{
#if WIN32
		//TRACE(L"[TSRecord::ImportAVPacket]eMediaType is %d, iTimestamp is %lld, iTimestampDTS is %lld.\n", eMediaType, iTimestamp, iTimestampDTS);
#endif
		return m_TSMuxer.ImportAVPacket(eMediaType, eFormatType, pData, iDataSize, iTimestamp, iTimestampDTS);// iTimestampDTS);
	}
	return false;
}

bool TSRecord::Start(FormatType eAFormatType, FormatType eVFormatType, const std::string strAbFilePath)
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

void TSRecord::Stop()
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

bool TSRecord::Pause()
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

bool TSRecord::Resume()
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

bool TSRecord::IsRunning() const
{
	return m_thread.isRunning() && m_TSMuxer.IsRunning();
}

void TSRecord::DataHandle(void *pUserData, const uint8_t *pData, uint32_t iLen, int64_t iPts)
{
	if (m_RecordState == RS_Record)
	{
		TSPacketData *pTSPacketData = new TSPacketData;
		pTSPacketData->pData = new uint8_t[iLen];
		pTSPacketData->iDataSize = iLen;
		pTSPacketData->iOffset = 0;
		pTSPacketData->iTimeCode = iPts;
		memcpy(pTSPacketData->pData, pData, iLen);
		{
			Poco::Mutex::ScopedLock lock(m_DataMutex);
			m_TSPacketDatas.push_back(pTSPacketData);
		}
	}
}

void TSRecord::run()
{
	bool bNeedSplit = true;
	bool bEmpty = false;
	TSPacketData *pPacket = NULL;
	Poco::FileStream tsFStream;
	bool bCloseFile = false;
	m_i64StartRecord = 0;
	int64_t i64LastPacketTime = 0;
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
			tsFStream.open(strFilePath, std::ios::in);
			bNeedSplit = false;
			m_nRecordedSize = 0;
			m_i64StartRecord = i64LastPacketTime;
			m_nPausedTime = 0;
			m_nRecordedDuration = 0;
			bCloseFile = false;
			
			//Destroy();
			//continue;
		}
		Poco::Mutex::ScopedLock lock(m_DataMutex);
		pPacket = m_TSPacketDatas.front();
		tsFStream.write((const char*)pPacket->pData, pPacket->iDataSize);
		m_nRecordedSize += pPacket->iDataSize;
		if (m_nSplitType == 0)
		{
			m_nRecordedDuration = pPacket->iTimeCode - m_i64StartRecord - m_nPausedTime;
			if (m_nRecordedDuration >= m_nSplitDuration)
			{
				bNeedSplit = true;
				tsFStream.close();
				bCloseFile = true;
			}
		}
		else if (m_nSplitType == 1)
		{
			if (m_nRecordedSize >= m_nSplitSize)
			{
				bNeedSplit = true;
				tsFStream.close();
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
		tsFStream.close();
}

void TSRecord::Destroy()
{
	Poco::Mutex::ScopedLock lock(m_DataMutex);
	for (int i = 0; i < m_TSPacketDatas.size(); ++i)
	{
		delete[] m_TSPacketDatas[i]->pData;
		delete m_TSPacketDatas[i];
	}
	m_TSPacketDatas.clear();
}

std::string TSRecord::GenerateFilePath()
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
		strRet += ".ts";
		}
	return strRet;
}