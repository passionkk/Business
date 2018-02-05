#ifndef _TSRecord_H_
#define _TSRecord_H_

/************************************************************************/
/*
	�ļ�˵��	��	TS�ļ�¼����
	����		��	������Ƶ����¼�Ƴ�TS�ļ�,֧��¼�ƹ�������ͣ�ָ����ָ��ļ���
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

private:
	// release resource
	void Destroy();

	//���ڷָ�ʱ�����µ��ļ�·�������ɷ�ʽ�����������ʱ��ȡ�
	std::string GenerateFilePath();
private:
	bool				m_bStop;
	Poco::Thread		m_thread;
	TSMuxer				m_TSMuxer;
	Poco::Mutex			m_DataMutex;
	std::deque<TSPacketData *> m_TSPacketDatas;
	uint64_t			m_i64StartRecord;
	Poco::Clock			m_clkPauseOnce; //���ڼ�����ͣһ�εĳ���ʱ��
	bool				m_bEnableVideo;
	bool				m_bEnableAudio;
};

#endif
