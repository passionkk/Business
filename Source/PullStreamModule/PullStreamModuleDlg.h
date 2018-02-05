
// PullStreamModuleDlg.h : ͷ�ļ�
//

#pragma once

#include "PullStreamModule.h"
#include "../TSRecord/TSRecord.h"

// CPullStreamModuleDlg �Ի���
class CPullStreamModuleDlg : public CDialogEx
{
// ����
public:
	CPullStreamModuleDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_PULLSTREAMMODULE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
	
// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedBtnOpenStream();
	afx_msg void OnBnClickedBtnCloseStream2();
	afx_msg void OnClose();
	afx_msg void OnBnClickedBtnStartRecord();
	afx_msg void OnBnClickedBtnStopRecord();
	DECLARE_MESSAGE_MAP()

public:
	static void DataHandle(
		void *pUserData,
		MediaType eMediaType,
		FormatType eFormatType,
		uint8_t *pData,
		uint32_t iLen,
		uint64_t iPts /*ms*/,
		uint64_t iDts);

	void OnDataHandle(MediaType eMediaType,
				FormatType eFormatType,
				uint8_t *pData,
				uint32_t iLen,
				uint64_t iPts /*ms*/,
				uint64_t iDts);

public:
	PullStreamModule	m_rmtpModule;
	TSRecord			m_TSRecord;
	bool				m_bStartRecord;
	afx_msg void OnBnClickedBtnPauseRecord();
	afx_msg void OnBnClickedBtnResumeRecord3();
};
