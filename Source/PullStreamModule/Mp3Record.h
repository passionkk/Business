#ifndef _MP3Record_H_
#define _MP3Record_H_

/************************************************************************/
/*
	�ļ�˵��	��	MP3�ļ�¼����
	����		��	������Ƶ����¼�Ƴ�MP3�ļ�,֧��¼�ƹ�������ͣ�ָ����ָ��ļ���
*/
/************************************************************************/

#include "Mp3Muxer.h"
#include "../Common/RecordBase.h"
#include <Poco/Clock.h>
#include <Poco/FileStream.h>

class Mp3Record
	: public RecordBase
	, public Mp3DataIface
	, public Poco::Runnable
{
public:

	struct Mp3PacketData
	{
		uint8_t*	pData;
		int			iDataSize;
		int			iOffset;
		uint64_t	iTimeCode;
		uint64_t	iTimeCodeDTS;
	};

	Mp3Record();
	~Mp3Record();

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

	int WriteHeader(Poco::FileStream& mp3Stream);
	int WriteTailer(Poco::FileStream& mp3FStream);
private:
	// release resource
	void Destroy();

	//���ڷָ�ʱ�����µ��ļ�·�������ɷ�ʽ�����������ʱ��ȡ�
	std::string GenerateFilePath();
private:
	bool				m_bStop;
	Poco::Thread		m_thread;
	Mp3Muxer			m_mp3Muxer;
	Poco::Mutex			m_DataMutex;
	std::deque<Mp3PacketData *> m_mp3PacketDatas;
	uint64_t			m_i64StartRecord;
	Poco::Clock			m_clkPauseOnce; //���ڼ�����ͣһ�εĳ���ʱ��
	bool				m_bEnableVideo;
	bool				m_bEnableAudio;
	bool				m_bFirstPacket;
	unsigned char*		m_pHeaderData;
	int					m_nHeaderDataLen;
};

#endif
