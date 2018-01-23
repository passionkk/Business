#ifndef _RecordBase_H_
#define _RecordBase_H_

#include <stdint.h>
#include <string>

extern "C"
{
#include <libavformat/avformat.h> 
}

#ifdef WIN32
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#else
#endif

enum EventType
{
	Data = 1,
};

enum Record_State
{
	RS_Stop		= 0,
	RS_Record	= 1,
	RS_Pause	= 2,
	RS_Count,
};

enum MediaType
{
	Video = 0,
	Audio = 1,
};

enum FormatType
{
	none = -2, // ��ǰ��������
	unkonwn = -1, // ��ָ��AVInputFormat��FFmpeg�ڲ�̽���������ʽ
	g711a = 19,
	g711u = 20,
	aaclc = 42,
	h264 = 96,
	h265 = 265,
};
/************************************************************************/
/*
	�ļ�˵��	��	¼�ƹ��ܻ���
	����		��	������Ƶ����¼�Ƴ��ļ�,֧��¼�ƹ�������ͣ�ָ����ָ��ļ��ȡ�
*/
/************************************************************************/

class RecordBase
{
public:
	RecordBase();
	~RecordBase();

	// ��������Ƶ����
	virtual bool ImportAVPacket(
		MediaType eMediaType,
		FormatType eFormatType,
		uint8_t *pData,
		int iDataSize,
		uint64_t iTimestamp /*us*/) = 0;

	// ��ʼ
	virtual bool Start(
		FormatType eAFormatType,
		FormatType eVFormatType,
		const std::string strAbFilePath /*absolute file path*/) = 0;

	// ֹͣ
	virtual void Stop() = 0;

	// ��ͣ¼��
	virtual bool Pause() = 0;

	// ����¼��
	virtual bool Resume() = 0;

	// ���÷ָ�ʱ������λ��
	void SetSplitDuration(int nSecond);

	// ���÷ָ��С����λMB
	void SetSplitSize(int nMB);

protected:
	std::string		m_strAbFilePath;		//¼���ļ�����·��
	int				m_nSplitType;			//�ָʽ��0�����ļ�ʱ����1�����ļ���С
	int				m_nSplitDuration;		//�ָ�ʱ��
	int				m_nSplitSize;			//�ָ��С
	Record_State	m_RecordState;			//¼��״̬
	int				m_nRecordedDuration;	//��¼��ʱ��
	int				m_nRecordedSize;		//��¼�ƴ�С
	int				m_nPausedTime;			//��ͣ��ʱ��
};

bool InitEnv();
#endif