#ifndef _RecordBase_H_
#define _RecordBase_H_

#include "CommonDef.h"
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

/************************************************************************/
/*
	文件说明	：	录制功能基类
	功能		：	将音视频数据录制成文件,支持录制过程中暂停恢复，分割文件等。
*/
/************************************************************************/

class RecordBase
{
public:
	RecordBase();
	~RecordBase();

	// 导入音视频数据
	virtual bool ImportAVPacket(
		MediaType eMediaType,
		FormatType eFormatType,
		uint8_t *pData,
		int iDataSize,
		uint64_t iTimestamp /*us*/) = 0;

	// 开始
	virtual bool Start(
		FormatType eAFormatType,
		FormatType eVFormatType,
		const std::string strAbFilePath /*absolute file path*/) = 0;

	// 停止
	virtual void Stop() = 0;

	// 暂停录制
	virtual bool Pause() = 0;

	// 继续录制
	virtual bool Resume() = 0;

	// 设置分割时长，单位秒
	void SetSplitDuration(int nSecond);

	// 设置分割大小，单位MB
	void SetSplitSize(int nMB);

protected:
	std::string		m_strAbFilePath;		//录制文件绝对路径
	int				m_nSplitType;			//分割方式，0：按文件时长；1：按文件大小
	int				m_nSplitDuration;		//分割时长
	int				m_nSplitSize;			//分割大小
	Record_State	m_RecordState;			//录制状态
	int				m_nRecordedDuration;	//已录制时长
	int				m_nRecordedSize;		//已录制大小
	int				m_nPausedTime;			//暂停总时长
};

bool InitEnv();
#endif