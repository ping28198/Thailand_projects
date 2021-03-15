
// THP_OCRDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "THP_OCR.h"
#include "THP_OCRDlg.h"
#include "afxdialogex.h"
#include "CommonFunc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CALLBACK TimeFunctionMsg(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{

	CTHPOCRDlg * pView = (CTHPOCRDlg *)lpParam;
	pView->update_state();

}


#define IADM_TIMER_INTERVAL	50 //多媒体中断间隔
#define IADM_TIMES_COMM_RETRY 3000
#define IADM_TIMER_RESOLUTION 5  // 多媒体时钟精度（单位：毫秒）



// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTHPOCRDlg 对话框



CTHPOCRDlg::CTHPOCRDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_THP_OCR_DIALOG, pParent)
	, ratio_mode(0)
	, m_MMT1(50)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CTHPOCRDlg::~CTHPOCRDlg()
{
	
}

void CTHPOCRDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ST_OCRID, st_ocrid_ctr);
	DDX_Control(pDX, IDC_ST_IP, st_ip_ctr);
	DDX_Control(pDX, IDC_ST_CON_STATE, st_connect_state);
	DDX_Control(pDX, IDC_EDIT_MSG_BOX, m_edit_msg_box);
	DDX_Radio(pDX, IDC_MODE_NORMAL, ratio_mode);
	DDX_Control(pDX, IDC_ST_TOTAL_COUNT, st_total_num_ctr);
	DDX_Control(pDX, IDC_ST_OK_COUNT, st_ok_num_ctr);
	DDX_Control(pDX, IDC_ST_OC_PERCENT, st_percent_ctr);
}


BEGIN_MESSAGE_MAP(CTHPOCRDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CTHPOCRDlg::OnBnClickedButton1)
	
	ON_BN_CLICKED(IDC_MODE_NORMAL, &CTHPOCRDlg::OnBnClickedModeNormal)
	ON_BN_CLICKED(IDC_MODE_TEST, &CTHPOCRDlg::OnBnClickedModeTest)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON2, &CTHPOCRDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CTHPOCRDlg 消息处理程序

void CTHPOCRDlg::on_connect_state(int state /*= 0*/)
{
	if (state)
	{
		st_connect_state.SetBackColor(RGB(0, 255, 0));
		st_connect_state.SetWindowTextW(CString(L"on"));
	}
	else
	{
		st_connect_state.SetBackColor(RGB(255, 0, 0));
		st_connect_state.SetWindowTextW(CString(L"off"));
	}
}

void CTHPOCRDlg::update_statistic_info(size_t total_num, size_t ok_num)
{
	if (total_num == m_total_num && ok_num == m_ok_num)
	{
		return;
	}
	m_total_num = total_num;
	m_ok_num = ok_num;
	if (total_num == 0)
	{
		m_ok_percent = 0.0;
	}
	else
	{
		m_ok_percent = 100.0f * float(ok_num) / total_num;
	}
	

	s_tmp.Format(L"%d", m_total_num);
	st_total_num_ctr.SetWindowTextW(s_tmp);

	s_tmp.Format(L"%d", ok_num);
	st_ok_num_ctr.SetWindowTextW(s_tmp);

	s_tmp.Format(L"%.2f", m_ok_percent);
	s_tmp += L"%";
	st_percent_ctr.SetWindowTextW(s_tmp);
}

void CTHPOCRDlg::update_state()
{
	if (m_connect_state!=pMainWorker->pClient->m_is_connected)
	{
		m_connect_state = pMainWorker->pClient->m_is_connected;
		on_connect_state(m_connect_state);
	}
	update_statistic_info(pMainWorker->get_total_task_num(), pMainWorker->get_ocr_ok_num());
	std::string postcode;
	int res = pMainWorker->get_last_postcode(postcode);
	if (res)
	{
		ouput_msg(postcode);
	}

}

void CTHPOCRDlg::ouput_msg(const std::string &msg)
{
	if (m_edit_msg_box.GetLineCount() > 1000)
	{
		m_edit_msg_box.SetWindowText(_T(""));
	}

	CTime m_time;
	m_time = CTime::GetCurrentTime();             //获取当前时间日期          //格式化时间  
	s_tmp = m_time.Format(_T("[%H:%M:%S] "));
	s_tmp = s_tmp + CommonFunc::MCharToWChar(msg.c_str()) + L"\r\n";
	m_edit_msg_box.ReplaceSel(s_tmp);

}

BOOL CTHPOCRDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// TODO: 在此添加额外的初始化代码


	CString title = L"THP OCR";
	this->SetWindowText(title);





	on_connect_state(0);
	m_edit_msg_box.SetLimitText(-1);
	pMainWorker = new OCR_MainWoker();
	pMainWorker->initial();

	CString ipwstr = CommonFunc::MCharToWChar(pMainWorker->m_param.image_server_ip.c_str());

	s_tmp.Format(L"%s:%d", ipwstr, pMainWorker->m_param.image_server_port);
	st_ip_ctr.SetWindowTextW(s_tmp);


	CString ocrid_wstr;
	ocrid_wstr.Format(L"%d", pMainWorker->m_param.ocr_id);
	st_ocrid_ctr.SetWindowTextW(ocrid_wstr);

	if (pMainWorker->m_param.is_test_mode)
	{
		((CButton *)GetDlgItem(IDC_MODE_TEST))->SetCheck(TRUE);
	}
	else
	{
		((CButton *)GetDlgItem(IDC_MODE_NORMAL))->SetCheck(TRUE);
	}


	m_MMT1.AddTimerListener((LPTIMECALLBACK)TimeFunctionMsg);
	m_MMT1.Start((DWORD_PTR)this);




	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTHPOCRDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CTHPOCRDlg::OnPaint()
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
HCURSOR CTHPOCRDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




void CTHPOCRDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
// 	on_connect_state(1);
// 	ouput_msg("asdf");
// 	update_statistic_info(2, 1);
	pMainWorker->reset_count();
	update_state();

}




void CTHPOCRDlg::OnBnClickedModeNormal()
{


	pMainWorker->set_test_mode(0);
	//MessageBox(L"sadf");
	// TODO: 在此添加控件通知处理程序代码
}


void CTHPOCRDlg::OnBnClickedModeTest()
{
	pMainWorker->set_test_mode(1);
	// TODO: 在此添加控件通知处理程序代码
}



void CTHPOCRDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	int i = MessageBox(_T("Are you sure to Exit!"), _T("Warning"), MB_YESNO | MB_ICONQUESTION);
	if (i != IDYES)
	{
		return;
	}
	if (pMainWorker) delete pMainWorker;

	CDialogEx::OnClose();
}


void CTHPOCRDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strFile = _T("");

	CFileDialog    dlgFile(TRUE, NULL, NULL, OFN_HIDEREADONLY, _T("Describe Files (*.jpg)|*.jpg|All Files (*.*)|*.*||"), NULL);

	if (dlgFile.DoModal())
	{
		strFile = dlgFile.GetPathName();
		char image_path[512] = {0};
		CommonFunc::WCharToMChar(strFile.GetBuffer(0), image_path);
		std::string ss(image_path);
		pMainWorker->push_debug_image(ss);

	}

}
