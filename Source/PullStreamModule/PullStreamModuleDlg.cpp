
// PullStreamModuleDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PullStreamModulePro.h"
#include "PullStreamModuleDlg.h"
#include "afxdialogex.h"
#include "poco/Path.h"
#include "poco/File.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CPullStreamModuleDlg 对话框

CPullStreamModuleDlg::CPullStreamModuleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPullStreamModuleDlg::IDD, pParent)
	, m_radioRecFileType(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pRecord = NULL;
}

void CPullStreamModuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_URL, m_cmbUrl);
	DDX_Control(pDX, IDC_COMBO_AUDIOTYPE, m_cmbAudioType);
	DDX_Control(pDX, IDC_COMBO_VIDEOTYPE, m_cmbVideoType);
	DDX_Radio(pDX, IDC_RADIO_TS, m_radioRecFileType);
}

BEGIN_MESSAGE_MAP(CPullStreamModuleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BTN_OPEN_STREAM, &CPullStreamModuleDlg::OnBnClickedBtnOpenStream)
	ON_BN_CLICKED(IDC_BTN_CLOSE_STREAM2, &CPullStreamModuleDlg::OnBnClickedBtnCloseStream2)
	ON_BN_CLICKED(IDC_BTN_START_RECORD, &CPullStreamModuleDlg::OnBnClickedBtnStartRecord)
	ON_BN_CLICKED(IDC_BTN_STOP_RECORD, &CPullStreamModuleDlg::OnBnClickedBtnStopRecord)
	ON_BN_CLICKED(IDC_BTN_PAUSE_RECORD, &CPullStreamModuleDlg::OnBnClickedBtnPauseRecord)
	ON_BN_CLICKED(IDC_BTN_RESUME_RECORD3, &CPullStreamModuleDlg::OnBnClickedBtnResumeRecord3)
	ON_BN_CLICKED(IDC_RADIO_TS, &CPullStreamModuleDlg::OnBnClickedRadioRecordType)
	ON_BN_CLICKED(IDC_RADIO_FLV, &CPullStreamModuleDlg::OnBnClickedRadioRecordType)
	ON_BN_CLICKED(IDC_RADIO_MP3, &CPullStreamModuleDlg::OnBnClickedRadioRecordType)
END_MESSAGE_MAP()


// CPullStreamModuleDlg 消息处理程序

BOOL CPullStreamModuleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
	int64_t iDuration = 100000;
	unsigned char chDura[8] = { 0 };
	for (auto i = sizeof(chDura) - 1; i > 0; --i)
	{
		chDura[i] = (unsigned char)(iDuration >> (i * 8));
	}

	for (auto i = 0; i < sizeof(chDura); ++i)
	{
		if (i < 4)
			chDura[i] = (unsigned char)(iDuration >> ((sizeof(chDura) - i) * 8));
		else
			chDura[i] = (unsigned char)((iDuration & 0xffffffff) >> ((sizeof(chDura) - i) * 8));
	}

	uint32_t i1 = (uint32_t)(iDuration >> 32);
	uint32_t i2 = (uint32_t)(iDuration & 0xffffffff);
	chDura[0] = (uint8_t)(i1 >> 24);
	chDura[1] = (uint8_t)(i1 >> 16);
	chDura[2] = (uint8_t)(i1 >> 8);
	chDura[3] = (uint8_t)i1;

	chDura[4] = (uint8_t)(i2 >> 24);
	chDura[5] = (uint8_t)(i2 >> 16);
	chDura[6] = (uint8_t)(i2 >> 8);
	chDura[7] = (uint8_t)i2;

	InitCtrl();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPullStreamModuleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPullStreamModuleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPullStreamModuleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPullStreamModuleDlg::DataHandle(
	void *pUserData,
	MediaType eMediaType,
	FormatType eFormatType,
	uint8_t *pData,
	uint32_t iLen,
	int64_t iPts /*ms*/, 
	int64_t iDts)
{
	if (pUserData != NULL)
	{
		CPullStreamModuleDlg* pThis = (CPullStreamModuleDlg*)pUserData;
		pThis->OnDataHandle(eMediaType, eFormatType, pData, iLen, iPts, iDts);
	}
}

