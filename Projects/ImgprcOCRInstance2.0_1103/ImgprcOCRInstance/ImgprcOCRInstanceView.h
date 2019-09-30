// ImgprcOCRInstanceView.h : CImgprcOCRInstanceView 类的接口
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
protected: // 仅从序列化创建
	CImgprcOCRInstanceView();
	DECLARE_DYNCREATE(CImgprcOCRInstanceView)

public:
	enum{ IDD = IDD_IMGPRCOCRINSTANCE_FORM };

// 属性
public:
	CImgprcOCRInstanceDoc* GetDocument() const;

// 操作
public:

// 重写
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void OnInitialUpdate(); // 构造后第一次调用

// 实现
public:
	virtual ~CImgprcOCRInstanceView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()
public:
	default_random_engine e;
	MPFCommuication m_MachineComm;

//	CEdit m_crlTaskCnt;
	// 接受任务数量
	CEdit m_crlTotal;
	// 读取文件数
	CEdit m_crlReadImg;
	CEdit m_crlRestTask;
	int m_valueRestTask;
	CEdit m_crlProcessImg;
	CEdit m_crlTagOCROK;
	CEdit m_crlObrOK;
	CEdit m_crlSocket;
	// 数据库连接
	CEdit m_crlDb;

	//信息显示
	CEdit m_crlMain;
	CEdit m_crlMsg;
	DisplayRegion m_Region[2];

	//任务缓存
	std::vector<ImgprcTask> m_Tasks;

	VCS_RES m_VcsRes;

	OcrAlgorithm_config m_ocrConifg;
	//tesseract::TessBaseAPI m_Tess;

	//数据库

	//IMGSVRDB m_IMGSVRDB;
	
	//中断
	Timer m_MMT1;	//100ms
	Timer m_MMT2;	//200ms 删除
	UINT m_MMT_ID;	//时间中断的ID
	
	//最长处理时间
	DWORD m_dwLastIMAGE;

	FSock m_ClientSock;						//网络通讯 server
	BOOL m_bClientSockState;				//连接状态
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

	//服务器相关


	//日志
	FILE * m_pFileLog;
	FILE * m_pFileSta;

	//程序开始运行时间
	CTime m_ctime ;					//开机时间

	int m_iMaxImageNum;					//一个任务最多图像数量

	int m_iRank;					//tx20171201

	BYTE m_OCRID; //本补码机器号
	//统计
	ImgprcStatistics m_Sta;
	ImgprcError m_Err;

	bool m_bShowMain;
	//路径
	CString m_cstrLogDir;			//日志文件文件夹
	CString m_cstrImgDir;
	CString m_cstrProcessedDir;
	float m_tagDetectConfidence;
	//时间
	CString m_cstrPreDay;	
	//处理函数
	void ImagesProcessing(OcrAlgorithm_config* pConfig);//tx 
	// 定时发送在线消息
	void SendNetMsg(void);
	// 将在窗体下方的统计数据写到当天stalog中
	int WriteStatoLog(void);
	// //接受Imgserver的消息
	void TCPClientTask(void);
	// 处理消息
	void ProcessOneRMes(BYTE *mbMesPos);
	// //发送IMGSVR处理结果
//	void SendImgprcResult(ImgprcTask   * Task);
	// 发送前编码
	void CodeMessage(unsigned char * uchMsg, unsigned char * uchNew, int  len, int & newLen);
	// 定期确认数据库连接

	void ClientSocketonTimer(void);

	void CheckConnection(void);

	//向队列中推入一个待处理的任务
	void PushAnImageProcessTask(ImgprcTask mTask);

	//从队列中获取一个未处理的任务
	int GetAnImageProcessTask(ImgprcTask &mTask);

	//标记已经完成的任务，
	int OnAnImageProcessTaskOver(ImgprcTask &mTask);

	//获取一个已经完成的任务
	int GetAnFinishedImageProcessTask(ImgprcTask &mTask);

	//从队列中删除一个任务,包括子任务
	int DelAnImageProcessTask(ImgprcTask &mTask);

	//获取队列中未完成的任务个数,包括正在进行的
	int GetUnderProcessTaskNum();

	//获取所有等待打包的任务
	int GetWaitPackageTasksAll(std::vector<ImgprcTask> &wTasks);

	//根据当前任务，获取其等待打包的兄弟任务,同一任务类型
	int GetWaitPackageTasks(ImgprcTask &mTask,std::vector<ImgprcTask> &wTasks);

	//改变任务的 任务类型, 同时复位处理状态为未处理
	int ChangeTasksProcessType(ImgprcTask &mTask, BYTE old_type, BYTE new_type);


	int GenerateMsgData2ImageServer(BYTE *pData, ImgprcTask &mTask);
	int SplitTask(ImgprcTask &mTask, std::vector<ImgprcTask> &subTasks);

	//合并队列中已经完成的子任务
	int MergeHomoTasks(std::vector<ImgprcTask> &subTasks, ImgprcTask &mTask);


	// 数据库
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	// 向显示区放入显示内容
	void ToDisplay(BYTE nRegionNo, CString cstrMsg);

	// 重启程序
	int Reboot(void);
	// 关闭电脑
	int ShutDown(void);

		// 读配置文件
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
	// 计算Tasks当前空闲可放入新任务的Index
	int GetTasksIndex(int  iTrayno);

	afx_msg void OnDebugeBt(); //调试按钮响应函数

};



#ifndef _DEBUG  // ImgprcOCRInstanceView.cpp 中的调试版本
inline CImgprcOCRInstanceDoc* CImgprcOCRInstanceView::GetDocument() const
   { return reinterpret_cast<CImgprcOCRInstanceDoc*>(m_pDocument); }
#endif

