
// TSRecord.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CTSRecordApp: 
// �йش����ʵ�֣������ TSRecord.cpp
//

class CTSRecordApp : public CWinApp
{
public:
	CTSRecordApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CTSRecordApp theApp;