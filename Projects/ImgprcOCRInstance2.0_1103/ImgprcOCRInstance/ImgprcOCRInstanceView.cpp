// ImgprcOCRInstanceView.cpp : CImgprcOCRInstanceView 类的实现
//

#include "stdafx.h"
#include "ImgprcOCRInstance.h"

#include "ImgprcOCRInstanceDoc.h"
#include "ImgprcOCRInstanceView.h"
#include "HWDigitsRecogDll.h"
#include "MPFCommuication.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// int nRes;//判定结果，0正常邮件，1不确定，2可疑邮件
// int nType;//对于正常邮件和不确定邮件=0，对于可疑邮件，表示可疑类型

//返回值=0正常返回，<0异常返回
//extern "C" __declspec(dllexport) int ImageFilte( const char *input_img_file,const char *output_img_file, int& nRes,int& nType,int iRank);

void CALLBACK TimeFunctionMsg(PVOID lpParam, BOOLEAN TimerOrWaitFired);
void CALLBACK TimeFunctionImg(PVOID lpParam, BOOLEAN TimerOrWaitFired);


#define IADM_TIMER_INTERVAL	50 //多媒体中断间隔
#define IADM_TIMES_COMM_RETRY 3000
#define IADM_TIMER_RESOLUTION 5  // 多媒体时钟精度（单位：毫秒）


#define OCR_VIEW_DEBUG

#ifndef OCR_VIEW_DEBUG
Logger logger("./log", LogLevelMid);
#else
Logger logger("./log", LogLevelAll);
#endif // !1










// CImgprcOCRInstanceView

IMPLEMENT_DYNCREATE(CImgprcOCRInstanceView, CFormView)

BEGIN_MESSAGE_MAP(CImgprcOCRInstanceView, CFormView)
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_COMMAND(ID_TEST_SENDREQ, &CImgprcOCRInstanceView::OnTestSendreq)
	ON_COMMAND(ID_DEBUG_BT, &CImgprcOCRInstanceView::OnDebugeBt)
END_MESSAGE_MAP()

// CImgprcOCRInstanceView 构造/析构

CRITICAL_SECTION crt_section;

// 工作线程用于处理图像
extern UINT ImageProcessThread(LPVOID pParam)
{
	ImgprcTask mTask;
	CImgprcOCRInstanceView *pParent = (CImgprcOCRInstanceView*)pParam;
	OcrAlgorithm_config mconfig = pParent->m_ocrConifg;


	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0; // 删除文件名，只获得路径字串
	CString str_url = szFilePath;  // 例如str_url==e:\program\Debug\  //
	USES_CONVERSION;
	std::string exe_dir = T2A(str_url.GetBuffer());

	//mconfig.ORB_template_img1_path = exe_dir + mconfig.ORB_template_img1_path;
	//mconfig.ORB_template_img2_path = exe_dir + mconfig.ORB_template_img2_path;
	//mconfig.handwrite_ref_img1_path = exe_dir + mconfig.handwrite_ref_img1_path;//

	//加载tag检测功能
	tag_detector detecor;
	std::string model_path = mconfig.detect_model_file_path;
	float confidence_threshold = mconfig.TagDetectConfidence;
	int max_instance_per_class = mconfig.max_instance_per_class;
	int res = detecor.initial(model_path, confidence_threshold, max_instance_per_class);
	if (res==0)
	{
		logger.TraceError("目标检测初始化失败");
	}
	mconfig.pTagDetector = &detecor;


	//加载匹配数据
	res = mconfig.match_data.getMatchDataFromImg(mconfig.ORB_template_img1_path, mconfig.ORB_template_img2_path);
	if (res == 0)
	{
		logger.TraceError("加载匹配数据失败");
	}
	res = mconfig.match_data.getMatchDataFromImg_handwrite_addr(mconfig.handwrite_ref_img1_path);
	if (res == 0)
	{
		logger.TraceError("加载手写框匹配数据失败");
	}

	//加载ocr功能
	tesseract::TessBaseAPI mTess;
	std::string tessDataDir = exe_dir + "tessdata";
	if (mTess.Init(tessDataDir.c_str(), "eng"))
	{
		logger.TraceError("未加载到OCR引擎文件！");
		return 0;
		//MessageBoxW(L"错误，未加载到OCR引擎文件！");
	}
	mconfig.pTess = &mTess;


	//加载手写数字识别引擎
	HWDigitsRecog hwDigitRec;
	std::string handwrite_model_path =mconfig.handwrite_ocr_model_path;
	res = hwDigitRec.initial(handwrite_model_path);
	if (res == 0)
	{
		logger.TraceError("加载手写数字识别模型数据失败");
	}
	mconfig.pHWDigitsRecog = &hwDigitRec;

	


	



	logger.TraceKeyInfo("线程初始化完毕！");
	//循环检测
	while (true)
	{

		pParent->ImagesProcessing(&mconfig);

	}
	return 0;
}


CImgprcOCRInstanceView::CImgprcOCRInstanceView()
	: CFormView(CImgprcOCRInstanceView::IDD),
	m_MMT1(20), 
	m_MMT2(50)
{
	// TODO: 在此处添加构造代码
	m_bClientSockState = 0;				//连接状态
	m_dwTimeCount=0;
	m_dwTcp2LastSendToNow=0;
	m_dwTimesTcp2Retry1=0;
	m_bShowMain=true;
	m_bClientSockState = 0;	
	m_iRank =1;

}

CImgprcOCRInstanceView::~CImgprcOCRInstanceView()
{


}

void CImgprcOCRInstanceView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_TOTAL, m_crlTotal);
	//DDX_Control(pDX, IDC_EDIT_READIMG, m_crlReadImg);
	//DDX_Control(pDX, IDC_EDIT_PRCIMG, m_crlProcessImg);
	//DDX_Control(pDX, IDC_EDIT_TAG_OK, m_crlTagOCROK);
	DDX_Control(pDX, IDC_EDIT_OBR, m_crlObrOK);
	DDX_Control(pDX, IDC_EDIT_SOCKET, m_crlSocket);
	DDX_Control(pDX, IDC_EDIT_DB, m_crlDb);
	DDX_Control(pDX, IDC_MAIN, m_crlMain);
	DDX_Control(pDX, IDC_MSG, m_crlMsg);
	DDX_Control(pDX, IDC_EDIT_COPYIMG, m_crlCopyImg);
	DDX_Control(pDX, IDC_EDIT_REST_TASK, m_crlRestTask);
	//DDX_Text(pDX, IDC_EDIT_REST_TASK, m_valueRestTask);
	DDX_Control(pDX, IDC_EDIT_TAG_OK, m_crlTagOCROK);
}

BOOL CImgprcOCRInstanceView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CFormView::PreCreateWindow(cs);
}

void CImgprcOCRInstanceView::OnInitialUpdate()
{

//	ReadINI();  //tx20171004
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();
	m_ctime = CTime::GetCurrentTime();

	char fn[IS_IMAGE_MAX_PATH];
	char chsTemp[IS_IMAGE_MAX_PATH];
	fn[0] = 0;
	chsTemp[0] = 0;
	CString dir;
	CString item;

	//for(int i=0; i<m_iMaxImageNum;i++)
	//{
	//	if(!m_cstrImgDir.IsEmpty())
	//	{
	//		dir =m_cstrImgDir;
	//		dir.Delete(m_cstrImgDir.GetLength()-1);
	//		item.Format(_T("%d\\"),i+1);
	//		dir = dir + item;
	//		CreateDirectoryW(dir,NULL);
	//	}

	//	if(!m_cstrProcessedDir.IsEmpty())
	//	{
	//		dir =m_cstrProcessedDir;
	//		dir.Delete(m_cstrProcessedDir.GetLength()-1);
	//		item.Format(_T("%d\\"),i+1);
	//		dir = dir + item;
	//		CreateDirectoryW(dir,NULL);
	//	}
	//}

	//创建log（即将弃用）
	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0; // 删除文件名，只获得路径字串
	CString str_url = szFilePath;  // 例如str_url==e:\program\Debug\  //
	CStringToCharArray(str_url + m_cstrLogDir, chsTemp);
	sprintf_s(fn,"%sOCR%dLog%04d%02d%02d.txt",chsTemp,m_iLocPort,m_ctime.GetYear(),m_ctime.GetMonth(),m_ctime.GetDay());
	m_pFileLog=fopen(fn,"a+");



	unsigned int nchLen;

	nchLen = WideCharToMultiByte(CP_ACP,0,m_cstrSeverIP,m_cstrSeverIP.GetLength(),NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,m_cstrSeverIP,m_cstrSeverIP.GetLength(),m_MachineComm.m_chsIPAddress,nchLen,NULL,NULL);
	m_MachineComm.m_chsIPAddress[nchLen]=NULL;
	m_MachineComm.m_chsDstPort = m_cstrSeverPort;

//	strcpy_s(m_MachineComm.m_chsIPAddress,m_cstrIADMIP.GetBuffer(0)); //tX
	strcpy_s(m_MachineComm.m_chsPCName,"IADM");
	strcpy_s(m_MachineComm.m_chsShortName,"IADM");
	m_MachineComm.ClearMachineBuf();


	//配置ocrConfig
	//TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0; // 删除文件名，只获得路径字串
	str_url = szFilePath;  // 例如str_url==e:\program\Debug\  //
	USES_CONVERSION;
	std::string exe_dir = T2A(str_url.GetBuffer());

	m_ocrConifg.ORB_template_img1_path = exe_dir + m_ocrConifg.ORB_template_img1_path;
	m_ocrConifg.ORB_template_img2_path = exe_dir + m_ocrConifg.ORB_template_img2_path;
	m_ocrConifg.handwrite_ref_img1_path = exe_dir + m_ocrConifg.handwrite_ref_img1_path;//


	logger.TraceKeyInfo(("tag detect confidence threshold:" + std::to_string(m_ocrConifg.TagDetectConfidence)).c_str());
	logger.TraceKeyInfo(("max homo class tag in an image:" + std::to_string(m_ocrConifg.max_instance_per_class)).c_str());
	//logger.TraceKeyInfo(("OCR模型路径：" + tessDataDir).c_str());
	logger.TraceKeyInfo(("tag detect model path:" + m_ocrConifg.detect_model_file_path).c_str());
	logger.TraceKeyInfo(("handwrite digits detect model path:" + m_ocrConifg.handwrite_ocr_model_path).c_str());
	logger.TraceKeyInfo(("handwrite address range reference image path:" + m_ocrConifg.handwrite_ref_img1_path).c_str());



	logger.TraceKeyInfo("starting threading 1...");
	CWinThread *pTheard_process1;
	pTheard_process1 = AfxBeginThread(ImageProcessThread, this);
	if (pTheard_process1 == NULL)
	{
		logger.TraceError("start threading 1 error");
		MessageBox(_T("start threading 1 error"));
	}
	Sleep(100);

	logger.TraceKeyInfo("starting threading 2...");
	CWinThread *pTheard_process2;
	pTheard_process2 = AfxBeginThread(ImageProcessThread, this);
	if (pTheard_process2 == NULL)
	{
		logger.TraceError("start threading 2 error");
		MessageBox(_T("start threading 2 error"));
	}
	Sleep(100);

	logger.TraceKeyInfo("starting threading 3...");
	CWinThread *pTheard_process3;
	pTheard_process3 = AfxBeginThread(ImageProcessThread, this);
	if (pTheard_process3 == NULL)
	{
		logger.TraceError("start threading 3 error");
		MessageBox(_T("start threading 3 error"));
	}
}