void CPullStreamModuleDlg::OnDataHandle(MediaType eMediaType,
				  FormatType eFormatType,
				  uint8_t *pData,
				  uint32_t iLen,
				  int64_t iPts /*ms*/,
				  int64_t iDts)
{
	if (m_bStartRecord)
	{
		//TRACE(L"[CPullStreamModuleDlg::OnDataHandle]eFormatType : %d, pData : %0x, iLen = %d, iPts : %lld, iDts : %lld.\n", 
		//	  eFormatType, (void*)pData, iLen, iPts, iDts);
		if (m_pRecord)
			m_pRecord->ImportAVPacket(eMediaType, eFormatType, pData, iLen, iPts * 1000, iDts * 1000);
		//m_TSRecord.ImportAVPacket(eMediaType, eFormatType, pData, iLen, iPts * 1000, iDts * 1000);
	}
}

void CPullStreamModuleDlg::OnBnClickedBtnOpenStream()
{
	std::string strUrl = "rtmp://live.hkstv.hk.lxdns.com/live/hks";//;
	//strUrl = "https://www.douyu.com/room/share/2151987";
	//strUrl = "D:\\测试素材\\Media\\mp4\\H264-1VA0-FRAME.mp4";
	//strUrl = "http://tvbilive6-i.akamaihd.net/hls/live/494646/CKF4/CKF4-03.m3u8";
	//strUrl = "http://live.hkstv.hk.lxdns.com/live/hks/playlist.m3u8";
	//strUrl = "sintel.ts";
	//strUrl = "Test.flv";
	//strUrl = "rtmp://192.168.253.201/live/1";
	//strUrl = "rtmp://v1.one-tv.com/live/mpegts.stream";
	m_rmtpModule.Subscribe(DataHandle, (void*)this);

	CString strText;
	int nLen = m_cmbUrl.GetLBTextLen(m_cmbUrl.GetCurSel());
	m_cmbUrl.GetLBText(m_cmbUrl.GetCurSel(), strText.GetBuffer(nLen));
	strText.ReleaseBuffer();

	std::string sUrl = CT2A(strText);

	bool bAutoReconnect = (BST_CHECKED == ((CButton *)GetDlgItem(IDC_CHECK_AUTORECONNECT))->GetCheck());
	if (sUrl.find("H264-1VA0-FRAME.mp4") != std::string::npos)
	{
		m_rmtpModule.Seek(10 * 1000);
	}
	if (m_rmtpModule.OpenStream(sUrl, true, true, bAutoReconnect))
		TRACE(L"[拉流成功].\n");
	else
		TRACE(L"[拉流失败].\n");
}


void CPullStreamModuleDlg::OnBnClickedBtnCloseStream2()
{
	m_rmtpModule.CloseStream();
}


void CPullStreamModuleDlg::OnClose()
{
	if (m_pRecord)
	{
		m_pRecord->Stop();
		delete m_pRecord;
		m_pRecord = nullptr;
	}
	m_rmtpModule.UnSubscribe(DataHandle);
	m_rmtpModule.CloseStream();
	FFmpegInit::GetInstance()->Uninitialize();

	CDialogEx::OnClose();
}


void CPullStreamModuleDlg::OnBnClickedBtnStartRecord()
{
	FormatType eAudio, eVideo;
	std::string strRecordPath = "";

	switch (m_cmbAudioType.GetCurSel())
	{
	case 0:
		eAudio = aaclc;
		break;
	case 1:
		eAudio = mp3;
		break;
	case 2:
		eAudio = opus;
		break;
	case 3:
		eAudio = g711a;
		break;
	case 4:
		eAudio = g711u;
		break;
	case 5:
		eAudio = none;
		break;
	default:
		eAudio = none;
		break;
	}

	switch (m_cmbVideoType.GetCurSel())
	{
	case 0:
		eVideo = h264;
		break;
	case 1:
		eVideo = h265;
		break;
	case 2:
		eVideo = png;
		break;
	default:
		eVideo = none;
	}

	if (m_pRecord)
	{
		m_pRecord->Stop();
		delete m_pRecord;
		m_pRecord = NULL;
	}

	UpdateData(TRUE);
	Poco::File recordDir("./RecordFile");
	if (!recordDir.exists())
	{
		if (!recordDir.createDirectory())
			AfxMessageBox(L"创建录制目录RecordFile失败");
	}

	switch (m_radioRecFileType)
	{
	case 0:
		m_pRecord = new TSRecord;
		strRecordPath = "./RecordFile/Record.ts";
		break;
	case 1:
		m_pRecord = new FlvRecord;
		strRecordPath = "./RecordFile/Record.flv";
		break;
	case 2:
		m_pRecord = new Mp3Record;
		strRecordPath = "./RecordFile/Record.mp3";
		break;
	default:
		m_pRecord = new TSRecord;
		break;
	}
	
	//if (m_TSRecord.Start(eAudio, eVideo, "./RecordFile/Record.ts"))
	if (m_pRecord->Start(eAudio, eVideo, strRecordPath))
	{
		if (BST_CHECKED == ((CButton *)GetDlgItem(IDC_RADIO_TIME))->GetCheck())
		{
			int nTime = GetDlgItemInt(IDC_EDIT_TIME);
			m_pRecord->SetSplitDuration(nTime);
		}
		else if(BST_CHECKED == ((CButton *)GetDlgItem(IDC_RADIO_SIZE))->GetCheck())
		{ 
			int nSize = GetDlgItemInt(IDC_EDIT_SIZE);
			m_pRecord->SetSplitSize(nSize);
		}
		m_bStartRecord = true;
	}
}


