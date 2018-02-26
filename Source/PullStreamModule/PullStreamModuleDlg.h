
// PullStreamModuleDlg.h : 头文件
//

#pragma once

#include "PullStreamModule.h"
#include "../TSRecord/TSRecord.h"
#include "FlvRecord.h"
#include "Mp3Record.h"
#include "afxwin.h"

// CPullStreamModuleDlg 对话框
class CPullStreamModuleDlg : public CDialogEx
{
// 构造
public:
	CPullStreamModuleDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_PULLSTREAMMODULE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
		int64_t iPts /*ms*/,
		int64_t iDts);

	void OnDataHandle(MediaType eMediaType,
				FormatType eFormatType,
				uint8_t *pData,
				uint32_t iLen,
				int64_t iPts /*ms*/,
				int64_t iDts);

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
