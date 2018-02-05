
// PullStreamModuleDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PullStreamModulePro.h"
#include "PullStreamModuleDlg.h"
#include "afxdialogex.h"

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
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPullStreamModuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
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
	FFmpegInit::GetInstance()->Initialize();
	m_bStartRecord = false;
	((CButton *)GetDlgItem(IDC_RADIO_TIME))->SetCheck(TRUE);
	SetDlgItemInt(IDC_EDIT_TIME, 60);
	SetDlgItemInt(IDC_EDIT_SIZE, 2);
	
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
	uint64_t iPts /*ms*/, 
	uint64_t iDts)
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
				  uint64_t iPts /*ms*/,
				  uint64_t iDts)
{
	if (m_bStartRecord)
	{
		//TRACE(L"[CPullStreamModuleDlg::OnDataHandle]eFormatType : %d, pData : %0x, iLen = %d, iPts : %lld, iDts : %lld.\n", 
		//	  eFormatType, (void*)pData, iLen, iPts, iDts);
		m_TSRecord.ImportAVPacket(eMediaType, eFormatType, pData, iLen, iPts * 1000, iDts * 1000);
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
	m_rmtpModule.OpenStream(strUrl, true, true);
}


void CPullStreamModuleDlg::OnBnClickedBtnCloseStream2()
{
	m_rmtpModule.CloseStream();
}


void CPullStreamModuleDlg::OnClose()
{
	m_rmtpModule.CloseStream();
	FFmpegInit::GetInstance()->Uninitialize();

	CDialogEx::OnClose();
}


void CPullStreamModuleDlg::OnBnClickedBtnStartRecord()
{
	if (m_TSRecord.Start(aaclc, h264, "./RecordFile/Record.ts"))
	{
		if (BST_CHECKED == ((CButton *)GetDlgItem(IDC_RADIO_TIME))->GetCheck())
		{
			int nTime = GetDlgItemInt(IDC_EDIT_TIME);
			m_TSRecord.SetSplitDuration(nTime);
		}
		else if(BST_CHECKED == ((CButton *)GetDlgItem(IDC_RADIO_SIZE))->GetCheck())
		{ 
			int nSize = GetDlgItemInt(IDC_EDIT_SIZE);
			m_TSRecord.SetSplitSize(nSize);
		}
		m_bStartRecord = true;
	}
}


void CPullStreamModuleDlg::OnBnClickedBtnStopRecord()
{
	if (m_bStartRecord)
	{
		m_bStartRecord = false;
		m_TSRecord.Stop();
	}
}

Poco::Clock g_clock;
void CPullStreamModuleDlg::OnBnClickedBtnPauseRecord()
{
	if (m_bStartRecord)
	{
		m_TSRecord.Pause();
		g_clock.update();
	}
}


void CPullStreamModuleDlg::OnBnClickedBtnResumeRecord3()
{
	if (m_bStartRecord)
	{
		m_TSRecord.Resume();
		
		Poco::Clock clk = g_clock;
		g_clock.update();
		int64_t iDiffer = g_clock - clk;
		clk.update();
		int nTest = iDiffer / 1000 / 1000;
		TRACE(L"pause %d sec.\n", nTest);
	}
}
