
// PullStreamModuleDlg.h : ͷ�ļ�
//

#pragma once

#include "PullStreamModule.h"
#include "../TSRecord/TSRecord.h"
#include "FlvRecord.h"
#include "afxwin.h"

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
	afx_msg void OnBnClickedBtnPauseRecord();
	afx_msg void OnBnClickedBtnResumeRecord3();
	afx_msg void OnBnClickedRadioRecordType();

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

	void InitCtrl();
public:
	PullStreamModule	m_rmtpModule;
	TSRecord			m_TSRecord;
	RecordBase*			m_pRecord;
	bool				m_bStartRecord;
	CComboBox m_cmbUrl;
	CComboBox m_cmbAudioType;
	CComboBox m_cmbVideoType;
	int m_radioRecFileType;
};
