#include "RecordBase.h"


RecordBase::RecordBase()
: m_strAbFilePath("")
, m_nSplitType(0)
, m_nSplitDuration(60 * 1 * 1000)
, m_nSplitSize(50 * 2 * 1024)
, m_RecordState(RS_Stop)
, m_nRecordedDuration(0)
, m_nRecordedSize(0)
, m_nPausedTime(0)
{}

RecordBase::~RecordBase()
{}

bool g_bGlobalInitSuccess = InitEnv();

bool InitEnv()
{
	static bool bInit = false;
	if (!bInit)
	{
		bInit = true;
		av_register_all();
	}
	return bInit;
}

void RecordBase::SetSplitDuration(int nSecond)
{
	m_nSplitType = 0;
	m_nSplitDuration = nSecond * 1000;
}

// 设置分割大小，单位MB
void RecordBase::SetSplitSize(int nMB)
{
	m_nSplitType = 1;
	m_nSplitSize = nMB * 1024 * 1024;
}
