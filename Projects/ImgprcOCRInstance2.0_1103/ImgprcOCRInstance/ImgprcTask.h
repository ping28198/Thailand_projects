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
#define IS_BAR_MAX_LEN  51					//�������󳤶�
#define IS_BAR_MAX_NUM	3					//����������
#define IS_IMAGE_MAX_PATH		512			//�ļ�·������󳤶�
#define IS_OCR_MAX_LENGTH		256			//�ļ�·������󳤶�
#define IS_DISPLAY_BUFFER_ROW 2200			//��ʾ�����������
#define IS_MES_MAX_LEN 512					//��Ϣ������ַ�����
#define IS_POSTCODE_LEN 5					//��׼�ʱ೤��
#define IS_POSTCODE_MAX_LEN 512				//�洢�ʱ�Ļ�����󳤶�
#define ST_TASK_UNPROCESS 0					//������״̬
#define ST_TASK_PROCESSING 1
#define ST_TASK_WAIT_PACKAGE 2
//#define ST_TASK_UNPROCESS_2

#define ST_TASK_PROC_TAG 0		//���б�ǩocr
#define ST_TASK_PROC_HWBOX 1		//������д��ocr
#define ST_TASK_PROC_UNKNOWN_TAG 2	  //�����ǩocr
#define ST_TASK_PROC_OVER 250     //û�д���������

#define ST_TASK_PROCESSED 3
#define IS_MAX_IMG_PER_TASK 6				//ÿ����������ͼƬ����

#define ST_TASK_RESULT_OK 1
#define ST_TASK_RESULT_NO_IMAGE 11
#define ST_TASK_RESULT_NO_POSTCODE 12
#define ST_TASK_RESULT_MUL_POSTCODE 13




void CStringToCharArray(CString & cstrItem, char * pchs);//��CStringת��ΪChar����
CString GetNowTime(BYTE bFlag); //����ǰʱ��ת��ΪCString ��bFlag��������
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
	BYTE m_TaskID[6]; //����id
	DWORD m_dwHImageID;
	DWORD m_dwLImageID;
	int m_iOCRID; //�������

	BYTE m_requestType; // �������ͣ�=1 OBR��ʶ�����룬=4 ��������Ŀ�ĵ���Ϣ
	BYTE m_isProcessing; //������״̬��=0 δ���� = 1 ���ڴ���=2 �ȴ������=3 �Ѿ�����
	BYTE m_processType; //�������ͣ�=0 ��׼��ǩOCR��=1 ��д��OCR��=2 �����ǩOCR =250 û������
	BYTE m_resultState; //��������=1 ����ɹ��� = 11 ��ͼ��= 12 δʶ���ʱ࣬=13 ʶ�����ʱ�
	BYTE m_PostcodeIndextImage; //�ʱ�����ͼ��������δʶ��Ϊ0
	BYTE m_isTopViewImage; //�Ƿ��Ƕ���ͼƬ������ͼƬ��һ��Ϊ����ͼ
	int m_iPrcTime; //�㷨����ʱ��
	char m_chsIPAddress[16]; //Imgserver��IP��ַ,��ʱ����
	BYTE m_image_total_num; //�����е���ͼƬ������
	BYTE m_image_num; //���������е�ͼƬ����,Ĭ��Ϊ1
	BYTE m_index_sub_image;//��������ͼƬ���������ܵ���������0��ʼ��
	char m_chsFileName[IS_IMAGE_MAX_PATH*IS_MAX_IMG_PER_TASK]; //ͼƬ��ַ����������';'�ֿ�
	//char m_chsDonePath[IS_IMAGE_MAX_PATH];
	//char m_chsLocalImgDir[IS_IMAGE_MAX_PATH];
	char m_chsLocalImgPath[IS_IMAGE_MAX_PATH*IS_MAX_IMG_PER_TASK];//�����ڱ��ص�ͼƬ·��';'�ֿ�
	BYTE m_Barcode_len;
	char m_chsBarcode[IS_BAR_MAX_LEN]; //�����������������루��ʱ���ã�ֱ�ӷ��أ�
	//char m_chsOCRResult[IS_OCR_MAX_LENGTH];
	char m_chsOcrPostcode[IS_POSTCODE_MAX_LEN]; //�洢ocr���������ã��ֿ�
	cv::Mat* pSrcMat;
	//BYTE m_image_processed_count; //�Ѿ�������������
	int m_postcodeNum;//=0δ����ʱ࣬=1 ���һ���ʱ࣬= 2 ��������ʱ�..
	//BYTE m_bFlag;
	//DWORD m_dwTimeCount;
	DWORD m_dwTimeTrigger; //�����ڲ��������
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
	int PreloadImage2Mat(Logger *pLogger);//Ԥ����ͼƬ���ü���parcel
	int ReleaseMat();
	string ImageID_str();
public:
	static int splitStrByChar(char sC, const char * srcString, std::vector<std::string> &rStrVec); //�ָ��ַ���
};


class DisplayRegion
{
public:
	int m_nMax;				//��ʾ���������
	DWORD m_dwRevP;			//���յ����һ��
	DWORD m_dwProcP;		//ָ����������һ��
	char m_uchsMsg[IS_DISPLAY_BUFFER_ROW][IS_MES_MAX_LEN];	//�����Ϣ����
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
	DWORD m_dwTotal;		//����
	DWORD m_dwReadImgok;    //��ȡok
	DWORD m_dwTagOcrOKCount;
	DWORD m_dwPrcImgok; 
	DWORD m_dwCopyImgok;
	DWORD m_dwObrOK;
	DWORD m_dwAdd2DBOK;
	DWORD m_dwDoneImg;

	DWORD m_dwReConnectedDB;
	// ����
	void clear(void);
};


class ImgprcError
{
public:
	ImgprcError(void);
	~ImgprcError(void);
public:
	//error����
	DWORD m_dwErrDB;			//д���ݿ�Ĵ���
	DWORD m_dwErrUpdateImgprc;	//��������ʧ��
	void clear(void);
};
