#ifndef _FLVRecord_H_
#define _FLVRecord_H_

/************************************************************************/
/*
	�ļ�˵��	��	FLV�ļ�¼����
	����		��	������Ƶ����¼�Ƴ�FLV�ļ�,֧��¼�ƹ�������ͣ�ָ����ָ��ļ���
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
		uint64_t	iTimeCode;
		uint64_t	iTimeCodeDTS;
	};

	FlvRecord();
	~FlvRecord();

	// ��������Ƶ����
	bool ImportAVPacket(
		MediaType eMediaType,
		FormatType eFormatType,
		uint8_t *pData,
		int iDataSize,
		uint64_t iTimestamp /*us*/,	
		uint64_t iTimestampDTS /*us*/);

	// ��ʼ¼��
	bool Start(
		FormatType eAFormatType,
		FormatType eVFormatType,
		const std::string strAbFilePath);

	// ֹͣ¼��
	void Stop();

	// ��ͣ¼�� ����ע�ⶪ��
	bool Pause();

	// ����¼��
	bool Resume();

	// ��̨�߳��Ƿ�������
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

	int WriteHeader(Poco::FileStream& flvStream);
	int WriteTailer(Poco::FileStream& flvFStream);

	int WriteHeader(Poco::FileStream& flvFStream, uint64_t uintPts);

private:
	// release resource
	void Destroy();

	//���ڷָ�ʱ�����µ��ļ�·�������ɷ�ʽ�����������ʱ��ȡ�
	std::string GenerateFilePath();
private:
	bool				m_bStop;
	Poco::Thread		m_thread;
	FlvMuxer				m_FlvMuxer;
	Poco::Mutex			m_DataMutex;
	std::deque<FlvPacketData *> m_FlvPacketDatas;
	uint64_t			m_i64StartRecord;
	Poco::Clock			m_clkPauseOnce; //���ڼ�����ͣһ�εĳ���ʱ��
	bool				m_bEnableVideo;
	bool				m_bEnableAudio;
	bool				m_bFirstPacket;
	unsigned char*		m_pHeaderData;
	int					m_nHeaderDataLen;
};

#endif
