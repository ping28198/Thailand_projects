// ImgprcOCRInstanceView.cpp : CImgprcOCRInstanceView ���ʵ��
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

// int nRes;//�ж������0�����ʼ���1��ȷ����2�����ʼ�
// int nType;//���������ʼ��Ͳ�ȷ���ʼ�=0�����ڿ����ʼ�����ʾ��������

//����ֵ=0�������أ�<0�쳣����
//extern "C" __declspec(dllexport) int ImageFilte( const char *input_img_file,const char *output_img_file, int& nRes,int& nType,int iRank);

void CALLBACK TimeFunctionMsg(PVOID lpParam, BOOLEAN TimerOrWaitFired);
void CALLBACK TimeFunctionImg(PVOID lpParam, BOOLEAN TimerOrWaitFired);


#define IADM_TIMER_INTERVAL	50 //��ý���жϼ��
#define IADM_TIMES_COMM_RETRY 3000
#define IADM_TIMER_RESOLUTION 5  // ��ý��ʱ�Ӿ��ȣ���λ�����룩


//#define OCR_VIEW_DEBUG
//
//#ifndef OCR_VIEW_DEBUG
//Logger logger("./log", LogLevelMid);
//#else
//Logger logger("./log", LogLevelAll);
//#endif // !1


Logger *plogger=NULL;


// CImgprcOCRInstanceView

IMPLEMENT_DYNCREATE(CImgprcOCRInstanceView, CFormView)

BEGIN_MESSAGE_MAP(CImgprcOCRInstanceView, CFormView)
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_COMMAND(ID_TEST_SENDREQ, &CImgprcOCRInstanceView::OnTestSendreq)
	ON_COMMAND(ID_DEBUG_BT, &CImgprcOCRInstanceView::OnDebugeBt)
END_MESSAGE_MAP()

// CImgprcOCRInstanceView ����/����

CRITICAL_SECTION crt_section;


// �����߳����ڴ���ͼ��
extern UINT ImageProcessThread(LPVOID pParam)
{
	ImgprcTask mTask;
	CImgprcOCRInstanceView *pParent = (CImgprcOCRInstanceView*)pParam;
	OcrAlgorithm_config mconfig = pParent->m_ocrConifg;

	mconfig.pLogger = plogger;

	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0; // ɾ���ļ�����ֻ���·���ִ�
	CString str_url = szFilePath;  // ����str_url==e:\program\Debug\  //
	USES_CONVERSION;
	std::string exe_dir = T2A(str_url.GetBuffer());

	//mconfig.ORB_template_img1_path = exe_dir + mconfig.ORB_template_img1_path;
	//mconfig.ORB_template_img2_path = exe_dir + mconfig.ORB_template_img2_path;
	//mconfig.handwrite_ref_img1_path = exe_dir + mconfig.handwrite_ref_img1_path;//

	//����tag detector����
	tag_detector detecor;
	std::string model_path = mconfig.detect_model_file_path;
	float confidence_threshold = mconfig.TagDetectConfidence;
	int max_instance_per_class = mconfig.max_instance_per_class;
	int res = detecor.initial(model_path, confidence_threshold, max_instance_per_class);
	if (res==0)
	{
		plogger->TraceError("Fail to initial tag detection model!");
	}
	mconfig.pTagDetector = &detecor;


	//����ƥ������
	res = mconfig.match_data.getMatchDataFromImg_tagRotate_SIFT(mconfig.ORB_template_img1_path, mconfig.ORB_template_img2_path);
	if (res == 0)
	{
		plogger->TraceError("Fail to load tag match image!");
	}
	res = mconfig.match_data.getMatchDataFromImg_handwrite_addr(mconfig.handwrite_ref_img1_path);
	if (res == 0)
	{
		plogger->TraceError("Fail to load handwritten box match image!");
	}

	//����ocr����
	tesseract::TessBaseAPI mTess;
	if (mTess.Init(mconfig.tess_data_path.c_str(), "eng"))
	{
		plogger->TraceError("Fail to load OCR engine!");
		return 0;
	}
	mconfig.pTess = &mTess;


	//������д����ʶ����
	HWDigitsRecog hwDigitRec;
	std::string handwrite_model_path =mconfig.handwrite_ocr_model_path;
	res = hwDigitRec.initial(handwrite_model_path);
	if (res == 0)
	{
		plogger->TraceError("Fail to load handwritten digit recognition model!");
	}
	mconfig.pHWDigitsRecog = &hwDigitRec;

	
	plogger->TraceKeyInfo("Success to start work thread!");
	//ѭ�����
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
	// TODO: �ڴ˴���ӹ������
	m_bClientSockState = 0;				//����״̬
	m_dwTimeCount=0;
	m_dwTcp2LastSendToNow=0;
	m_dwTimesTcp2Retry1=0;
	m_bShowMain=true;
	m_bClientSockState = 0;	
	m_iRank =1;

}

