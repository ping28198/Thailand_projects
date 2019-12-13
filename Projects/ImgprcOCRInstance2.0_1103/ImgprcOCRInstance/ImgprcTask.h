#pragma once
#include "OcrAlgorithm.h"
//#include "OBJLOC.h"
#include "opencv/cv.h"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv/highgui.h"
#include "time.h"
#include "logger.h"
//#include "MailChain.h"
#include "tag_detect.h"
using namespace cv;
using namespace std;

#define IMAGE_NOT_EXIST -1
#define IS_BAR_MAX_LEN  51					//条码的最大长度
#define IS_BAR_MAX_NUM	3					//最大条码个数
#define IS_IMAGE_MAX_PATH		512			//文件路径名最大长度
#define IS_OCR_MAX_LENGTH		256			//文件路径名最大长度
#define IS_DISPLAY_BUFFER_ROW 2200			//显示区的最大行数
#define IS_MES_MAX_LEN 512					//消息的最大字符长度
#define IS_POSTCODE_LEN 5					//标准邮编长度
#define IS_POSTCODE_MAX_LEN 512				//存储邮编的缓存最大长度
#define ST_TASK_UNPROCESS 0					//任务处理状态
#define ST_TASK_PROCESSING 1
#define ST_TASK_WAIT_PACKAGE 2
//#define ST_TASK_UNPROCESS_2

#define ST_TASK_PROC_TAG 0		//进行标签ocr
#define ST_TASK_PROC_HWBOX 1		//进行手写框ocr
#define ST_TASK_PROC_UNKNOWN_TAG 2	  //任意标签ocr
#define ST_TASK_PROC_OVER 250     //没有处理任务了

#define ST_TASK_PROCESSED 3
#define IS_MAX_IMG_PER_TASK 6				//每个任务的最大图片数量

#define ST_TASK_RESULT_OK 1
#define ST_TASK_RESULT_NO_IMAGE 11
#define ST_TASK_RESULT_NO_POSTCODE 12
#define ST_TASK_RESULT_MUL_POSTCODE 13




void CStringToCharArray(CString & cstrItem, char * pchs);//将CString转换为Char数组
CString GetNowTime(BYTE bFlag); //将当前时间转换为CString 用bFlag控制类型
class ImgprcTask
{
public:
	ImgprcTask(void);
	~ImgprcTask(void);
	void clear(void);

	/*
	IMAGEID	NUMBER(14)	N	0		
	OCRID	NUMBER(2)	N	0		OCR Produce id
	ENABLE	CHAR(1)	Y	1		bit0: wb loc bit1:obr bit2:ocr
	HASWAYBILL	CHAR(1)	Y	0		0: no waybill 1: one waybill
	DONEPATH	VARCHAR2(512)	Y	''		
	BARCODE	VARCHAR2(50)	Y	''		OBRed by OCRID
	OCRRESULT	VARCHAR2(256)	Y	''		
	PROCESSINGTIME	NUMBER(10)	Y	0		
	CREATETIME	DATE	N			
	*/

public:
	BYTE m_TaskID[6]; //任务id
	DWORD m_dwHImageID;
	DWORD m_dwLImageID;
	int m_iOCRID; //机器编号

