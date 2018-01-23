#include "RecordBase.h"


RecordBase::RecordBase()
: m_strAbFilePath("")
, m_nSplitType(0)
, m_nSplitDuration(60 * 5)
, m_nSplitSize(50 * 2)
, m_RecordState(RS_Stop)
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