CImgprcOCRInstanceView::~CImgprcOCRInstanceView()
{
	if (plogger!=NULL)
	{
		delete plogger;
	}
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
	// TODO: �ڴ˴�ͨ���޸�
	//  CREATESTRUCT cs ���޸Ĵ��������ʽ

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
	USES_CONVERSION;


	//����log���������ã�
	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0; // ɾ���ļ�����ֻ���·���ִ�
	CString str_url = szFilePath;  // ����str_url==e:\program\Debug\  //
	CStringToCharArray(str_url + m_cstrLogDir, chsTemp);
	sprintf_s(fn,"%sOCR%dLog%04d%02d%02d.txt",chsTemp,m_iLocPort,m_ctime.GetYear(),m_ctime.GetMonth(),m_ctime.GetDay());
	m_pFileLog=fopen(fn,"a+");


	unsigned int nchLen;

	nchLen = WideCharToMultiByte(CP_ACP,0,m_cstrSeverIP,m_cstrSeverIP.GetLength(),NULL,0,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,m_cstrSeverIP,m_cstrSeverIP.GetLength(),m_MachineComm.m_chsIPAddress,nchLen,NULL,NULL);
	m_MachineComm.m_chsIPAddress[nchLen]=NULL;
	m_MachineComm.m_chsDstPort = m_cstrSeverPort;

//	strcpy_s(m_MachineComm.m_chsIPAddress,m_cstrIADMIP.GetBuffer(0)); //tX
	strcpy_s(m_MachineComm.m_chsPCName,"ImageServer");
	strcpy_s(m_MachineComm.m_chsShortName,"ImageServer");
	m_MachineComm.ClearMachineBuf();

	//����������Ϣ
	plogger->TraceKeyInfo((std::string("ImageServerIP:") + m_MachineComm.m_chsIPAddress + ":" + to_string(m_MachineComm.m_chsDstPort)).c_str());
	plogger->TraceKeyInfo((std::string("LocalIP:") + T2A(m_cstrIP.GetBuffer())+":"+to_string(m_iLocPort)).c_str());
	

	//����ocrConfig
	//TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0; // ɾ���ļ�����ֻ���·���ִ�
	str_url = szFilePath;  // ����str_url==e:\program\Debug\  //
	
	//����ȫ·��
	std::string exe_dir = T2A(str_url.GetBuffer());
	m_ocrConifg.ORB_template_img1_path = exe_dir + m_ocrConifg.ORB_template_img1_path;
	m_ocrConifg.ORB_template_img2_path = exe_dir + m_ocrConifg.ORB_template_img2_path;
	m_ocrConifg.handwrite_ref_img1_path = exe_dir + m_ocrConifg.handwrite_ref_img1_path;//
	m_ocrConifg.detect_model_file_path = exe_dir + m_ocrConifg.detect_model_file_path;
	m_ocrConifg.handwrite_ocr_model_path = exe_dir + m_ocrConifg.handwrite_ocr_model_path;
	m_ocrConifg.tess_data_path = exe_dir + "tessdata";

	//log�㷨��Ϣ
	plogger->TraceKeyInfo(("Tag detect confidence threshold:" + std::to_string(m_ocrConifg.TagDetectConfidence)).c_str());
	plogger->TraceKeyInfo(("Max homo class tag in an image:" + std::to_string(m_ocrConifg.max_instance_per_class)).c_str());
	plogger->TraceKeyInfo(("Handwrite digits confidence threshold:" + std::to_string(m_ocrConifg.HandwriteDigitsConfidence)).c_str());
	//plogger->TraceKeyInfo(("OCRģ��·����" + tessDataDir).c_str());
	plogger->TraceKeyInfo(("Tag detect model path:" + m_ocrConifg.detect_model_file_path).c_str());
	plogger->TraceKeyInfo(("Handwrite digits detect model path:" + m_ocrConifg.handwrite_ocr_model_path).c_str());
	plogger->TraceKeyInfo(("Handwrite address range reference image path:" + m_ocrConifg.handwrite_ref_img1_path).c_str());
	plogger->TraceKeyInfo(("Tesseract data path:" + m_ocrConifg.tess_data_path).c_str());

	if (m_ocrConifg.is_test_model==1)
	{
		plogger->TraceKeyInfo("Run as test mode!");
	}

	plogger->TraceKeyInfo("Starting threading 1...");
	CWinThread *pTheard_process1;
	pTheard_process1 = AfxBeginThread(ImageProcessThread, this);
	if (pTheard_process1 == NULL)
	{
		plogger->TraceError("Start threading 1 error");
		MessageBox(_T("start threading 1 error"));
	}
	Sleep(100);

	plogger->TraceKeyInfo("Starting threading 2...");
	CWinThread *pTheard_process2;
	pTheard_process2 = AfxBeginThread(ImageProcessThread, this);
	if (pTheard_process2 == NULL)
	{
		plogger->TraceError("Start threading 2 error");
		MessageBox(_T("start threading 2 error"));
	}
	Sleep(100);

	plogger->TraceKeyInfo("Starting threading 3...");
	CWinThread *pTheard_process3;
	pTheard_process3 = AfxBeginThread(ImageProcessThread, this);
	if (pTheard_process3 == NULL)
	{
		plogger->TraceError("Start threading 3 error");
		MessageBox(_T("start threading 3 error"));
	}
}


