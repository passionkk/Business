
// PullStreamModuleDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PullStreamModulePro.h"
#include "PullStreamModuleDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CPullStreamModuleDlg �Ի���



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


// CPullStreamModuleDlg ��Ϣ�������

BOOL CPullStreamModuleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������
	FFmpegInit::GetInstance()->Initialize();
	m_bStartRecord = false;
	((CButton *)GetDlgItem(IDC_RADIO_TIME))->SetCheck(TRUE);
	SetDlgItemInt(IDC_EDIT_TIME, 60);
	SetDlgItemInt(IDC_EDIT_SIZE, 2);
	
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CPullStreamModuleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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
	//strUrl = "D:\\�����ز�\\Media\\mp4\\H264-1VA0-FRAME.mp4";
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