	BYTE m_requestType; // 请求类型：=1 OBR拒识无条码，=4 有条码无目的地信息
	BYTE m_isProcessing; //任务处理状态：=0 未处理， = 1 正在处理，=2 等待打包，=3 已经处理。
	BYTE m_processType; //处理类型：=0 标准标签OCR，=1 手写框OCR，=2 任意标签OCR =250 没有任务
	BYTE m_resultState; //补码结果：=1 补码成功， = 11 无图像，= 12 未识别邮编，=13 识别多个邮编
	BYTE m_PostcodeIndextImage; //邮编所在图像索引，未识别为0
	BYTE m_isTopViewImage; //是否是顶视图片，多张图片第一张为顶视图
	int m_iPrcTime; //算法处理时间
	char m_chsIPAddress[16]; //Imgserver的IP地址,暂时不用
	BYTE m_image_total_num; //任务中的总图片数量。
	BYTE m_image_num; //该子任务中的图片数量,默认为1
	BYTE m_index_sub_image;//该子任务图片在总任务总的索引，从0开始。
	char m_chsFileName[IS_IMAGE_MAX_PATH*IS_MAX_IMG_PER_TASK]; //图片地址（服务器）';'分开
	//char m_chsDonePath[IS_IMAGE_MAX_PATH];
	//char m_chsLocalImgDir[IS_IMAGE_MAX_PATH];
	char m_chsLocalImgPath[IS_IMAGE_MAX_PATH*IS_MAX_IMG_PER_TASK];//保存在本地的图片路径';'分开
	BYTE m_Barcode_len;
	char m_chsBarcode[IS_BAR_MAX_LEN]; //服务器发过来的条码（暂时无用，直接返回）
	//char m_chsOCRResult[IS_OCR_MAX_LENGTH];
	char m_chsOcrPostcode[IS_POSTCODE_MAX_LEN]; //存储ocr结果，多个用；分开
	cv::Mat* pSrcMat;
	//BYTE m_image_processed_count; //已经处理的任务计数
	int m_postcodeNum;//=0未获得邮编，=1 获得一个邮编，= 2 获得两个邮编..
	//BYTE m_bFlag;
	//DWORD m_dwTimeCount;
	DWORD m_dwTimeTrigger; //用于内部辨别任务
	//DWORD m_dwImsQueTime;
	//int nXrayReturn;
	//int nXrayType;
	//float m_confidenceThreshold;
	//WORD Box1_TBLR[4];
	//WORD Box2_TBLR[4];
	ImgprcTask& operator = (const ImgprcTask &t) {
		if (this != &t) 
		{
			memcpy(this, &t, sizeof(ImgprcTask));
		}
		return *this;
	}
public:
	int _GetTagROIs(string imgname,string model_file, Mat &mat_tag1, Mat &mat_tag2);
	int GetPostcode( OcrAlgorithm_config *pOcrConifg, Logger *pLogger);
	int GetPostcode_v2(const std::string &localimgpath,bool isTopView,
		OcrAlgorithm_config *pOcrConifg, Logger *pLogger,std::vector<std::string>&detected_postcodes);
	int PorcessTask(OcrAlgorithm_config *pOcrConifg, Logger *pLogger);
	int CopyImageFilesFromServer(const std::string &serverIP, const CString &localImgDir, Logger *pLogger, void* pRand, bool appliedCopy=true);
	int DeleteLocalCacheFile(Logger *pLogger);
	int GetPostcodeFromHandwrite(const std::string &localimgpath, bool isTopView,
		OcrAlgorithm_config *pOcrConifg, Logger *pLogger, std::vector<std::string> &detected_postcodes);
	int GetPostcodeFromArbitTag(const std::string &localimgpath, bool isTopView,
		OcrAlgorithm_config *pOcrConifg, Logger *pLogger, std::vector<std::string> &detected_postcodes);
	BYTE GetNextProcessType(OcrAlgorithm_config *pOcrConifg);
	int PreloadImage2Mat(Logger *pLogger);//预加载图片并裁剪出parcel
	int ReleaseMat();
	string ImageID_str();
public:
	static int splitStrByChar(char sC, const char * srcString, std::vector<std::string> &rStrVec); //分割字符串
};


class DisplayRegion
{
public:
	int m_nMax;				//显示的最大行数
	DWORD m_dwRevP;			//接收的最后一行
	DWORD m_dwProcP;		//指向处理过的最后一行
	char m_uchsMsg[IS_DISPLAY_BUFFER_ROW][IS_MES_MAX_LEN];	//存放消息内容
public:
	DisplayRegion();
	~DisplayRegion();
	
};



class ImgprcStatistics
{

public:
	ImgprcStatistics(void);
	~ImgprcStatistics(void);
public:
	DWORD m_dwTotal;		//总数
	DWORD m_dwReadImgok;    //读取ok
	DWORD m_dwTagOcrOKCount;
	DWORD m_dwPrcImgok; 
	DWORD m_dwCopyImgok;
	DWORD m_dwObrOK;
	DWORD m_dwAdd2DBOK;
	DWORD m_dwDoneImg;

	DWORD m_dwReConnectedDB;
	// 清零
	void clear(void);
};


class ImgprcError
{
public:
	ImgprcError(void);
	~ImgprcError(void);
public:
	//error计数
	DWORD m_dwErrDB;			//写数据库的错误
	DWORD m_dwErrUpdateImgprc;	//更新数据失败
	void clear(void);
};