// CImgprcOCRInstanceView ���

#ifdef _DEBUG
void CImgprcOCRInstanceView::AssertValid() const
{
	CFormView::AssertValid();
}

void CImgprcOCRInstanceView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CImgprcOCRInstanceDoc* CImgprcOCRInstanceView::GetDocument() const // �ǵ��԰汾��������
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CImgprcOCRInstanceDoc)));
	return (CImgprcOCRInstanceDoc*)m_pDocument;
}
#endif //_DEBUG


// CImgprcOCRInstanceView ��Ϣ�������

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
////		ToDisplay(0,_T("ͳ�����㣡"));
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

	//�����������
	ClientSocketonTimer();

	//�����ݿ�����
	//CheckConnection();

	int unprocessdnum = GetUnderProcessTaskNum();

	cstrNewLine.Format(_T("%d"), unprocessdnum);

	m_crlRestTask.SetWindowTextW(cstrNewLine);


	//����������Ŀ
	m_crlTotal.GetWindowTextW(cstrLine);
	cstrNewLine.Format(_T("%lu"), m_Sta.m_dwTotal);
	if (cstrNewLine != cstrLine)
	{
		m_crlTotal.SetWindowTextW(cstrNewLine);
	}

	//tagOCR����

	m_crlTagOCROK.GetWindowTextW(cstrLine);
	cstrNewLine.Format(_T("%lu"), m_Sta.m_dwTagOcrOKCount);
	if (cstrNewLine != cstrLine)
	{
		m_crlTagOCROK.SetWindowTextW(cstrNewLine);
	}


	//ȡͼ����
	//m_crlReadImg.GetWindowTextW(cstrLine);
	//cstrNewLine.Format(_T("%lu"), m_Sta.m_dwReadImgok);
	//if(cstrNewLine!=cstrLine)
	//{
	//	m_crlReadImg.SetWindowTextW(cstrNewLine);
	//}
	//����ͼ������
	//m_crlProcessImg.GetWindowTextW(cstrLine);
	//cstrNewLine.Format(_T("%lu"), m_Sta.m_dwPrcImgok);
	//if(cstrNewLine!=cstrLine)
	//{
	//	m_crlProcessImg.SetWindowTextW(cstrNewLine);
	//}
	//ʶ����������

	m_crlObrOK.GetWindowTextW(cstrLine);
	cstrNewLine.Format(_T("%lu"), m_Sta.m_dwObrOK);
	if(cstrNewLine!=cstrLine)
	{
		m_crlObrOK.SetWindowTextW(cstrNewLine);
	}


		//��д����

	m_crlCopyImg.GetWindowTextW(cstrLine);
	cstrNewLine.Format(_T("%lu"), m_Sta.m_dwCopyImgok);
	if(cstrNewLine!=cstrLine)
	{
		m_crlCopyImg.SetWindowTextW(cstrNewLine);
	}

	m_crlSocket.GetWindowTextW(cstrLine);
	if(m_bClientSockState)
	{
		cstrNewLine.Format(_T("OCR:%d connected with ImageServer %s"),m_iLocPort,m_cstrSeverIP);
	}
	else
	{
		cstrNewLine.Format(_T("OCR:%d disconnect with ImageServer %s"),m_iLocPort,m_cstrSeverIP);
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
	//	sTemp = CString(_T("���ݿ�����ʧ�ܣ�"))+CString((char *)m_IMGSVRDB.m_bstrErrDsp);
	//	ToDisplay(0,sTemp);
	//}
	//else
	//{
	//	ToDisplay(0,CString(_T("���ݿ����ӳɹ���")));
	//}
	EnumLogLevel loglev;
	switch (m_LogLevel)
	{
	case 0:
		loglev = LogLevelAll;
		break;
	case 1:
		loglev = LogLevelMid;
		break;
	case 2:
		loglev = LogLevelNormal;
		break;
	case 3:
		loglev = LogLevelStop;
		break;
	default:
		loglev = LogLevelAll;
		break;
	}
	plogger = new Logger("./log",loglev);

	//int res = SF_InfoInit(&m_VcsRes);

	//if ( 0 != res )
 //   {
 //       if ( -10 == res )
 //       {
 //           // ����ֵΪ-10��ʾ������Ȩ����ʼ��ʧ��
 //           sTemp = CString(_T("�˵���δ��Ȩ��\n" ));
 //       }
 //       else
 //       {
 //           sTemp = CString(_T( "��ʼ��ʧ�ܡ�\n" ));
 //       }


 //   }
 //   else
 //   {
 //       sTemp = CString(_T("��ʼ���ɹ���\n" ));
 //   }


	ToDisplay(0,sTemp);


	CTime ct =CTime::GetCurrentTime();
	m_cstrPreDay = 	ct.Format("%Y-%m-%d");

	InitializeCriticalSection(&crt_section);



		//������ʾ��ʱ��
	SetTimer(1, IADM_TIMER_INTERVAL,NULL);  // Create display time 

	//����ͼ��
	m_MMT1.AddTimerListener((LPTIMECALLBACK)TimeFunctionMsg);
	m_MMT1.Start((DWORD_PTR)this);


	m_MMT2.AddTimerListener((LPTIMECALLBACK)TimeFunctionImg);
	m_MMT2.Start((DWORD_PTR)this);


	return 0;

}

