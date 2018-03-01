#ifndef _FLVRecord_H_
#define _FLVRecord_H_

/************************************************************************/
/*
	文件说明	：	FLV文件录制类
	功能		：	将音视频数据录制成FLV文件,支持录制过程中暂停恢复，分割文件。
*/
/************************************************************************/

#include "FlvMuxer.h"
#include "../Common/RecordBase.h"
#include <Poco/Clock.h>
#include <Poco/FileStream.h>

class FlvRecord
	: public RecordBase
	, public FlvDataIface
	, public Poco::Runnable
{
public:

	struct FlvPacketData
	{
		uint8_t*	pData;
		int			iDataSize;
		int			iOffset;
		int64_t	iTimeCode;
		int64_t	iTimeCodeDTS;
	};

	FlvRecord();
	virtual ~FlvRecord();

	// 导入音视频数据
	bool ImportAVPacket(
		MediaType eMediaType,
		FormatType eFormatType,
		uint8_t *pData,
		int iDataSize,
		int64_t iTimestamp /*us*/,	
		int64_t iTimestampDTS /*us*/);

	// 开始录制
	bool Start(
		FormatType eAFormatType,
		FormatType eVFormatType,
		const std::string strAbFilePath);

	// 停止录制
	void Stop();

	// 暂停录制 这里注意丢包
	bool Pause();

	// 继续录制
	bool Resume();

	// 后台线程是否都在运行
	bool IsRunning() const;

protected:
	// push ts data to deque
	void DataHandle(
		void *pUserData,
		const uint8_t *pData,
		uint32_t iLen,
		int64_t iPts /*ms*/);

	// get ts data from deque then write to file and split file if need.
	void run();

	int WriteHeader(Poco::FileStream& flvStream, int64_t iPts = -1);
	int WriteTailer(Poco::FileStream& flvFStream, int64_t iLastPts);

	int64_t UpdatePacketPts(FlvPacketData* pPacketData, int64_t i64FirstPacketPts);

private:
	// release resource
	void Destroy();

	//用于分割时生成新的文件路径，生成方式待定，如根据时间等。
	std::string GenerateFilePath();
private:
	bool				m_bStop;
	Poco::Thread		m_thread;
	FlvMuxer			m_FlvMuxer;
	Poco::Mutex			m_DataMutex;
	std::deque<FlvPacketData *> m_FlvPacketDatas;
	int64_t				m_i64StartRecord;
	int64_t				m_i64FirstPacketPts;
	Poco::Clock			m_clkPauseOnce; //用于计算暂停一次的持续时长
	bool				m_bEnableVideo;
	bool				m_bEnableAudio;
	bool				m_bFirstPacket;
	unsigned char*		m_pHeaderData;
	int					m_nHeaderDataLen;
};

#endif