// CImgprcOCRInstanceView 诊断

#ifdef _DEBUG
void CImgprcOCRInstanceView::AssertValid() const
{
	CFormView::AssertValid();
}

void CImgprcOCRInstanceView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CImgprcOCRInstanceDoc* CImgprcOCRInstanceView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CImgprcOCRInstanceDoc)));
	return (CImgprcOCRInstanceDoc*)m_pDocument;
}
#endif //_DEBUG


// CImgprcOCRInstanceView 消息处理程序

void CImgprcOCRInstanceView::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	CFormView::OnTimer(nIDEvent);
		int i;
	DWORD dwIndex,dwRep1,dwTemp1,dwTemp2;
	CString sTemp;
	CString cstrLine;
	CString cstrNewLine;
	CTime ct = CTime::GetCurrentTime();
	char chsTemp[IS_IMAGE_MAX_PATH];
	char chsFN[IS_IMAGE_MAX_PATH];
	CImgprcOCRInstanceDoc *pDoc = (CImgprcOCRInstanceDoc *) GetDocument();
	CString cstrCurrDay = ct.Format("%Y-%m-%d");
//	if(cstrCurrDay!=m_cstrPreDay)
//	{
//		m_ctime = CTime::GetCurrentTime();//tx20171004
////		WriteStatoLog();
//		m_Sta.clear();
//		m_Err.clear();
////		ToDisplay(0,_T("统计清零！"));
//		
//		m_cstrPreDay=cstrCurrDay;
//
//		if(m_pFileLog!=NULL)
//		{
//			fclose(m_pFileLog);
//			CStringToCharArray(m_cstrLogDir,chsTemp);
//			sprintf_s(chsFN,"%sOCR%dLog%04d%02d%02d.txt",chsTemp,m_iLocPort,m_ctime.GetYear(),m_ctime.GetMonth(),m_ctime.GetDay());
//			m_pFileLog=fopen(chsFN,"a+");
//
//		}
//	}

	for (i=0;i<2;i++)
	{
		if (m_bShowMain==0) 
		{
			m_Region[i].m_dwProcP=m_Region[i].m_dwRevP;
			continue;
		}

		dwTemp1=m_Region[i].m_dwProcP;
		dwTemp2=m_Region[i].m_dwRevP;
		for (dwRep1=dwTemp1+1;dwRep1<=dwTemp2;dwRep1++)
		{
			switch (i)
			{
			case 0:  // Main message
				if (m_crlMain.GetLineCount()>=m_Region[i].m_nMax)
				{
					m_crlMain.SetWindowText(_T(""));
				}
				dwIndex=dwRep1%IS_DISPLAY_BUFFER_ROW;
				sTemp.Format(_T("%s\r\n"),m_Region[i].m_uchsMsg[dwIndex]);
				m_crlMain.ReplaceSel(sTemp);
				if(m_pFileLog!=NULL)
				{
					CStringToCharArray(sTemp, chsTemp);
					fprintf(m_pFileLog,"%s", chsTemp);
					fflush(m_pFileLog);
				}
				break;
			case 1:  //message
				if (m_crlMsg.GetLineCount()>=m_Region[i].m_nMax)
				{
					m_crlMsg.SetWindowText(_T(""));
				}
				dwIndex=dwRep1%IS_DISPLAY_BUFFER_ROW;
				sTemp.Format(_T("%s\r\n"),m_Region[i].m_uchsMsg[dwIndex]);
				m_crlMsg.ReplaceSel(sTemp);
				break;

			}
		}
		m_Region[i].m_dwProcP=dwTemp2;

	}//END OF FOR

	//与服务器连接
	ClientSocketonTimer();

	//与数据库连接
	//CheckConnection();

	int unprocessdnum = GetUnderProcessTaskNum();

	cstrNewLine.Format(_T("%d"), unprocessdnum);

	m_crlRestTask.SetWindowTextW(cstrNewLine);



	//接收任务数目
	m_crlTotal.GetWindowTextW(cstrLine);
	cstrNewLine.Format(_T("%lu"), m_Sta.m_dwTotal);
	if (cstrNewLine != cstrLine)
	{
		m_crlTotal.SetWindowTextW(cstrNewLine);
	}

	//tagOCR数量

	m_crlTagOCROK.GetWindowTextW(cstrLine);
	cstrNewLine.Format(_T("%lu"), m_Sta.m_dwTagOcrOKCount);
	if (cstrNewLine != cstrLine)
	{
		m_crlTagOCROK.SetWindowTextW(cstrNewLine);
	}




	//取图数量
	//m_crlReadImg.GetWindowTextW(cstrLine);
	//cstrNewLine.Format(_T("%lu"), m_Sta.m_dwReadImgok);
	//if(cstrNewLine!=cstrLine)
	//{
	//	m_crlReadImg.SetWindowTextW(cstrNewLine);
	//}
	//处理图像数量
	//m_crlProcessImg.GetWindowTextW(cstrLine);
	//cstrNewLine.Format(_T("%lu"), m_Sta.m_dwPrcImgok);
	//if(cstrNewLine!=cstrLine)
	//{
	//	m_crlProcessImg.SetWindowTextW(cstrNewLine);
	//}
	//识别条码数量

	m_crlObrOK.GetWindowTextW(cstrLine);
	cstrNewLine.Format(_T("%lu"), m_Sta.m_dwObrOK);
	if(cstrNewLine!=cstrLine)
	{
		m_crlObrOK.SetWindowTextW(cstrNewLine);
	}


		//回写数量

	m_crlCopyImg.GetWindowTextW(cstrLine);
	cstrNewLine.Format(_T("%lu"), m_Sta.m_dwCopyImgok);
	if(cstrNewLine!=cstrLine)
	{
		m_crlCopyImg.SetWindowTextW(cstrNewLine);
	}

	m_crlSocket.GetWindowTextW(cstrLine);
	if(m_bClientSockState)
	{
		cstrNewLine.Format(_T("OCR:%d connected with IADM %s"),m_iLocPort,m_cstrSeverIP);
	}
	else
	{
		cstrNewLine.Format(_T("OCR:%d disconnect with IADM %s"),m_iLocPort,m_cstrSeverIP);
	}
	if(cstrNewLine!=cstrLine)
	{
		m_crlSocket.SetWindowTextW(cstrNewLine);
	}

}

