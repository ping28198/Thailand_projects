// ImgprcOCRInstanceView.h : CImgprcOCRInstanceView ��Ľӿ�
//


#pragma once
#include "afxwin.h"
#include "timer.h"
#include "FSock.h"
//#include "IMGSVRDB.H"
#include "ImgprcTask.h"
#include <vector>
#include "MPFCommuication.h"
#include <random>
#include "logger.h"
#include "tag_detect.h"
#define MAIL_MAX_NUM 500
#define DATABASE_NAME_LEN 100
class CImgprcOCRInstanceView : public CFormView
{
protected: // �������л�����
	CImgprcOCRInstanceView();
	DECLARE_DYNCREATE(CImgprcOCRInstanceView)

public:
	enum{ IDD = IDD_IMGPRCOCRINSTANCE_FORM };

// ����
public:
	CImgprcOCRInstanceDoc* GetDocument() const;

// ����
public:

// ��д
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual void OnInitialUpdate(); // ������һ�ε���

// ʵ��
public:
	virtual ~CImgprcOCRInstanceView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// ���ɵ���Ϣӳ�亯��
protected:
	DECLARE_MESSAGE_MAP()
public:
	default_random_engine e;
	MPFCommuication m_MachineComm;

//	CEdit m_crlTaskCnt;
	// ������������
	CEdit m_crlTotal;
	// ��ȡ�ļ���
	CEdit m_crlReadImg;
	CEdit m_crlRestTask;
	int m_valueRestTask;
	CEdit m_crlProcessImg;
	CEdit m_crlTagOCROK;
	CEdit m_crlObrOK;
	CEdit m_crlSocket;
	// ���ݿ�����
	CEdit m_crlDb;

	//��Ϣ��ʾ
	CEdit m_crlMain;
	CEdit m_crlMsg;
	DisplayRegion m_Region[2];

	//���񻺴�
	std::vector<ImgprcTask> m_Tasks;

	VCS_RES m_VcsRes;

	OcrAlgorithm_config m_ocrConifg;
	//tesseract::TessBaseAPI m_Tess;

	//���ݿ�

	//IMGSVRDB m_IMGSVRDB;
	
	//�ж�
	Timer m_MMT1;	//100ms
	Timer m_MMT2;	//200ms ɾ��
	UINT m_MMT_ID;	//ʱ���жϵ�ID
	
	//�����ʱ��
	DWORD m_dwLastIMAGE;

	FSock m_ClientSock;						//����ͨѶ server
	BOOL m_bClientSockState;				//����״̬
	DWORD m_dwTimeCount;
	DWORD m_dwTcp2LastSendToNow;
	DWORD m_dwTimesTcp2Retry1;
	CString m_cstrIP;
	CString m_cstrSeverIP;
	int m_cstrSeverPort;
	int m_iLocPort;

    CString m_DirA;
	CString m_DirAA;
	CString m_DirAAA;

	//���������


	//��־
	FILE * m_pFileLog;
	FILE * m_pFileSta;

	//����ʼ����ʱ��
	CTime m_ctime ;					//����ʱ��

	int m_iMaxImageNum;					//һ���������ͼ������

	int m_iRank;					//tx20171201

	BYTE m_OCRID; //�����������
	//ͳ��
	ImgprcStatistics m_Sta;
	ImgprcError m_Err;

	bool m_bShowMain;
	//·��
	CString m_cstrLogDir;			//��־�ļ��ļ���
	CString m_cstrImgDir;
	CString m_cstrProcessedDir;
	float m_tagDetectConfidence;
	//ʱ��
	CString m_cstrPreDay;	
	//������
	void ImagesProcessing(OcrAlgorithm_config* pConfig);//tx 
	// ��ʱ����������Ϣ
	void SendNetMsg(void);
	// ���ڴ����·���ͳ������д������stalog��
	int WriteStatoLog(void);
	// //����Imgserver����Ϣ
	void TCPClientTask(void);
	// ������Ϣ
	void ProcessOneRMes(BYTE *mbMesPos);
	// //����IMGSVR������
//	void SendImgprcResult(ImgprcTask   * Task);
	// ����ǰ����
	void CodeMessage(unsigned char * uchMsg, unsigned char * uchNew, int  len, int & newLen);
	// ����ȷ�����ݿ�����

	void ClientSocketonTimer(void);

	void CheckConnection(void);

	//�����������һ�������������
	void PushAnImageProcessTask(ImgprcTask mTask);

	//�Ӷ����л�ȡһ��δ���������
	int GetAnImageProcessTask(ImgprcTask &mTask);

	//����Ѿ���ɵ�����
	int OnAnImageProcessTaskOver(ImgprcTask &mTask);

	//��ȡһ���Ѿ���ɵ�����
	int GetAnFinishedImageProcessTask(ImgprcTask &mTask);

	//�Ӷ�����ɾ��һ������,����������
	int DelAnImageProcessTask(ImgprcTask &mTask);

	//��ȡ������δ��ɵ��������,�������ڽ��е�
	int GetUnderProcessTaskNum();

	//��ȡ���еȴ����������
	int GetWaitPackageTasksAll(std::vector<ImgprcTask> &wTasks);

	//���ݵ�ǰ���񣬻�ȡ��ȴ�������ֵ�����,ͬһ��������
	int GetWaitPackageTasks(ImgprcTask &mTask,std::vector<ImgprcTask> &wTasks);

	//�ı������ ��������, ͬʱ��λ����״̬Ϊδ����
	int ChangeTasksProcessType(ImgprcTask &mTask, BYTE old_type, BYTE new_type);


	int GenerateMsgData2ImageServer(BYTE *pData, ImgprcTask &mTask);
	int SplitTask(ImgprcTask &mTask, std::vector<ImgprcTask> &subTasks);

	//�ϲ��������Ѿ���ɵ�������
	int MergeHomoTasks(std::vector<ImgprcTask> &subTasks, ImgprcTask &mTask);


	// ���ݿ�
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	// ����ʾ��������ʾ����
	void ToDisplay(BYTE nRegionNo, CString cstrMsg);

	// ��������
	int Reboot(void);
	// �رյ���
	int ShutDown(void);

		// �������ļ�
	int ReadINI(void);
	//tx20171004

	char m_chsDBName[DATABASE_NAME_LEN];
	char m_chsDBIP[DATABASE_NAME_LEN];
	char m_chsDBUser[DATABASE_NAME_LEN];
	char m_chsDBPassword[DATABASE_NAME_LEN];


	void SendImgprcResultMsg(ImgprcTask * pTask);
	void TestTask();
	CEdit m_crlCopyImg;
	afx_msg void OnTestSendreq();
	// ����Tasks��ǰ���пɷ����������Index
	int GetTasksIndex(int  iTrayno);

	afx_msg void OnDebugeBt(); //���԰�ť��Ӧ����

};



#ifndef _DEBUG  // ImgprcOCRInstanceView.cpp �еĵ��԰汾
inline CImgprcOCRInstanceDoc* CImgprcOCRInstanceView::GetDocument() const
   { return reinterpret_cast<CImgprcOCRInstanceDoc*>(m_pDocument); }
#endif

