#ifndef _TSRecord_H_
#define _TSRecord_H_

/************************************************************************/
/*
	文件说明	：	TS文件录制类
	功能		：	将音视频数据录制成TS文件,支持录制过程中暂停恢复，分割文件。
*/
/************************************************************************/

#include "TSMuxer.h"
#include "../Common/RecordBase.h"
#include <Poco/Clock.h>

class TSRecord
	: public RecordBase
	, public TSDataIface
	, public Poco::Runnable
{
public:

	struct TSPacketData
	{
		uint8_t*	pData;
		int			iDataSize;
		int			iOffset;
		uint64_t	iTimeCode;
		uint64_t	iTimeCodeDTS;
	};

	TSRecord();
	~TSRecord();

	// 导入音视频数据
	bool ImportAVPacket(
		MediaType eMediaType,
		FormatType eFormatType,
		uint8_t *pData,
		int iDataSize,
		uint64_t iTimestamp /*us*/,	
		uint64_t iTimestampDTS /*us*/);

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
		uint64_t iPts /*ms*/ /*,uint64_t iDts*/);

	// get ts data from deque then write to file and split file if need.
	void run();

private:
	// release resource
	void Destroy();

	//用于分割时生成新的文件路径，生成方式待定，如根据时间等。
	std::string GenerateFilePath();
private:
	bool				m_bStop;
	Poco::Thread		m_thread;
	TSMuxer				m_TSMuxer;
	Poco::Mutex			m_DataMutex;
	std::deque<TSPacketData *> m_TSPacketDatas;
	uint64_t			m_i64StartRecord;
	Poco::Clock			m_clkPauseOnce; //用于计算暂停一次的持续时长
	bool				m_bEnableVideo;
	bool				m_bEnableAudio;
};

#endif
