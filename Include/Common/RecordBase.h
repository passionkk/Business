#ifndef _RecordBase_H_
#define _RecordBase_H_

#include "CommonDef.h"
#include <stdint.h>
#include <string>


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
	virtual ~RecordBase();

	// ��������Ƶ����
	virtual bool ImportAVPacket(
		MediaType eMediaType,
		FormatType eFormatType,
		uint8_t *pData,
		int iDataSize,
		int64_t iTimestamp /*us*/,
		int64_t iTimestampDTS) = 0;

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
	int64_t			m_nRecordedDuration;	//��¼��ʱ��
	int				m_nRecordedSize;		//��¼�ƴ�С
	int				m_nPausedTime;			//��ͣ��ʱ��
};

bool InitEnv();
#endif