// ����ʾ��������ʾ����
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

//��Ϣ����
void CALLBACK TimeFunctionMsg(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	CImgprcOCRInstanceView * pView = (CImgprcOCRInstanceView *)lpParam;
	pView->TCPClientTask();
}

//׼����Ҫ���͵���Ϣ
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
	BYTE pBuffer[512] = { 0 };//��������
	BYTE pRawBuffer[512] = { 0 };//�洢�Ӵ��������ݣ��Ѽ���
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
		info_str.Format(_T("%s(%d)"), A2T(mTask.m_chsOcrPostcode),mTask.m_postcodeNum);
		info_str = proc_type + _T(":") + info_str;
		pView->ToDisplay(1, info_str);
		int a = pView->DelAnImageProcessTask(mTask);
		//sprintf((char*)pBuffer, "ɾ����%d������", a);
		//plogger->TraceInfo((char*)pBuffer);
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


	// //����Imgserver����Ϣ
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
	//ToDisplay(0,L"��ʱ����Ӧ��");
	// Process receive socket message
	if (m_bClientSockState == 1)
	{
		//��socket��ȡ����
		size_t received_count = m_ClientSock.GetReceivedData(m_MachineComm.m_uchsRawData, IADM_MES_MAX_LEN * 2);
		for (size_t i=0;i<received_count;i++)
		{
			m_MachineComm.AnalyseOneByte(m_MachineComm.m_uchsRawData[i]);
		}
		//plogger->TraceInfo("�յ���һ����Ϣ");
		//������Ϣ
		BYTE * messageDataBuffer = m_MachineComm.GetOneMessage();
		if (messageDataBuffer!=NULL)
		{
			ProcessOneRMes(messageDataBuffer);
			//plogger->TraceInfo("������һ����Ϣ");
		}
		
	}
}