afx_msg int CImgprcOCRInstanceView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFormView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	ReadINI();
	CString sTemp;
	//if(m_IMGSVRDB.InitiDB(m_chsDBUser,m_chsDBPassword,m_chsDBName ,m_chsDBIP)!=0)
	//{
	//	sTemp = CString(_T("数据库连接失败！"))+CString((char *)m_IMGSVRDB.m_bstrErrDsp);
	//	ToDisplay(0,sTemp);
	//}
	//else
	//{
	//	ToDisplay(0,CString(_T("数据库连接成功！")));
	//}


	//int res = SF_InfoInit(&m_VcsRes);

	//if ( 0 != res )
 //   {
 //       if ( -10 == res )
 //       {
 //           // 返回值为-10表示不能授权，初始化失败
 //           sTemp = CString(_T("此电脑未授权。\n" ));
 //       }
 //       else
 //       {
 //           sTemp = CString(_T( "初始化失败。\n" ));
 //       }


 //   }
 //   else
 //   {
 //       sTemp = CString(_T("初始化成功。\n" ));
 //   }


	ToDisplay(0,sTemp);


	CTime ct =CTime::GetCurrentTime();
	m_cstrPreDay = 	ct.Format("%Y-%m-%d");
	InitializeCriticalSection(&crt_section);



		//设置显示的时间
	SetTimer(1, IADM_TIMER_INTERVAL,NULL);  // Create display time 

	//处理图像
	m_MMT1.AddTimerListener((LPTIMECALLBACK)TimeFunctionMsg);
	m_MMT1.Start((DWORD_PTR)this);


	m_MMT2.AddTimerListener((LPTIMECALLBACK)TimeFunctionImg);
	m_MMT2.Start((DWORD_PTR)this);


	return 0;

}

// 向显示区放入显示内容
void CImgprcOCRInstanceView::ToDisplay(BYTE nRegionNo, CString cstrMsg)
{

	BYTE bRep;
	int bLen;
	DWORD dwTemp1;
	CString sTemp;
	if ((m_Region[nRegionNo].m_dwRevP-m_Region[nRegionNo].m_dwProcP)<(IS_DISPLAY_BUFFER_ROW-1))
	{
		dwTemp1=(m_Region[nRegionNo].m_dwRevP+1)%IS_DISPLAY_BUFFER_ROW;
		if (nRegionNo==0 ||nRegionNo==1 ) sTemp.Format(_T("[%s]%s"),GetNowTime(2),cstrMsg);
		else sTemp.Format(_T("%s"),cstrMsg);

		if(sTemp.GetLength()>200) bLen=200;
		else bLen=(BYTE)sTemp.GetLength();

		for(bRep=0;bRep<250;bRep++)
		{
			m_Region[nRegionNo].m_uchsMsg[dwTemp1][bRep]=0;
		}

		memcpy(m_Region[nRegionNo].m_uchsMsg[dwTemp1],sTemp.GetBuffer(bLen),bLen*2);
//		for(bRep=0;bRep<bLen;bRep++)
//		{
//			m_Region[nRegionNo].m_uchsMsg[dwTemp1][bRep]=sTemp.GetAt(bRep);
//		}	
		m_Region[nRegionNo].m_dwRevP++;
	}

}

//消息处理
void CALLBACK TimeFunctionMsg(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	CImgprcOCRInstanceView * pView = (CImgprcOCRInstanceView *)lpParam;
	pView->TCPClientTask();
}

//准备需要发送的消息
void CALLBACK TimeFunctionImg(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{

	CImgprcOCRInstanceView * pView = (CImgprcOCRInstanceView *)lpParam;

	if (pView->m_bClientSockState == FALSE) return;

	ImgprcTask mTask;
	int datanum = pView->GetAnFinishedImageProcessTask(mTask);
	if (datanum !=1 )
	{
		return;
	}
	BYTE pBuffer[512] = { 0 };//储存数据
	BYTE pRawBuffer[512] = { 0 };//存储加待发送数据，已加密
	int datalength = pView->GenerateMsgData2ImageServer(pBuffer, mTask);
	pView->m_MachineComm.CodeMsgFromOCR2ImgServer(datalength, pBuffer,pRawBuffer);

	pView->m_ClientSock.Send(pRawBuffer, strlen((char*)pRawBuffer), 0);
	DWORD re = GetLastError();
	if (re == 0)
	{
		CString info_str;
		CString proc_type;
		if (mTask.m_processType==ST_TASK_PROC_TAG)
		{
			proc_type = _T("T");
		}
		if (mTask.m_processType == ST_TASK_PROC_HWBOX)
		{
			proc_type = _T("H");
		}
		if (mTask.m_processType == ST_TASK_PROC_UNKNOWN_TAG)
		{
			proc_type = _T("U");
		}
		USES_CONVERSION;
		info_str.Format(_T("%s"), A2T(mTask.m_chsOcrPostcode));
		info_str = proc_type + _T(":") + info_str;
		pView->ToDisplay(1, info_str);
		int a = pView->DelAnImageProcessTask(mTask);
		//sprintf((char*)pBuffer, "删除了%d个任务", a);
		//logger.TraceInfo((char*)pBuffer);
	}
	else
	{
		pView->ToDisplay(0, CString(_T("OCR2Imsgsvr Send MSG Error")));
	}
	if (mTask.m_postcodeNum >= 1)
	{
		pView->m_Sta.m_dwTagOcrOKCount++;
	}
	//pView->CheckConnection();
}


	// //接受Imgserver的消息
void CImgprcOCRInstanceView::TCPClientTask(void)
{
	DWORD dwRep1,dwTemp1,dwTemp2;
	CString sMsg;
	BYTE bTemp1;
	BYTE * pbTemp1=0;
	DWORD i;
	BYTE nLen=0;
	DWORD re;
	CImgprcOCRInstanceDoc * pDoc =(CImgprcOCRInstanceDoc *) GetDocument();
	//ToDisplay(0,L"定时器响应了");
	// Process receive socket message
	if (m_bClientSockState == 1)
	{
		//从socket获取数据
		size_t received_count = m_ClientSock.GetReceivedData(m_MachineComm.m_uchsRawData, IADM_MES_MAX_LEN * 2);
		for (size_t i=0;i<received_count;i++)
		{
			m_MachineComm.AnalyseOneByte(m_MachineComm.m_uchsRawData[i]);
		}
		//logger.TraceInfo("收到了一个消息");
		//处理消息
		BYTE * messageDataBuffer = m_MachineComm.GetOneMessage();
		if (messageDataBuffer!=NULL)
		{
			ProcessOneRMes(messageDataBuffer);
			//logger.TraceInfo("处理了一个消息");
		}
		
	}
}


//解析任务，并将任务推入队列
void  CImgprcOCRInstanceView::ProcessOneRMes(BYTE* mbMesPos)
{
	int i=0;
	BYTE *p = mbMesPos;
	BYTE bType;
	CString sTemp;
	ImgprcTask Task;
	DWORD dwque=0, dwqueH;
	int img_path_length = 0;
	int bar_code_length = 0;
	BYTE img_length_1;
	BYTE img_length_2;
	std::vector<ImgprcTask> subTasks;
	try
	{
		int posnow = 0;
		posnow = 1;
		bType=*(p+ posnow++); //第一位为msg length
		BYTE bb[512] = {0};
		memcpy(bb, p, 10);
	

		switch (bType)  // Message type
		{
		case MPF_MSG_IMG2OCR_REQUEST:
			Task.clear();
			Task.m_dwTimeTrigger = GetTickCount();
			//Task.m_iTrayNo = p[3] * 256 + p[4];
			BytesToLongBigEndian(Task.m_dwLImageID, Task.m_dwHImageID, p, 3, 6);
			for (i = 0; i < 6; i++)
			{
				Task.m_TaskID[i] =  *(p + posnow++);
			}
			Task.m_iOCRID = m_OCRID;
			//Task.m_bEnable = p[11];
			Task.m_requestType = *(p + posnow++);
			Task.m_image_total_num = *(p + posnow++);
			img_length_1 = *(p + posnow++);
			img_length_2 = *(p + posnow++);
			img_path_length = img_length_1 * 256 + img_length_2;
			if (img_path_length>0 && img_path_length <= IS_IMAGE_MAX_PATH * IS_MAX_IMG_PER_TASK)
			{
				memcpy(Task.m_chsFileName, (p + posnow), img_path_length);
			}
			posnow += img_path_length;
			Task.m_isTopViewImage = *(p + posnow++);
			Task.m_Barcode_len = *(p + posnow++);
			if (Task.m_Barcode_len >0)
			{
				memcpy(Task.m_chsBarcode, p + posnow, Task.m_Barcode_len);
			}
			
			SplitTask(Task, subTasks);
			for (i=0;i<subTasks.size();i++)
			{
				PushAnImageProcessTask(subTasks[i]);
			}
			m_Sta.m_dwTotal++;
			//logger.TraceInfo("推入了一个任务");
			break;
		case MPF_MSG_IMG2OCR_FUNC:
			break;

		}
	}
	catch(...)
	{
		sTemp.Format(_T("ProcessOneRMes():Error IADM RevMesPos"), mbMesPos);
		ToDisplay(0, sTemp);
	}
}

// 重启电脑
int CImgprcOCRInstanceView::Reboot(void)
{
		HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken))
		return -1;
	LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
	tkp.PrivilegeCount=1;
	tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);
	ExitWindowsEx(EWX_REBOOT|EWX_FORCE,0);
	return 1;

}