void CPullStreamModuleDlg::OnBnClickedBtnStopRecord()
{
	if (m_bStartRecord)
	{
		m_bStartRecord = false;
		if (m_pRecord)
			m_pRecord->Stop();
	}
}

Poco::Clock g_clock;
void CPullStreamModuleDlg::OnBnClickedBtnPauseRecord()
{
	if (m_bStartRecord)
	{
		if (m_pRecord)
			m_pRecord->Pause();
		g_clock.update();
	}
}


void CPullStreamModuleDlg::OnBnClickedBtnResumeRecord3()
{
	if (m_bStartRecord)
	{
		if (m_pRecord)
			m_pRecord->Resume();
		
		Poco::Clock clk = g_clock;
		g_clock.update();
		int64_t iDiffer = g_clock - clk;
		clk.update();
		int nTest = int(iDiffer / 1000 / 1000);
		TRACE(L"pause %d sec.\n", nTest);
	}
}

void CPullStreamModuleDlg::InitCtrl()
{
	FFmpegInit::GetInstance()->Initialize();
	m_bStartRecord = false;
	((CButton *)GetDlgItem(IDC_RADIO_TIME))->SetCheck(TRUE);
	SetDlgItemInt(IDC_EDIT_TIME, 60);
	SetDlgItemInt(IDC_EDIT_SIZE, 2);
	
	int nAIndex = m_cmbAudioType.GetCount();
	m_cmbAudioType.InsertString(nAIndex++, L"aaclc");
	m_cmbAudioType.InsertString(nAIndex++, L"mp3");
	m_cmbAudioType.InsertString(nAIndex++, L"opus");
	m_cmbAudioType.InsertString(nAIndex++, L"711a");
	m_cmbAudioType.InsertString(nAIndex++, L"711u");
	m_cmbAudioType.InsertString(nAIndex++, L"none");
	m_cmbAudioType.SetCurSel(0);

	int nVIndex = m_cmbVideoType.GetCount();
	m_cmbVideoType.InsertString(nVIndex++, L"h264");
	m_cmbVideoType.InsertString(nVIndex++, L"h265");
	m_cmbVideoType.InsertString(nVIndex++, L"png");
	m_cmbVideoType.InsertString(nVIndex++, L"none");
	m_cmbVideoType.SetCurSel(0);

	((CButton *)GetDlgItem(IDC_RADIO_TS))->SetCheck(TRUE);
	
	int nIndex = m_cmbUrl.GetCount();

	CString strUrl = _T("rtmp://live.hkstv.hk.lxdns.com/live/hks");
	m_cmbUrl.InsertString(nIndex++, strUrl);
	
	strUrl = _T("http://live.hkstv.hk.lxdns.com/live/hks/playlist.m3u8");
	m_cmbUrl.InsertString(nIndex++, strUrl);

	strUrl = _T("http://tvbilive6-i.akamaihd.net/hls/live/494646/CKF4/CKF4-03.m3u8");
	m_cmbUrl.InsertString(nIndex++, strUrl);

	strUrl = _T("D:\\测试素材\\Media\\mp4\\H264-1VA0-FRAME.mp4");
	m_cmbUrl.InsertString(nIndex++, strUrl);

	strUrl = _T("./4 Non Blondes - What's Up.mp3");
	m_cmbUrl.InsertString(nIndex++, strUrl);

	strUrl = _T("rtmp://v1.one-tv.com/live/mpegts.stream");
	m_cmbUrl.InsertString(nIndex++, strUrl);
	
	m_cmbUrl.SetCurSel(0);
}

void CPullStreamModuleDlg::OnBnClickedRadioRecordType()
{
	UpdateData(TRUE);
	switch (m_radioRecFileType)
	{
	case 0:
		TRACE(L"Record TS.\n");
		break;
	case 1:
		TRACE(L"Record Flv.\n");
		break;
	case 2:
		TRACE(L"Record MP3.\n");
		break;
	default:
		break;
	}
}