//�������񣬲��������������
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
		bType=*(p+ posnow++); //��һλΪmsg length
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
			
			SplitTask(Task, subTasks); //�����ж���������������
			for (i=0;i<subTasks.size();i++)
			{
				PushAnImageProcessTask(subTasks[i]);
			}
			m_Sta.m_dwTotal++;
			//plogger->TraceInfo("������һ������");
			break;
		case MPF_MSG_IMG2OCR_FUNC:
			break;

		}
	}
	catch(...)
	{
		sTemp.Format(_T("ProcessOneRMes():Error ImageServer RevMesPos"), mbMesPos);
		ToDisplay(0, sTemp);
	}
}

// ��������
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

// �رյ���
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







	// �������ļ�
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

	//����
	key =CString(_T("ImgDir")); 
	GetPrivateProfileStringW(app,key,LPCWSTR("0"),wchsTemp,IS_MES_MAX_LEN,cstrIniPath); 
	m_cstrImgDir= wchsTemp;
	//����
	key =CString(_T("ProcessedDir")); 
	GetPrivateProfileStringW(app,key,LPCWSTR("D:\\ImageForVideo\\"),wchsTemp,IS_MES_MAX_LEN,cstrIniPath); 
	m_cstrProcessedDir= wchsTemp;



	//����IP��ַ
	key =CString(_T("LocIP"));
	GetPrivateProfileStringW(app,key,LPCWSTR("0"),wchsTemp,IS_MES_MAX_LEN,cstrIniPath); 
	m_cstrIP= wchsTemp;
	
	//���ض˿ں�
	key  = CString(_T("LocPort"));
	m_iLocPort = GetPrivateProfileIntW(app,key,10001,cstrIniPath);

	//��־��¼����
	key = CString(_T("LogLevel"));
	m_LogLevel = GetPrivateProfileIntW(app, key, 0, cstrIniPath);

	//����ocrID��
	key = CString(_T("OCRID"));
	m_OCRID = GetPrivateProfileIntW(app, key, 1, cstrIniPath);

	//��־Ŀ¼
	key =CString(_T("LogPath"));
	GetPrivateProfileStringW(app,key,LPCWSTR(_T("log\\")),wchsTemp,IS_MES_MAX_LEN,cstrIniPath);
	m_cstrLogDir = wchsTemp;

	//ͼ�������IP��ַ
	key =CString(_T("ImageServerIP"));
	GetPrivateProfileStringW(app,key,LPCWSTR("192.168.1.30"),wchsTemp,IS_MES_MAX_LEN,cstrIniPath); 
	m_cstrSeverIP= wchsTemp;

	//ͼ��������˿ں�
	key = CString(_T("ImageServerPort"));
	m_cstrSeverPort = GetPrivateProfileIntW(app, key, 9999, cstrIniPath);

	//��ǩ������Ŷ���ֵ
	key = CString(_T("TagDetectConfidence"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0.3"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.TagDetectConfidence = _ttof(wchsTemp);

	//��д����ʶ�����Ŷ���ֵ
	key = CString(_T("HWDigits_confidence"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0.85"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.HandwriteDigitsConfidence = _ttof(wchsTemp);

	//�Ƿ����ǩ
	key = CString(_T("Run_OCR_on_standard_tag"));
	GetPrivateProfileStringW(app, key, LPCWSTR("1"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.Run_OCR_on_standard_tag = _ttoi(wchsTemp);

	//�Ƿ�����д��
	key = CString(_T("Run_OCR_on_handwrite_box"));
	GetPrivateProfileStringW(app, key, LPCWSTR("1"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.Run_OCR_on_handwrite_box = _ttoi(wchsTemp);

	//ÿһ��ͼƬͬ����ǩ�������
	key = CString(_T("InstanceNumPerClass"));
	GetPrivateProfileStringW(app, key, LPCWSTR("1"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.max_instance_per_class = _ttoi(wchsTemp);


	//��ǩ1ƥ����ͼ·��
	USES_CONVERSION;
	key = CString(_T("ORB_Template_img1"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.ORB_template_img1_path = CT2A(wchsTemp);

	//��ǩ2ƥ����ͼ·��
	key = CString(_T("ORB_Template_img2"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.ORB_template_img2_path = CT2A(wchsTemp);

	//��д��ƥ����ͼ·��
	key = CString(_T("HW_Template_img1"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.handwrite_ref_img1_path = CT2A(wchsTemp);

	//��ǩ���ģ��·��
	key = CString(_T("Detect_model_path"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.detect_model_file_path = CT2A(wchsTemp);

	//��д����ʶ��ģ��·��
	key = CString(_T("HWDigits_model_path"));
	GetPrivateProfileStringW(app, key, LPCWSTR("0"), wchsTemp, IS_MES_MAX_LEN, cstrIniPath);
	m_ocrConifg.handwrite_ocr_model_path = CT2A(wchsTemp);

	//����ģʽ
	key = CString(_T("TestMode"));
	m_ocrConifg.is_test_model = GetPrivateProfileIntW(app, key, 0, cstrIniPath);



	//��������
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

//���������������
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
	bool isCacheImageFile = false;
	//std::string local_img_dir = CT2A(m_cstrImgDir);
	if (mtask.m_chsLocalImgPath[0]==0)
	{
		re = mtask.CopyImageFilesFromServer(m_MachineComm.m_chsIPAddress, m_cstrImgDir, plogger, &e, isCacheImageFile);
	}
	else
	{
		re = 1;
	}
	if (re != 0)
	{
		mtask.PorcessTask(pConfig, plogger);//���������������
	}
	else
	{
		//plogger->TraceWarning("Copy image file from Image server Error!");
		mtask.m_resultState = 11;
		OnAnImageProcessTaskOver(mtask); //������һ������
		return;
	}
	
	OnAnImageProcessTaskOver(mtask); //������һ������
	if (mtask.m_image_total_num == 1)
	{
		if (isCacheImageFile)
		{
			mtask.DeleteLocalCacheFile(plogger);
		}
		return;
	}

	//���������
	std::vector<ImgprcTask> waitPackageTasks;
	re = GetWaitPackageTasks(mtask, waitPackageTasks);
	if (re >= mtask.m_image_total_num)
	{
		MergeHomoTasks(waitPackageTasks, mtask);
		BYTE next_type = mtask.GetNextProcessType(pConfig);
		//�������û�н�������û���ҵ��ʱ࣬������һ������
		if (next_type != ST_TASK_PROC_OVER && mtask.m_postcodeNum == 0)
		{
			ChangeTasksProcessType(mtask, mtask.m_processType, next_type);//������������
		}
		else
		{
			DelAnImageProcessTask(mtask);//ɾ�����д��������
			//mtask.m_processType = ST_TASK_PROC_OVER;
			mtask.m_isProcessing = ST_TASK_PROCESSED;
			PushAnImageProcessTask(mtask);//������������������
			if (isCacheImageFile)
			{
				for (int i = 0; i < waitPackageTasks.size(); i++)
				{
					waitPackageTasks[i].DeleteLocalCacheFile(plogger);
				}
			}

		}
	}


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
	pData[posnow++] = m_OCRID; //����id
	pData[posnow++] = mTask.m_resultState; //���״̬
	pData[posnow++] = mTask.m_PostcodeIndextImage; //�ʱ�����ͼƬ������
	//��������ֵ
	pData[posnow++] = strlen(mTask.m_chsBarcode) % 256; // �������256������
	memcpy(pData+posnow, mTask.m_chsBarcode, strlen(mTask.m_chsBarcode) % 256);
	posnow += strlen(mTask.m_chsBarcode) % 256;
	//�ʱ���Ϣ
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

	//ʣ������
	pData[posnow++] = unProcessNum % 256;
	pData[posnow] = 0;

	//char a[64] = { 0 };
	//sprintf_s(a, "�ŶӵȺ�������%d", unProcessNum);
	//plogger->TraceInfo(a);
	return posnow;
}



int CImgprcOCRInstanceView::SplitTask(ImgprcTask &mTask, std::vector<ImgprcTask> &subTasks)
{
	
	if (mTask.m_image_total_num <= 0)
	{
		plogger->TraceWarning("Received one task, but image quantity is zero!");
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
		plogger->TraceWarning("Image path number is less than the number specified!");
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
		subTask.m_image_num = 1;//�������ͼƬ����Ĭ��Ϊ1
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
			if (mTask.m_postcodeNum!=0)
			{
				strcat(mTask.m_chsOcrPostcode, ";");
			}
			mTask.m_PostcodeIndextImage = subTasks[i].m_index_sub_image + 1;
			strcat(mTask.m_chsOcrPostcode, subTasks[i].m_chsOcrPostcode);
			//strcat(mTask.m_chsOcrPostcode, ";");
			mTask.m_postcodeNum += subTasks[i].m_postcodeNum;
		}
	}
	if (mTask.m_postcodeNum == 0) //ʶ��0���ʱ�
	{
		mTask.m_resultState = 12;
	}
	else if (mTask.m_postcodeNum == 1) //��ȷʶ��1���ʱ�
	{
		mTask.m_resultState = 1;
	}
	else if (mTask.m_postcodeNum > 1)
	{
		mTask.m_resultState = 13;//����ʱ�
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
		fprintf_s(m_pFileSta,"\n\n��ʼʱ��:%s\n",chArrays);
		fflush(m_pFileSta);
		//socket
		this->m_crlSocket.GetWindowTextW(sTemp);
		CStringToCharArray(sTemp,chArrays);		
		GetDlgItem(IDC_STATIC_SOCKET)->GetWindowTextW(cstrCaption);
		CStringToCharArray(cstrCaption, chCaption);
		fprintf_s(m_pFileSta,"%s:%s\n",chCaption,chArrays);
		fflush(m_pFileSta);

		//������ʼ�����
		this->m_crlTotal.GetWindowTextW(sTemp);
		CStringToCharArray(sTemp,chArrays);
		GetDlgItem(IDC_STATIC_TOTAL)->GetWindowTextW(cstrCaption);
		CStringToCharArray(cstrCaption, chCaption);
		fprintf_s(m_pFileSta,"%s:%s\n",chCaption,chArrays);
		fflush(m_pFileSta);

		//��ͼ		
		//this->m_crlReadImg.GetWindowTextW(sTemp);
		//CStringToCharArray(sTemp,chArrays);
		//GetDlgItem(IDC_STATIC_READIMG)->GetWindowTextW(cstrCaption);
		//CStringToCharArray(cstrCaption, chCaption);
		//fprintf_s(m_pFileSta,"%s:%s\n",chCaption,chArrays);
		//fflush(m_pFileSta);

		//����ͼ��	
		//this->m_crlProcessImg.GetWindowTextW(sTemp);
		//CStringToCharArray(sTemp,chArrays);
		//GetDlgItem(IDC_STATIC_PRCIMG)->GetWindowTextW(cstrCaption);
		//CStringToCharArray(cstrCaption, chCaption);
		//fprintf_s(m_pFileSta,"%s:%s\n",chCaption,chArrays);
		//fflush(m_pFileSta);

		//��ȡͼ���XML	
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



		ToDisplay(0,CString(_T("ͳ��ֵ�Ѿ�д�뵱��Sta��־��")));
		fclose(m_pFileSta);
		m_pFileSta = NULL;
		return 1;
	}
	else
	{
		ToDisplay(0,CString(_T("ͳ��ֵδ��д�뵱��Sta��־��")));
		return 0;
	}
}



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

		//������Ϣ��IADM
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
			/////2012-5-21  ֱ�ӷ���Ϣ�����ǵȻ��������ٷ���Ϣ//////
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
	uchMsg[2] = MPF_MSG_OCR2IMG_RESULT;	//ͼ������͸�IADM
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
		temp.Format(_T("���ͳɹ�"));
	}
	else
	{
		temp.Format(_T("����ʧ��Err=%d"),err);
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
	uchMsg[2] = 0xC5;	//ͼ������͸�IADM

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
		temp.Format(_T("���ͳɹ�"));
	}
	else
	{
		temp.Format(_T("����ʧ��Err=%d"),err);
	}
	sTemp += temp;
	ToDisplay(1, sTemp);
}
// ����ǰ����
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
	//uchMsg[2] = 0xC4;	//ͼ������͸�IADM

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
	uchMsg[2] = 0xC2;	//����VCS

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

// ����Tasks��ǰ���пɷ����������Index
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
	if (fileDlg.DoModal() == IDOK)    //�����Ի���  
	{
		gReadFilePathName = fileDlg.GetPathName();//�õ��������ļ�����Ŀ¼����չ��  
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