// 关闭电脑
int CImgprcOCRInstanceView::ShutDown(void)
{

	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken))
		return -1;
	LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
	tkp.PrivilegeCount=1;
	tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);
	ExitWindowsEx(EWX_SHUTDOWN|EWX_FORCE,0);
	return 1;
}







	// 读配置文件
int CImgprcOCRInstanceView::ReadINI(void)
{
	WCHAR wchsTemp[IS_MES_MAX_LEN];
	CString cstrTemp;

	GetCurrentDirectoryW(IS_MES_MAX_LEN,wchsTemp);
	CString cstrIniPath = CString(wchsTemp);
	cstrIniPath +=CString(_T("\\system.ini"));



	CString app;
	CString key;
	app = CString(_T("SETTINGS"));

	key =CString(_T("ImgDir")); 
	GetPrivateProfileStringW(app,key,LPCWSTR("0"),wchsTemp,IS_MES_MAX_LEN,cstrIniPath); 
	m_cstrImgDir= wchsTemp;

	key =CString(_T("ProcessedDir")); 
	GetPrivateProfileStringW(app,key,LPCWSTR("D:\\ImageForVideo\\"),wchsTemp,IS_MES_MAX_LEN,cstrIniPath); 
	m_cstrProcessedDir= wchsTemp;


	key =CString(_T("LocIP"));
	GetPrivateProfileStringW(app,key,LPCWSTR("0"),wchsTemp,IS_MES_MAX_LEN,cstrIniPath); 
	m_cstrIP= wchsTemp;
	
	key  = CString(_T("LocPort"));
	m_iLocPort = GetPrivateProfileIntW(app,key,10001,cstrIniPath);


	key = CString(_T("OCRID"));
	m_OCRID = GetPrivateProfileIntW(app, key, 1, cstrIniPath);

	key =CString(_T("LogPath"));
	GetPrivateProfileStringW(app,key,LPCWSTR(_T("log\\")),wchsTemp,IS_MES_MAX_LEN,cstrIniPath);
	m_cstrLogDir = wchsTemp;

	key =CString(_T("IADMIP"));
	GetPrivateProfileStringW(app,key,LPCWSTR("192.168.1.30"),wchsTemp,IS_MES_MAX_LEN,cstrIniPath); 
	m_cstrSeverIP= wchsTemp;

	key = CString(_T("IADMPORT"));
	m_cstrSeverPort = GetPrivateProfileIntW(app, key, 9999, cstrIniPath);

	key = CString(_T("TagDetectConfidence"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0.3"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.TagDetectConfidence = _ttof(wchsTemp);

	key = CString(_T("HWDigits_confidence"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0.85"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.HandwriteDigitsConfidence = _ttof(wchsTemp);


	key = CString(_T("Run_OCR_on_standard_tag"));
	GetPrivateProfileStringW(app, key, LPCWSTR("1"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.Run_OCR_on_standard_tag = _ttoi(wchsTemp);

	key = CString(_T("Run_OCR_on_handwrite_box"));
	GetPrivateProfileStringW(app, key, LPCWSTR("1"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.Run_OCR_on_handwrite_box = _ttoi(wchsTemp);

	key = CString(_T("InstanceNumPerClass"));
	GetPrivateProfileStringW(app, key, LPCWSTR("1"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.max_instance_per_class = _ttoi(wchsTemp);


	USES_CONVERSION;
	key = CString(_T("ORB_Template_img1"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.ORB_template_img1_path = CT2A(wchsTemp);

	key = CString(_T("ORB_Template_img2"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.ORB_template_img2_path = CT2A(wchsTemp);


	key = CString(_T("HW_Template_img1"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.handwrite_ref_img1_path = CT2A(wchsTemp);


	key = CString(_T("Detect_model_path"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.detect_model_file_path = CT2A(wchsTemp);




	key = CString(_T("HWDigits_model_path"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.handwrite_ocr_model_path = CT2A(wchsTemp);




	key  = CString(_T("LastIMAGE"));
	m_dwLastIMAGE = GetPrivateProfileIntW(app,key,20000,cstrIniPath);
	
	key  = CString(_T("MaxImageNum"));

	m_iMaxImageNum = GetPrivateProfileIntW(app,key,6,cstrIniPath);
	//tx20171004
	key = CString(_T("DBName"));

	char chsPath[512]={0};
	CStringToCharArray(cstrIniPath, chsPath);
	GetPrivateProfileStringA("SETTINGS","DBName","0",m_chsDBName,IS_MES_MAX_LEN,chsPath);
	
	GetPrivateProfileStringA("SETTINGS","DBIP","0",m_chsDBIP,IS_MES_MAX_LEN,chsPath);

	GetPrivateProfileStringA("SETTINGS","DBUser","0",m_chsDBUser,IS_MES_MAX_LEN,chsPath);

	GetPrivateProfileStringA("SETTINGS","DBPassword","0",m_chsDBPassword,IS_MES_MAX_LEN,chsPath);
	


	key  = CString(_T("Rank")); //tx20171201

	m_iRank = GetPrivateProfileIntW(app,key,1,cstrIniPath);//tx20171201
	
	
	return 0;

}

CString GetLocalFilenameBy(DWORD ll,DWORD hh, int & va)
{
	CString cst=_T("");


	va=0;
	DWORD vv = (hh*4294967296+ll)/10%500;
//	DWORD tt = (hh*4294967296+ll)%10;
	cst.Format(_T("%03lu.jpg"),vv);
	va =(int) vv;

	return cst;

}
void CImgprcOCRInstanceView::ImagesProcessing(OcrAlgorithm_config* pConfig)
{
	ImgprcTask mtask;
	int re = 0;
	int task_num = GetAnImageProcessTask(mtask);
	if (task_num == 0)
	{
		Sleep(50);
		return;
	}
	//std::string local_img_dir = CT2A(m_cstrImgDir);
	if (mtask.m_chsLocalImgPath[0]==0)
	{
		re = mtask.CopyImageFilesFromServer(m_MachineComm.m_chsIPAddress, m_cstrImgDir, &logger, &e, false);
	}
	else
	{
		re = 1;
	}
	if (re != 0)
	{
		mtask.PorcessTask(pConfig, &logger);//处理任务的主函数
	}
	
	OnAnImageProcessTaskOver(mtask); //处理完一个任务
	if (mtask.m_image_total_num == 1) return;

	//打包子任务
	std::vector<ImgprcTask> waitPackageTasks;
	re = GetWaitPackageTasks(mtask, waitPackageTasks);
	if (re >= mtask.m_image_total_num)
	{
		MergeHomoTasks(waitPackageTasks, mtask);
		BYTE next_type = mtask.GetNextProcessType(pConfig);
		if (next_type!=ST_TASK_PROC_OVER)// && mtask.m_postcodeNum == 0)
		{
			ChangeTasksProcessType(mtask, mtask.m_processType, next_type);
		}
		else
		{
			DelAnImageProcessTask(mtask);//删除所有待打包任务
			PushAnImageProcessTask(mtask);//推入打包后的已完成任务
		}
	}
	////从服务器拷贝图像文件到本地
	//USES_CONVERSION;
	//CString imgFileName = A2T(mtask.m_chsFileName);

	////imgFileName.Format(_T("%s"), mtask.m_chsFileName);
	//int pos = imgFileName.Find(_T(":"));
	//imgFileName.Delete(0, pos + 2);//删除本地盘符
	//CString ipAddressServer;
	//ipAddressServer.Format(_T("\\\\%s\\"), A2T(m_MachineComm.m_chsIPAddress));
	//imgFileName.Insert(0, ipAddressServer);//加入网络地址
	//CString localImgDir;//本地地址
	//localImgDir.Format(_T("%s"), m_cstrImgDir);
	//CString localImgName;
	//DWORD vv = (mtask.m_dwHImageID * e() + mtask.m_dwLImageID) / 10 % 500;
	//localImgName.Format(_T("%03lu.jpg"), vv);
	//localImgDir.Append(localImgName);
	//re = CopyFile(imgFileName.GetBuffer(0), localImgDir.GetBuffer(0), FALSE);
	//if (re==FALSE)
	//{
	//	logger.TraceKeyInfo("从远程拷贝文件失败！");
	//	mtask.m_resultState = 11;
	//	OnAnImageProcessTaskOver(mtask);
	//	return;
	//}
	////获取图片的本地位置
	//string imgNamestring = CT2A(localImgDir.GetBuffer(0));
	//strcpy(mtask.m_chsLocalImgPath, imgNamestring.c_str());
	

}







	


void CImgprcOCRInstanceView::CheckConnection(void)
{
	long re=0;

//	this->m_dwTimeCount += IADM_TIMER_INTERVAL;
//	int i;
	CString cstrLine;

	//if((m_dwTimeCount%3000)==0)
	//{
	//	re=m_IMGSVRDB.CheckConnection();
	//	if(re==IMGSVRDB_RECONECTED)
	//	{
	//		m_Sta.m_dwReConnectedDB++;
	//		cstrLine.Format(_T("Reconnected DB %d"), m_Sta.m_dwReConnectedDB);
	//		ToDisplay(1, cstrLine);
	//	}
	//	else if(re==IMGSVRDB_CONN_CLOSED)
	//	{
	//		cstrLine.Format(_T("Reconnected DB Failed! ConnState%d"), m_IMGSVRDB.m_ptrConn->State);
	//		ToDisplay(1, cstrLine);
	//	}
	//	else if(re==IMGSVRDB_CONN_NOTCREATED)
	//	{
	//		cstrLine.Format(_T("Connection is Not Created! "));
	//		ToDisplay(1, cstrLine);
	//	}
	//}
}




void CImgprcOCRInstanceView::PushAnImageProcessTask(ImgprcTask mTask)
{
	EnterCriticalSection(&crt_section);
	this->m_Tasks.push_back(mTask);
	LeaveCriticalSection(&crt_section);
}

int CImgprcOCRInstanceView::GetAnImageProcessTask(ImgprcTask &mTask)
{
	int getElementsNum = 0;
	EnterCriticalSection(&crt_section);
	for (std::vector<ImgprcTask>::iterator iter = m_Tasks.begin(); iter != m_Tasks.end(); iter++)
	{
		if (iter->m_isProcessing == ST_TASK_UNPROCESS)
		{
			iter->m_isProcessing = ST_TASK_PROCESSING;
			mTask = *iter;
			getElementsNum = 1;
			break;
		}
	}
	LeaveCriticalSection(&crt_section);
	return getElementsNum;
}

int CImgprcOCRInstanceView::OnAnImageProcessTaskOver(ImgprcTask &mTask)
{
	int getElementsNum = 0;
	EnterCriticalSection(&crt_section);
	for (std::vector<ImgprcTask>::iterator iter = m_Tasks.begin(); iter != m_Tasks.end(); iter++)
	{
		if (iter->m_dwTimeTrigger == mTask.m_dwTimeTrigger 
			&& iter->m_index_sub_image == mTask.m_index_sub_image)
		{
			if (mTask.m_image_total_num == 1)
			{
				mTask.m_isProcessing = ST_TASK_PROCESSED;
				*iter = mTask;
				break;
			}
			else
			{
				mTask.m_isProcessing = ST_TASK_WAIT_PACKAGE;
				*iter = mTask;
				break;
			}
		}
	}
	LeaveCriticalSection(&crt_section);
	return getElementsNum;
}

int CImgprcOCRInstanceView::GetAnFinishedImageProcessTask(ImgprcTask &mTask)
{
	int getElementsNum = 0;
	EnterCriticalSection(&crt_section);
	for (std::vector<ImgprcTask>::iterator iter = m_Tasks.begin(); iter != m_Tasks.end(); iter++)
	{
		if (iter->m_isProcessing == ST_TASK_PROCESSED)
		{
			mTask = *iter;
			getElementsNum = 1;
			break;
		}
	}
	LeaveCriticalSection(&crt_section);
	return getElementsNum;
}

int CImgprcOCRInstanceView::DelAnImageProcessTask(ImgprcTask &mTask)
{
	int getElementsNum = 0;
	EnterCriticalSection(&crt_section);
	for (std::vector<ImgprcTask>::iterator iter = m_Tasks.begin(); iter != m_Tasks.end();)
	{
		if (iter->m_dwTimeTrigger == mTask.m_dwTimeTrigger)
		{
			iter = m_Tasks.erase(iter);
			getElementsNum ++;
		}
		else
		{
			iter++;
		}
	}
	LeaveCriticalSection(&crt_section);
	return getElementsNum;
}



int CImgprcOCRInstanceView::GetUnderProcessTaskNum()
{
	int getElementsNum = 0;
	EnterCriticalSection(&crt_section);
	for (std::vector<ImgprcTask>::iterator iter = m_Tasks.begin(); iter != m_Tasks.end(); iter++)
	{
		if (iter->m_isProcessing != ST_TASK_PROCESSED)
		{
			getElementsNum++;
		}
	}
	LeaveCriticalSection(&crt_section);
	return getElementsNum;
}

int CImgprcOCRInstanceView::GetWaitPackageTasksAll(std::vector<ImgprcTask> &wTasks)
{
	int getElementsNum = 0;
	EnterCriticalSection(&crt_section);
	for (std::vector<ImgprcTask>::iterator iter = m_Tasks.begin(); iter != m_Tasks.end(); iter++)
	{
		if (iter->m_isProcessing == ST_TASK_WAIT_PACKAGE)
		{
			wTasks.push_back(*iter);
			getElementsNum++;
		}
	}
	LeaveCriticalSection(&crt_section);
	return getElementsNum;
}

int CImgprcOCRInstanceView::GetWaitPackageTasks(ImgprcTask &mTask, std::vector<ImgprcTask> &wTasks)
{
	int getElementsNum = 0;
	EnterCriticalSection(&crt_section);
	for (std::vector<ImgprcTask>::iterator iter = m_Tasks.begin(); iter != m_Tasks.end(); iter++)
	{
		if (iter->m_isProcessing == ST_TASK_WAIT_PACKAGE 
			&& iter->m_dwTimeTrigger == mTask.m_dwTimeTrigger
			&& iter->m_processType == mTask.m_processType)
		{
			wTasks.push_back(*iter);
			getElementsNum++;
		}
	}
	LeaveCriticalSection(&crt_section);
	return getElementsNum;
}


int CImgprcOCRInstanceView::ChangeTasksProcessType(ImgprcTask &mTask, BYTE old_type, BYTE new_type)
{
	int getElementsNum = 0;
	EnterCriticalSection(&crt_section);
	for (std::vector<ImgprcTask>::iterator iter = m_Tasks.begin(); iter != m_Tasks.end(); iter++)
	{
		if (iter->m_dwTimeTrigger == mTask.m_dwTimeTrigger
			&& iter->m_processType == old_type)
		{
			iter->m_processType = new_type;
			iter->m_isProcessing = ST_TASK_UNPROCESS;
			getElementsNum++;
		}
	}
	LeaveCriticalSection(&crt_section);
	return getElementsNum;
}

int CImgprcOCRInstanceView::GenerateMsgData2ImageServer(BYTE *pData, ImgprcTask &mTask)
{
	int i = 0;
	int unProcessNum = GetUnderProcessTaskNum();
	int posnow = 0;
	for (i = 0; i < 6; i++)
	{
		pData[posnow++] = mTask.m_TaskID[i];
	}
	pData[posnow++] = m_OCRID; //本机id
	pData[posnow++] = mTask.m_resultState; //结果状态
	pData[posnow++] = mTask.m_PostcodeIndextImage; //邮编所在图片索引号
	//返回条码值
	pData[posnow++] = strlen(mTask.m_chsBarcode) % 256; // 如果超过256？？？
	memcpy(pData+posnow, mTask.m_chsBarcode, strlen(mTask.m_chsBarcode) % 256);
	posnow += strlen(mTask.m_chsBarcode) % 256;
	//邮编信息
	if (mTask.m_postcodeNum==0)
	{
		pData[posnow++] = 0;
	}
	else
	{
		int post_len = strlen(mTask.m_chsOcrPostcode);
		//pData[posnow++] = post_len / 256;
		pData[posnow++] = post_len % 256;
		memcpy(pData + posnow, mTask.m_chsOcrPostcode, post_len % 256);
		posnow += post_len % 256;
	}

	//剩余任务
	pData[posnow++] = unProcessNum % 256;
	pData[posnow] = 0;

	//char a[64] = { 0 };
	//sprintf_s(a, "排队等候数量：%d", unProcessNum);
	//logger.TraceInfo(a);
	return posnow;
}



int CImgprcOCRInstanceView::SplitTask(ImgprcTask &mTask, std::vector<ImgprcTask> &subTasks)
{
	
	if (mTask.m_image_total_num<=0)
	{
		return 0;
	}
	ImgprcTask subTask = mTask;
	if (mTask.m_image_total_num == 1) 
	{
		subTask.m_image_num = 1;
		subTasks.push_back(subTask);
		return 1;
	}
	std::vector<std::string> substrs;
	ImgprcTask::splitStrByChar(';', mTask.m_chsFileName, substrs);
	if (substrs.size() < mTask.m_image_total_num)
	{
		mTask.m_image_total_num = substrs.size();
	}

	for (int i=0;i< mTask.m_image_total_num;i++)
	{
		strcpy(subTask.m_chsFileName, substrs[i].c_str());
		if (i == 0)
		{
			subTask.m_isTopViewImage = true;
		}
		else
		{
			subTask.m_isTopViewImage = false;
		}
		subTask.m_index_sub_image = i;
		subTask.m_image_num = 1;
		subTasks.push_back(subTask);
	}

	return mTask.m_image_total_num;

}

int CImgprcOCRInstanceView::MergeHomoTasks(std::vector<ImgprcTask> &subTasks, ImgprcTask &mTask)
{
	if (subTasks.size() == 0) return 0;
	mTask = subTasks[0];

	for (int i = 1;i<subTasks.size();i++)
	{
		if (subTasks[i].m_postcodeNum>=1)
		{
			mTask.m_PostcodeIndextImage = subTasks[i].m_index_sub_image + 1;
			strcat(mTask.m_chsOcrPostcode, subTasks[i].m_chsOcrPostcode);
			//strcat(mTask.m_chsOcrPostcode, ";");
			mTask.m_postcodeNum += subTasks[i].m_postcodeNum;
		}
	}
	if (mTask.m_postcodeNum == 0) //识别到0个邮编
	{
		mTask.m_resultState = 12;
	}
	else if (mTask.m_postcodeNum == 1) //正确识别到1个邮编
	{
		mTask.m_resultState = 1;
	}
	else if (mTask.m_postcodeNum > 1)
	{
		mTask.m_resultState = 13;//多个邮编
	}

	mTask.m_isProcessing = ST_TASK_PROCESSED;
	
	return 1;
}

int CImgprcOCRInstanceView::WriteStatoLog(void)
{
	char chArrays[512],chCaption[512];
	unsigned int nchLen = 0;
	CString sTemp,cstrCaption;
	char fn[IS_IMAGE_MAX_PATH];
	char chsTemp[IS_IMAGE_MAX_PATH];
	fn[0] = 0;
	chsTemp[0] = 0;
	CStringToCharArray(m_cstrLogDir,chsTemp);	
	CTime ct = CTime::GetCurrentTime();
	if(m_pFileSta==NULL)
	{
		sprintf_s(fn,"OCR%dSTA%04d%02d%02d.txt",m_iLocPort,ct.GetYear(),ct.GetMonth(),ct.GetDay());
		m_pFileSta=fopen(fn,"a+");
	}
	// TODO: Add your command handler code here
	if(m_pFileSta)
	{
		sTemp  = GetNowTime(0);
		CStringToCharArray(sTemp,chArrays);
		fprintf_s(m_pFileSta,"\n\n开始时间:%s\n",chArrays);
		fflush(m_pFileSta);
		//socket
		this->m_crlSocket.GetWindowTextW(sTemp);
		CStringToCharArray(sTemp,chArrays);		
		GetDlgItem(IDC_STATIC_SOCKET)->GetWindowTextW(cstrCaption);
		CStringToCharArray(cstrCaption, chCaption);
		fprintf_s(m_pFileSta,"%s:%s\n",chCaption,chArrays);
		fflush(m_pFileSta);

		//处理的邮件总数
		this->m_crlTotal.GetWindowTextW(sTemp);
		CStringToCharArray(sTemp,chArrays);
		GetDlgItem(IDC_STATIC_TOTAL)->GetWindowTextW(cstrCaption);
		CStringToCharArray(cstrCaption, chCaption);
		fprintf_s(m_pFileSta,"%s:%s\n",chCaption,chArrays);
		fflush(m_pFileSta);

		//读图		
		//this->m_crlReadImg.GetWindowTextW(sTemp);
		//CStringToCharArray(sTemp,chArrays);
		//GetDlgItem(IDC_STATIC_READIMG)->GetWindowTextW(cstrCaption);
		//CStringToCharArray(cstrCaption, chCaption);
		//fprintf_s(m_pFileSta,"%s:%s\n",chCaption,chArrays);
		//fflush(m_pFileSta);

		//处理图像	
		//this->m_crlProcessImg.GetWindowTextW(sTemp);
		//CStringToCharArray(sTemp,chArrays);
		//GetDlgItem(IDC_STATIC_PRCIMG)->GetWindowTextW(cstrCaption);
		//CStringToCharArray(cstrCaption, chCaption);
		//fprintf_s(m_pFileSta,"%s:%s\n",chCaption,chArrays);
		//fflush(m_pFileSta);

		//读取图像和XML	
		this->m_crlCopyImg.GetWindowTextW(sTemp);
		CStringToCharArray(sTemp,chArrays);
		GetDlgItem(IDC_STATIC_COPYIMG)->GetWindowTextW(cstrCaption);
		CStringToCharArray(cstrCaption, chCaption);
		fprintf_s(m_pFileSta,"%s:%s\n",chCaption,chArrays);
		fflush(m_pFileSta);

		//obr
		this->m_crlObrOK.GetWindowTextW(sTemp);
		CStringToCharArray(sTemp,chArrays);
		GetDlgItem(IDC_STATIC_OBR)->GetWindowTextW(cstrCaption);
		CStringToCharArray(cstrCaption, chCaption);
		fprintf_s(m_pFileSta,"%s:%s\n",chCaption, chArrays);
		fflush(m_pFileSta);



		ToDisplay(0,CString(_T("统计值已经写入当天Sta日志！")));
		fclose(m_pFileSta);
		m_pFileSta = NULL;
		return 1;
	}
	else
	{
		ToDisplay(0,CString(_T("统计值未能写入当天Sta日志！")));
		return 0;
	}
}



/*
void CIMGSVRView::Case2ImageProcessing(MAILDATA * pMail)
{

	CString cstrfliter;
	CString cstrLine;
	//	MAILDATA * pMail = NULL;
	int re = 0,rexml=0;
	int i;
	int tt;
	char chsFormat[]="**&&&&&&&&&**";
	char chstemp[IS_MES_MAX_LEN];
	char chstemp2[IS_MES_MAX_LEN];
	//	char chsItem[IS_IMAGE_MAX_PATH];
	CString cstrItem;
	CString cstrNewName,cstrName;
	CString cstrDir;
	CString cstrJPG, cstrXML;
	int pos=-1;
	int reDB=0;

	//用文件最后修改时刻重新命名图像文件
	sprintf_s(pMail->m_chsNewFileName, "%04d%02d%02d%02d%02d%02d%03d_%s",
		pMail->m_stLastWriteTime.wYear,pMail->m_stLastWriteTime.wMonth, pMail->m_stLastWriteTime.wDay,\
		pMail->m_stLastWriteTime.wHour,pMail->m_stLastWriteTime.wMinute,pMail->m_stLastWriteTime.wSecond,\
		pMail->m_stLastWriteTime.wMilliseconds, pMail->m_chsImageName);

	if(m_bSaveOBRed)
	{
		re = SaveJpgXmltoNewDir(pMail, this->m_cstrOBRedDir);
		if(re & SAVE_JPG_OK)
		{
			m_sta.m_dwOBRedJPG++;
		}
		if(re & SAVE_XML_OK )
		{
			m_sta.m_dwOBRedXML++;		
		}
	}

	if(m_bSaveRej1==1 && pMail->m_bEnable==IADM_ENABLE_OBR) //保存条码阅读器返回拒绝识别的图像和xml
	{
		if(pMail->m_dwOBR1BarNum<=0)
		{
			tt = GetTickCount();
			re = 0;
			re = SaveJpgXmltoNewDir(pMail, this->m_cstrSaveRejPath1);
			tt = GetTickCount() - tt;
			if(re & SAVE_JPG_OK )
			{
				m_sta.m_dwRej1SaveJPG++;
			}
			if(re & SAVE_XML_OK )
			{
				m_sta.m_dwRej1SaveXML++;		
			}
			cstrLine.Format(_T("REGN:%03d E%x F%d Rej1SaveJPG&XML:%d (%dms) JPGNUM:%d XMLNUM:%d"),\
				pMail->m_dwTrayNo,pMail->m_bEnable,pMail->m_bFlag,re,tt,m_sta.m_dwRej1SaveJPG,m_sta.m_dwRej1SaveXML);
			ToDisplay(0,cstrLine);
		}
	}

	if(pMail->m_bEnable==IADM_ENABLE_OBR)
	{
		//读取XML
		tt = GetTickCount();
		re = ReadXMLFile(pMail);
		tt = GetTickCount() - tt;
		pMail->m_dwTimeReadXMLFile = tt;
		if(re==-1)
		{
			m_Err.m_dwErrReadXMLFile++;
		}
		else 
		{
			m_sta.m_dwOpenXmlOK++;
			if(pMail->m_dwOBR3BarNum>0)
				m_sta.m_dwOBR3++;
		}
		cstrLine.Format(_T("REGN:%03d E%x F%d ReadXML:%d(%dms) BarNUM:%d"), \
			pMail->m_dwTrayNo, pMail->m_bEnable, pMail->m_bFlag, re, tt, pMail->m_dwOBR3BarNum);
		ToDisplay(0,cstrLine);

		if(pMail->m_dwOBR3BarNum>0)
		{
			m_sta.m_dwOBR1++;
			for(i=0; i<int(pMail->m_dwOBR3BarNum); i++)
			{
				strcpy_s(chstemp, &(pMail->m_chsBar3[i*IS_BAR_MAX_LEN]));
				strcpy_s(chstemp2, &(pMail->m_chsPos3[i*IS_POS_MAX_LEN]));
				cstrLine.Format(_T("\t%s [%s] "),CString(chstemp), CString(chstemp2));
				ToDisplay(0,cstrLine);	
			}
		}
		tt = GetTickCount();
		//tx bind var
//		reDB = m_IMGSVRDB.UpdateMailRecordXML(pMail,m_dwImageNo);
		reDB = m_IMGSVRDB.bvUpdateMailRecordXML(pMail,m_dwImageNo);
		tt = GetTickCount() - tt;

		if(reDB== IMGSVRDB_RETURN_OK)
		{
			m_sta.m_dwUpdateMailRecordXML++;	
		}
		else
		{
			m_Err.m_dwErrUpdateMailRecordXML++;
		}
		cstrLine.Format(_T("REGN:%03d E%x F%d UpdateXMLtoDB:%d(%dms)"),pMail->m_dwTrayNo,pMail->m_bEnable,pMail->m_bFlag,reDB,tt);
		ToDisplay(0,cstrLine);	


		if(pMail->ReaderBarIsInXML())
			//if(pMail->m_dwOBR1BarNum == pMail->m_dwOBR3BarNum)
		{
			m_sta.m_dwReaderSame++;
			pMail->m_bReaderSame = 1;
			cstrLine.Format(_T("REGN:%03d E%x F%d Bars_in_RealTime&XML_Same:%d"),\
				pMail->m_dwTrayNo,pMail->m_bEnable,pMail->m_bFlag,pMail->m_bReaderSame);
		}
		else
		{
			pMail->m_bReaderSame = 0;
			cstrLine.Format(_T("REGN:%03d E%x F%d Bars_in_RealTime&XML_Same:%d FN:%s"),\
				pMail->m_dwTrayNo,pMail->m_bEnable,pMail->m_bFlag,pMail->m_bReaderSame,CString(pMail->m_chsNewFileName));
		}
		ToDisplay(0,cstrLine);	
	}//if(pMail->m_bEnable==IADM_ENABLE_OCR)

	//将图像复制到OVCR需要访问的路径
	tt = GetTickCount();
	re = ReadImageFile(pMail->m_chsJPGName, m_uchsImage,pMail->m_dwImgWidth, pMail->m_dwImgHeight);
	tt = GetTickCount()-tt;
	pMail->m_dwTimeReadJpgFile = tt;
	if(re!=1)
	{
		if(re ==-1) //图像过大
		{
			m_Err.m_dwErrImgOverSize++;
			cstrLine.Format(_T("REGN:%03d E%x F%d ImageSizeOver8196*8196! Image:%s"),\
				pMail->m_dwTrayNo,pMail->m_bEnable,pMail->m_bFlag,CString(pMail->m_chsJPGName));
			ToDisplay(0,cstrLine);
		}
		else //其他原因
		{
			m_Err.m_dwErrReadImageFile++;
			cstrLine.Format(_T("REGN:%03d E%x F%d ReadJpgFailed! Image:%s"),\
				pMail->m_dwTrayNo,pMail->m_bEnable,pMail->m_bFlag,CString(pMail->m_chsJPGName));
			ToDisplay(0,cstrLine);
		}
	}
	else
	{
		cstrLine.Format(_T("REGN:%03d E%x F%d ReadJpgOK! Image:%s"),\
				pMail->m_dwTrayNo,pMail->m_bEnable,pMail->m_bFlag,CString(pMail->m_chsJPGName));
		ToDisplay(0,cstrLine);
	}

	//	cstrNewName.Format(_T("%s%s.jpg"), this->m_cstrBGRemovedDir, CString(pMail->m_chsImageName));
	cstrName = CString(pMail->m_chsJPGName);

	re=GetJPGNAME(pMail,cstrNewName);
	if(re==1)
	{
		m_sta.m_dwJPGName++;
	}
	cstrNewName = this->m_cstrBGRemovedDir+cstrNewName;

	CFileFind ff;
	CString cstrBGDir;
	cstrBGDir.Format(_T("%s%03d*.jpg"),m_cstrBGRemovedDir,pMail->m_dwTrayNo);
	re = ff.FindFile(cstrBGDir);
	int reDel=0;
	if(re)
	{
		while(re)
		{
			re = ff.FindNextFile();
			reDel= DeleteFileW(ff.GetFilePath());
			if(reDel!=1)
			{
				int Err=GetLastError();

				m_Err.m_dwErrDeleteFile++;
				cstrLine.Format(_T("%s can delete Error%d"),ff.GetFilePath(),Err);
				ToDisplay(0,cstrLine);
			}
		}
	}
	re = 0;
	re = CopyFile(cstrName,cstrNewName,0);
	if(re==1)
	{
		pMail->m_dwBRImgWidth =  pMail->m_dwImgWidth;
		pMail->m_dwBRImgHeight = pMail->m_dwImgHeight;
		pMail->m_dwBRLeft = 0;
		pMail->m_dwBRTop = 0;
		pMail->m_dwBRRight = pMail->m_dwBRImgWidth;
		pMail->m_dwBRBottom = pMail->m_dwBRImgHeight;
		m_sta.m_dwBROK++;
		cstrLine.Format(_T("REGN:%03d E%x F%d 图像复制到Z盘成功!"), pMail->m_dwTrayNo, pMail->m_bEnable, pMail->m_bFlag);
		ToDisplay(0,cstrLine);	

		reDB=0;
		tt = GetTickCount();
		//tx bind var
//		reDB = m_IMGSVRDB.UpdateMailRecordBR(pMail,m_dwImageNo);
		reDB = m_IMGSVRDB.bvUpdateMailRecordBR(pMail,m_dwImageNo);
		tt = GetTickCount() - tt;

		if(reDB==IMGSVRDB_RETURN_OK)
		{
			m_sta.m_dwUpdateMailRecordBR++;
		}
		else
		{
			m_Err.m_dwErrUpdateMailRecordBR++;
		}
		cstrLine.Format(_T("REGN:%03d E%x F%d UpdateBR:%d(%dms)"),pMail->m_dwTrayNo, pMail->m_bEnable, pMail->m_bFlag,reDB,tt);
		ToDisplay(0,cstrLine);	
	}
	else
	{
		m_Err.m_dwErrBR++;
		cstrLine.Format(_T("REGN:%03d E%x F%d 图像复制到Z盘失败!"), pMail->m_dwTrayNo, pMail->m_bEnable, pMail->m_bFlag);
		ToDisplay(0,cstrLine);	
	}

	//tx 20140518
	SendIMGRESULT(pMail);
}
*/



void CImgprcOCRInstanceView::ClientSocketonTimer(void)
{
	CImgprcOCRInstanceDoc * pDoc = (CImgprcOCRInstanceDoc *) GetDocument();
	CString sMsg,sMsg1,cstrLine;
	m_dwTimeCount+=IADM_TIMER_INTERVAL;
	m_MachineComm.m_dwTimeCount+=IADM_TIMER_INTERVAL;

	m_dwTcp2LastSendToNow += IADM_TIMER_INTERVAL;
	m_dwTimesTcp2Retry1 += IADM_TIMER_INTERVAL;
	
	if(!m_ClientSock.GetServerClosed() && m_ClientSock.IsRunning())
	{
		m_bClientSockState=1;
		m_MachineComm.m_uchValidFlag=1;
		m_dwTimesTcp2Retry1=0;

		//发送消息给IADM
		//tx 2014-05-29 3000->10000
		if((m_dwTimeCount%10000)==0)
		{
			//SendNetMsg();

			//TestTask();
		}
	}
	else
	{
		m_bClientSockState=0;
		m_MachineComm.m_uchValidFlag=0;
		if (m_dwTimesTcp2Retry1>IADM_TIMES_COMM_RETRY)
		{
			m_dwTimesTcp2Retry1=0;
			m_ClientSock.Stop();
			m_ClientSock.SetType(0);  // Set as client
			m_ClientSock.SetLocalPort(m_iLocPort);
			m_ClientSock.SetLocalIP(m_cstrIP);

			//m_ClientSock.SetServerIP(CString(_T("192.168.1.30")));
			m_ClientSock.SetServerIP(CString(m_MachineComm.m_chsIPAddress));
			m_ClientSock.SetRemotePort(m_cstrSeverPort);

			m_ClientSock.Start();
			/////2012-5-21  直接发消息，而非等缓冲区满再发消息//////
			BOOL bNoDelay=1;
			int nlen=sizeof(BOOL);
			int rev=1;
			int error;
			
			rev = m_ClientSock.SetSockOpt(TCP_NODELAY,&bNoDelay,nlen,IPPROTO_TCP);
			
			error = m_ClientSock.GetLastError();
			if(!rev)
			{
				error = GetLastError();
				cstrLine.Format(_T("ERROR TCP_NODELAY=%d rev=%d\n"),bNoDelay,error);
				ToDisplay(0, cstrLine);
			}
			//tx201708 reuseaddr
			BOOL bReuseaddr = TRUE;
			int reSet=m_ClientSock.SetSockOpt(SO_REUSEADDR,&bReuseaddr,sizeof(bReuseaddr),SOL_SOCKET);
			if(!reSet)
			{

				int err = GetLastError();
			
				
				cstrLine.Format(_T("SO_REUSEADDR reSet=%d  ERROR %d \n"),reSet,err);
				ToDisplay(0, cstrLine);
			}


			///////////////////
			if (m_ClientSock.GetLastError()==0 && m_ClientSock.IsRunning()) 
			{
			
				m_dwTcp2LastSendToNow=0xffff;
				m_dwTimeCount=0;
				m_bClientSockState=1;
				cstrLine=CString(_T("Success connected with ImgServer!"));			
				ToDisplay(0,cstrLine);
			}
			else 
			{
				cstrLine=CString(_T("Fail connected with ImgServer!"));			
				ToDisplay(0,cstrLine);
			}
		}
	}
}


void CImgprcOCRInstanceView::SendNetMsg(void)
{
	
	BYTE uchMsg[IS_MES_MAX_LEN];
	BYTE uchNew[IS_MES_MAX_LEN];
	memset(uchMsg,0,sizeof(BYTE)*IS_MES_MAX_LEN);
	memset(uchNew,0,sizeof(BYTE)*IS_MES_MAX_LEN);
	
	uchMsg[0] = MPF_MSG_START;
	uchMsg[1] = MPF_MSG_ADDR_OCR;
	uchMsg[2] = MPF_MSG_OCR2IMG_RESULT;	//图像机发送给IADM
	uchMsg[3] = 0;

	
	int len = 4;
	int newLen = 0;
	int i;
	CodeMessage(uchMsg,uchNew,len,newLen);

	m_ClientSock.Send(uchNew,newLen,0);
	int err = GetLastError();
	CString temp,sTemp;
	sTemp.Empty();
	for(i=0;i<len;i++)
	{
		temp.Format(_T("%x "), uchMsg[i]);
		sTemp +=temp;
	}
	if(err==0)
	{
		temp.Format(_T("发送成功"));
	}
	else
	{
		temp.Format(_T("发送失败Err=%d"),err);
		ToDisplay(1, sTemp);
	}
	sTemp += temp;

}

void CImgprcOCRInstanceView::SendImgprcResultMsg(ImgprcTask * pTask)
{
	BYTE uchMsg[IS_MES_MAX_LEN];
	BYTE uchNew[IS_MES_MAX_LEN];
	memset(uchMsg,0,sizeof(BYTE)*IS_MES_MAX_LEN);
	memset(uchNew,0,sizeof(BYTE)*IS_MES_MAX_LEN);
	
	uchMsg[0] = MPF_MSG_START;
	uchMsg[1] = 0xA0;
	uchMsg[2] = 0xC5;	//图像机发送给IADM

	DWORD n = 0;//(DWORD)(pTask->m_iTrayNo);
	LongToBytesBigEndian(n,0, uchMsg, 3, 2);
	LongToBytesBigEndian(pTask->m_dwLImageID,pTask->m_dwHImageID, uchMsg, 5, 6);
	uchMsg[11]=m_Sta.m_dwTotal-m_Sta.m_dwDoneImg-1;
	
	int len = 12;
	int newLen = 0;
	int i;
	CodeMessage(uchMsg,uchNew,len,newLen);

	m_ClientSock.Send(uchNew,newLen,0);
	int err = GetLastError();
	CString temp,sTemp;
	sTemp.Empty();
	for(i=0;i<len;i++)
	{
		temp.Format(_T("%x "), uchMsg[i]);
		sTemp +=temp;
	}
	if(err==0)
	{
		temp.Format(_T("发送成功"));
	}
	else
	{
		temp.Format(_T("发送失败Err=%d"),err);
	}
	sTemp += temp;
	ToDisplay(1, sTemp);
}
// 发送前编码
void CImgprcOCRInstanceView::CodeMessage(unsigned char * uchMsg, unsigned char * uchNew, int  len, int & newLen)
{
	BYTE byte=0;
	BYTE check = 0;
	uchNew[newLen++] = MPF_MSG_START;
	int i;
	for(i=1;i<len;i++)
	{
		check +=uchMsg[i];
		byte=uchMsg[i]>>4;
		if(byte>=0 && byte<=9)
		{
			uchNew[newLen++] = byte+'0';
		}
		else if(byte>=0x0a && byte<=0x0f)
		{
			uchNew[newLen++] = byte-0x0a+'A';
		}
		else
		{
			uchNew[newLen++] = 0;
		}
		byte = uchMsg[i] &0x0f;
		if(byte>=0 && byte<=9)
		{
			uchNew[newLen++] = byte+'0';
		}
		else if(byte>=0x0a && byte<=0x0f)
		{
			uchNew[newLen++] = byte-0x0a+'A';
		}
		else
		{
			uchNew[newLen++] = 0;
		}

	}

	check = ~check;
	check++;
	byte=check>>4;
	if(byte>=0 && byte<=9)
	{
		uchNew[newLen++] = byte+'0';
	}
	else if(byte>=0x0a && byte<=0x0f)
	{
		uchNew[newLen++] = byte-0x0a+'A';
	}
	else
	{
		uchNew[newLen++] = 0;
	}
	byte = check & 0x0f;
	if(byte>=0 && byte<=9)
	{
		uchNew[newLen++] = byte+'0';
	}
	else if(byte>=0x0a && byte<=0x0f)
	{
		uchNew[newLen++] = byte-0x0a+'A';
	}
	else
	{
		uchNew[newLen++] = 0;
	}
	uchNew[newLen++] = MPF_MSG_END1;
	uchNew[newLen++] = MPF_MSG_END0;
	uchNew[newLen] = 0;
}





void CImgprcOCRInstanceView::TestTask()
{
	//BYTE uchMsg[IS_MES_MAX_LEN];
	//BYTE uchNew[IS_MES_MAX_LEN];
	//memset(uchMsg,0,sizeof(BYTE)*IS_MES_MAX_LEN);
	//memset(uchNew,0,sizeof(BYTE)*IS_MES_MAX_LEN);
	//
	//uchMsg[0] = MPF_MSG_START;
	//uchMsg[1] = 0xF0;
	//uchMsg[2] = 0xC4;	//图像机发送给IADM

	//DWORD n = (DWORD)(4);
	//LongToBytesBigEndian(n, 0,uchMsg, 3, 2); //TRAY
	//LongToBytesBigEndian(10301,0, uchMsg, 5, 6);//IMGID
	//uchMsg[11]=1;
	//LongToBytesBigEndian(101,0, uchMsg, 12, 4);
	//
	//int len = MPF_MSG_IMG2OCR_REQUEST_LEN + 3;
	//int newLen = 0;
	//int i;
	//CodeMessage(uchMsg,uchNew,len,newLen);

	//CString temp,sTemp;
	//sTemp.Empty();
	//for(i=0;i<newLen;i++)
	//{
	//	temp.Format(_T("%02x"), uchNew[i]);
	//	sTemp +=temp;
	//}

	//temp.Format(_T(" Len%d"),newLen);
	//sTemp += temp;
	//ToDisplay(0, sTemp);



		BYTE uchMsg[IS_MES_MAX_LEN];
	BYTE uchNew[IS_MES_MAX_LEN];
	memset(uchMsg,0,sizeof(BYTE)*IS_MES_MAX_LEN);
	memset(uchNew,0,sizeof(BYTE)*IS_MES_MAX_LEN);
	
	uchMsg[0] = MPF_MSG_START;
	uchMsg[1] = 0xE0;
	uchMsg[2] = 0xC2;	//发给VCS

	LongToBytesBigEndian(1012,0, uchMsg, 3, 6);//IMGID
	
	int len = 9;
	int newLen = 0;
	int i;
	CodeMessage(uchMsg,uchNew,len,newLen);

	CString temp,sTemp;
	sTemp.Empty();
	for(i=0;i<newLen;i++)
	{
		temp.Format(_T("%02x"), uchNew[i]);
		sTemp +=temp;
	}

	temp.Format(_T(" Len%d"),newLen);
	sTemp += temp;
	ToDisplay(0, sTemp);


}
void CImgprcOCRInstanceView::OnTestSendreq()
{
	ImgprcTask Task;
	Task.m_dwLImageID=1;
	Task.m_dwHImageID=1;

	CString cstrNewName;cstrNewName.Format(_T("\\\\%s\\ImageForVideo\\%I64d.jpg"),m_cstrSeverIP,Task.m_dwHImageID*4294967296 +Task.m_dwLImageID);
	//m_IMGSVRDB.bvGetImageInfo(&Task);
	// TODO: Add your command handler code here
	TestTask();
}

// 计算Tasks当前空闲可放入新任务的Index
int CImgprcOCRInstanceView::GetTasksIndex(int  iTrayNo)
{

	//int index = iTrayNo%MAIL_MAX_NUM;
	//if(m_Tasks[index].m_bFlag==0)
	//	return index;
	//
	//for(int j=0;j<MAIL_MAX_NUM;j++)
	//{
	//	index = (index+j+1)%MAIL_MAX_NUM;
	//	if(m_Tasks[index].m_bFlag==0)
	//		return index;
	//}

	return -1;
}

void CImgprcOCRInstanceView::OnDebugeBt()
{
	CString gReadFilePathName;
	CString filename;
	CFileDialog fileDlg(true, _T("jpg"), _T("*.jpg"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("jpg Files (*.jpg)|*.jpg|png File(*.png)|*.png|All File (*.*)|*.*||"), NULL);
	if (fileDlg.DoModal() == IDOK)    //弹出对话框  
	{
		gReadFilePathName = fileDlg.GetPathName();//得到完整的文件名和目录名拓展名  
		filename = fileDlg.GetFileName();
	}
	ImgprcTask mtask;
	USES_CONVERSION;
	string imgpath = CT2A(gReadFilePathName.GetBuffer(0));
	strcpy_s(mtask.m_chsFileName, 512, imgpath.c_str());
	mtask.m_dwTimeTrigger = clock();
	mtask.m_isTopViewImage = 1;
	mtask.m_image_num = 1;


	PushAnImageProcessTask(mtask);
}





