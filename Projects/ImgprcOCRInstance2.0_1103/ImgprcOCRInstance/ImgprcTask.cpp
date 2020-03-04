#include "StdAfx.h"
#include "ImgprcTask.h"
#include "atlconv.h"
#include <random>
#include <imagehlp.h>
//#include "CutParcelBoxDll.h"
#include "CutParcelBox.h"
//ImgprcTask
ImgprcTask::ImgprcTask(void)
{
	clear();
}

ImgprcTask::~ImgprcTask(void)
{
}
void ImgprcTask::clear()
{
	memset(this, 0, sizeof(ImgprcTask));
}

/*
int ImgprcTask::_GetTagROIs(string imgname, string model_file, Mat &mat_tag1, Mat &mat_tag2)
{
	Mat cvImg = cv::imread(imgname, 1);
	if (cvImg.empty())
	{
		return 0;
	}
	int  nWidth, nHeight;
	//imshow("abc", cvImg);
	//waitKey(0);
	int nResFlag = 0;

	//string sImage = cstrpath.GetBuffer(0);
	string sGraph = model_file;

	WORD mwTop1 = 0, mwBottom1 = 0, mwLeft1 = 0, mwRight1 = 0;
	WORD mwTop2 = 0, mwBottom2 = 0, mwLeft2 = 0, mwRight2 = 0;
	float mfConfidence_Lowest = 0.3, mfConfidence1 = 0, mfConfidence2 = 0;
	BYTE mbNUM_Detected = 0;
	
	nWidth = cvImg.cols; //
	nHeight = cvImg.rows;
	bool bInitModel = true;
	//try
	//{

	nResFlag = LocatFun(bInitModel, imgname, sGraph, nWidth, nHeight, 0,
		mwTop1, mwBottom1, mwLeft1, mwRight1,
		mwTop2, mwBottom2, mwLeft2, mwRight2,
		mfConfidence_Lowest, mfConfidence1, mfConfidence2,
		mbNUM_Detected);
	//}
	//catch (...)
	//{
	//	return 0;
	//}
	if (mbNUM_Detected==0)
	{
		this->m_postcodeNum = 0;
		return 0;
	}
	Rect roi;
	if (mfConfidence1 > mfConfidence_Lowest)//��Ҫȷ������Ŀ��tag���͵Ķ�Ӧ��ϵ??����������һ�����Ӧ�����ʱ��tag
	{
		roi.x = mwTop1;
		roi.y = mwBottom1;
		roi.width = mwLeft1 - mwTop1;
		roi.height = mwRight1 - mwBottom1;
		mat_tag1 = cvImg(roi);
	}
	if (mfConfidence2 > mfConfidence_Lowest)//��Ҫȷ������Ŀ��tag���͵Ķ�Ӧ��ϵ??����������һ�����Ӧ�����ʱ��tag
	{
		roi.x = mwTop2;
		roi.y = mwBottom2;
		roi.width = mwLeft2 - mwTop2;
		roi.height = mwRight2 - mwBottom2;
		mat_tag2 = cvImg(roi);
	}
	return 1;
}
*/
/*
int ImgprcTask::GetPostcode(OcrAlgorithm_config *pOcrConifg, Logger *pLogger)
{
	clock_t end_t,mtime;
	char logprint[128];
	pLogger->TraceWarning("����һ���µ�����");
	string localimgpath = string(this->m_chsLocalImgPath);
	if (localimgpath.empty())
	{
		pLogger->TraceWarning("ͼƬΪ�գ�");
		this->m_postcodeNum = 0;
		return 0;
	}
	mtime = clock();
	Mat mtag1, mtag2;
	_GetTagROIs(localimgpath,pOcrConifg->detect_model_file_path, mtag1, mtag2); //��ñ�ǩλ��
	//Ĭ��mtag1 Ϊ�����ʱ�Ŀ򣬸���ʵ���������
	end_t = clock();
	double timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	sprintf_s(logprint, "detect����ʱ�䣺%.4fs", timeconsume);
	pLogger->TraceInfo(logprint);

	if (mtag1.empty())
	{
		pLogger->TraceWarning("δʶ�𵽱�ǩ��");
		this->m_postcodeNum = 0;
		return 0;
	}
	

	string imgfile = "./saved_file/";
	string imgfilecode = "./saved_file_code/";
	char mstr[32] = { 0 };
	sprintf_s(mstr, "%d.jpg", mtime);
	imgfile.append(mstr);
	imwrite(imgfile, mtag1);

	Mat mpostcode_roi;
	OcrAlgorithm ocr_algo;
	std::string post_str;
	mtime = end_t;
	int res = ocr_algo.getOcrResultString(mtag1,post_str,pOcrConifg);//����ʱ�λ��
	end_t = clock();
	timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	sprintf_s(logprint, "OCR����ʱ�䣺%.4fs", timeconsume);
	pLogger->TraceInfo(logprint);
	if (res==0)
	{
		pLogger->TraceWarning("OCRδʶ�𵽽����");
		this->m_postcodeNum = 0;
		return 0;
	}

	ofstream out;//����ļ�
	out.open(".\\test.txt", std::ios::app);
	if (out.is_open())
	{
		out << mstr << endl;
		out << "����ʱ��:" << timeconsume <<endl;
		out << post_str << endl;
		out.close();
	}
	string logstr = "OCRʶ����Ϊ:";
	logstr = logstr + post_str;
	pLogger->TraceInfo(logstr.c_str());

	if (post_str.length() == IS_POSTCODE_LEN*2)//�������5λ�ʱ࣬��ʼλ����
	{
		memcpy(this->m_chsOcrPostcode1, post_str.c_str(), IS_POSTCODE_LEN);
		memcpy(this->m_chsOcrPostcode2, post_str.c_str() + IS_POSTCODE_LEN, IS_POSTCODE_LEN);
		this->m_postcodeNum = 2;
		return 2;
	}
	if (post_str.length() == IS_POSTCODE_LEN)//���һ��5λ�ʱ�
	{
		memcpy(this->m_chsOcrPostcode1, post_str.c_str(), IS_POSTCODE_LEN);
		this->m_postcodeNum = 1;
		return 1;
	}
	this->m_postcodeNum = 0;
	return 0;
}
*/
int ImgprcTask::GetPostcode_v2(const std::string &localimgpath, bool isTopView,
	OcrAlgorithm_config *pOcrConifg, Logger *pLogger, std::vector<std::string>&detected_postcodes)
 {
	//��־ͷ��Ϣ
	std::string informor = string("ImageID[") + ImageID_str() + ":" + to_string(this->m_index_sub_image).c_str() + "] Standard tag: ";

	clock_t end_t, mtime;
	char logprint[128];

	mtime = clock();
	cv::Mat src_img;
	if (pSrcMat!=NULL)
	{
		src_img = pSrcMat->clone();
	}
	else
	{
		pLogger->TraceWarning((informor + "Mat is empty!").c_str());
		return 0;
	}
	
	if (src_img.empty())
	{
		string _logstr = informor + "No parcel! Imagepath:" + localimgpath;
		pLogger->TraceWarning(_logstr.c_str());
		this->m_postcodeNum = 0;
		return 0;
	}

	std::vector<cv::Rect> detected_boxes;
	std::vector<int> detected_classes;
	std::vector<float> detected_scores;
	int status = ((tag_detector*)(pOcrConifg->pTagDetector))->detect_mat(src_img, detected_boxes, 
		detected_classes, detected_scores, pOcrConifg->TagDetectConfidence);
	
	double timeconsume;
	//end_t = clock();
	//timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	//sprintf_s(logprint, "detect����ʱ�䣺%.4fs", timeconsume);
	//pLogger->TraceInfo(logprint);

	//if (detected_boxes.empty())
	//{
	//	pLogger->TraceInfo("δʶ�𵽱�ǩ��");
	//	this->m_postcodeNum = 0;
	//	return 0;
	//}
	char mstr[128] = { 0 };
	//OCR, ���ǵ����ܼ�⵽���ͬ���tag
	//std::vector<std::string> detected_postcodes;
	int post_count = 0;
	for (int i=0;i<detected_classes.size();i++)
	{
		if(detected_classes[i]!=0) continue; //ֻ�����ǩR��ǩ
		cv::Mat tag_roi;
		src_img(detected_boxes[i]).copyTo(tag_roi);

		//////////////////////////////////////////////////////////////////////////
		//����
		//string imgfile = "./saved_file/";
		////string imgfilecode = "./saved_file_code/";
		//char mstr[32] = { 0 };
		//int mid = mtime % 1000;
		//sprintf_s(mstr, "%d.jpg", mtime);
		//imgfile.append(mstr);
		//imwrite(imgfile, tag_roi);
		//pLogger->TraceInfo((std::string("ͼƬid��")+mstr).c_str());
		//��������

		OcrAlgorithm ocr_algo;
		std::string post_str;
		//mtime = end_t;
		tesseract::TessBaseAPI *pTess = (tesseract::TessBaseAPI *)(pOcrConifg->pTess);
		int res = 0;
		try
		{
			res = ocr_algo.getOcrResultString(tag_roi, pTess, post_str, pOcrConifg);//����ʱ�
		}
		catch (...)
		{
			pLogger->TraceWarning((informor+"OCR algorithm exception!").c_str());
			continue;
		}

		if (res == 0 || post_str.empty())
		{
			int mid = mtime % 5000;
			string save_ng_file = "./saved_ng_file/";

			if (post_str.empty() && res !=0)
			{
				pLogger->TraceWarning((informor + "Postcode is empty but should not!").c_str());
				sprintf_s(mstr, "ng_%d.jpg", mid);
				save_ng_file.append(mstr);
				imwrite(save_ng_file, tag_roi);
				continue;
			}
			pLogger->TraceInfo((informor + "OCR result is empty").c_str());
			//this->m_postcodeNum = 0;
			sprintf_s(mstr, "%d.jpg", mid);
			save_ng_file.append(mstr);
			imwrite(save_ng_file, tag_roi);
			continue;
		}
		if (post_str.length()==9||post_str.length()==10)
		{
			std::string::iterator it = post_str.begin() + 5;
			post_str.insert(it,'-');
		}
		post_count++;
		detected_postcodes.push_back(post_str);
		//////////////////////////////////////////////////////////////////////////
		//����
		//std::string logstr = "OCRʶ����Ϊ:";
		//logstr = logstr + post_str;
		//pLogger->TraceInfo(logstr.c_str());
		//��������

	}
	//end_t = clock();
	//timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	//sprintf_s(logprint, "�㷨����ʱ�䣺%.4fs", timeconsume);
	//pLogger->TraceInfo(logprint);

	return post_count;
	//_GetTagROIs(localimgpath, pOcrConifg->detect_model_file_path, mtag1, mtag2); //��ñ�ǩλ��
	//Ĭ��mtag1 Ϊ�����ʱ�Ŀ򣬸���ʵ���������


}

int ImgprcTask::PorcessTask(OcrAlgorithm_config *pOcrConifg, Logger *pLogger)
{
	//��־����
	std::string process_type;
	if (this->m_processType == 0)
	{
		process_type = "Standard tag";
	}
	else if (this->m_processType == 1)
	{
		process_type = "Handwrite box";
	}
	else if (this->m_processType == 2)
	{
		process_type = "Arbitrary tag";
	}
	std::string informor = std::string("ImageID[") + ImageID_str() + ":" + to_string(this->m_index_sub_image).c_str() + "] "+ process_type+": ";

	int res = PreloadImage2Mat(pLogger);
	if (res==0)
	{
		this->m_resultState = ST_TASK_RESULT_NO_IMAGE;
		return 0;
	}
	//��ȡͼƬ·��
	//std::vector<std::string> image_pathes;
	//int img_num = splitStrByChar(';', this->m_chsLocalImgPath, image_pathes);
	char loginfo[512] = { 0 };

	//if (img_num != this->m_image_num)
	//{
	//	sprintf_s(loginfo, "Image number from the task is not equal to it from path! task:%d, path:d", this->m_image_num, img_num);
	//	this->m_image_num = img_num;
	//	pLogger->TraceWarning(loginfo);
	//}

	int post_index_image = 0; //�ʱ�����ͼ������
	clock_t end_t, mtime;

	//��ÿ��ͼƬ�����㷨(Ŀǰ����һ��ͼƬ)
	mtime = clock();

	std::vector<std::string> post_code_vec;
	int post_num = 0;
	bool isTopView = (this->m_isTopViewImage == 1) ? true : false;
	switch (this->m_processType)//��������
	{
	case ST_TASK_PROC_TAG:
		post_num = GetPostcode_v2("", isTopView, pOcrConifg, pLogger, post_code_vec);
		break;
	case ST_TASK_PROC_HWBOX:
		post_num = GetPostcodeFromHandwrite("", isTopView, pOcrConifg, pLogger, post_code_vec);
		break;
	case ST_TASK_PROC_UNKNOWN_TAG:
		post_num = GetPostcodeFromArbitTag("", isTopView, pOcrConifg, pLogger, post_code_vec);
		break;
	default:
		break;
	}
	for (int j=0;j< post_num;j++)
	{
		if (this->m_postcodeNum !=0)
		{
			strcat(this->m_chsOcrPostcode, ";");
		}
		//post_index_image = i+1;
		strcat(this->m_chsOcrPostcode, post_code_vec[j].c_str());
		this->m_postcodeNum++;
	}
	//}
	this->m_PostcodeIndextImage = m_index_sub_image; //���һ��

	if (this->m_postcodeNum == 0) //ʶ��0���ʱ�
	{
		this->m_resultState = ST_TASK_RESULT_NO_POSTCODE;
	}
	else if(this->m_postcodeNum == 1) //��ȷʶ��1���ʱ�
	{
		this->m_resultState = ST_TASK_RESULT_OK;
	}
	else if (this->m_postcodeNum > 1)
	{
		this->m_resultState = ST_TASK_RESULT_MUL_POSTCODE;//����ʱ�
	}
	if (this->m_postcodeNum != 0) //ʶ���ʱ�
	{
		std::string logstr = informor + "OCR results:";
		logstr = logstr + this->m_chsOcrPostcode;
		pLogger->TraceInfo(logstr.c_str());
	}
	else
	{
		pLogger->TraceInfo((informor + "Find no postcode!").c_str());
	}
	end_t = clock();
	double timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	sprintf_s(loginfo, "Time consume:%.4fs", timeconsume);
	pLogger->TraceInfo((informor + loginfo).c_str());
	//waitKey(1);
	return this->m_postcodeNum;
}

int ImgprcTask::CopyImageFilesFromServer(const std::string &serverIP, const CString &localImgDir, Logger *pLogger, void* pRand, bool appliedCopy)
{
	//�ӷ���������ͼ���ļ�������
	
	std::vector<std::string> image_pathes;
	int img_num = splitStrByChar(';', this->m_chsFileName, image_pathes);
	char loginfo[512] = { 0 };
	if (img_num != this->m_image_num)
	{
		sprintf_s(loginfo, "Image number from the task is not equal to it from path! task:%d, path:d", this->m_image_num, img_num);
		pLogger->TraceWarning(loginfo);
	}
	int copyed_img_num=0;
	USES_CONVERSION;
	for (int i=0;i<img_num;i++)
	{
		//׼��������ͼƬ��ַ
		CString imgFileName = A2T(image_pathes[i].c_str());
		int pos = imgFileName.Find(_T(":"));
		imgFileName.Delete(0, pos + 2);//ɾ�������̷�
		CString ipAddressServer;
		ipAddressServer.Format(_T("\\\\%s\\"), A2T(serverIP.c_str()));
		imgFileName.Insert(0, ipAddressServer);//���������ַ
		if (appliedCopy)//�Ƿ񿽱��ļ�������
		{
			//׼������ͼƬ��ַ
			CString localImgDir_tmp;//���ص�ַ
			localImgDir_tmp.Format(_T("%s"), localImgDir);
			std::string localImgDir_astr = W2A(localImgDir_tmp.GetBuffer(0));
			if (localImgDir_astr.empty())
			{
				localImgDir_astr = "\\";
			}
			MakeSureDirectoryPathExists(localImgDir_astr.c_str());
			//DWORD vv = (this->m_dwHImageID * (*(default_random_engine*)pRand)() + this->m_dwLImageID) / 10 % 500;
			//localImgName.Format(_T("%03lu.jpg"), vv);
			//
			
			LARGE_INTEGER tima = { 0 };
			QueryPerformanceCounter(&tima);
			long long _t = tima.QuadPart;
			string a = to_string(_t) + ".jpg";
			CString localImgName = A2T(a.c_str());
			localImgDir_tmp.Append(localImgName);
			
			int re = CopyFile(imgFileName.GetBuffer(0), localImgDir_tmp.GetBuffer(0), FALSE);
			if (re == FALSE)
			{
				pLogger->TraceWarning("copy file from image server fail!");
				pLogger->TraceKeyInfo(W2A(imgFileName.GetBuffer(0)));
			}
			else
			{
				copyed_img_num++;
				std::string imgNamestring = CT2A(localImgDir_tmp.GetBuffer(0));
				strcat(this->m_chsLocalImgPath, imgNamestring.c_str());
				strcat(this->m_chsLocalImgPath, ";");
			}
		}
		else
		{
			copyed_img_num++;
			std::string imgNamestring = CT2A(imgFileName.GetBuffer(0));
			strcat(this->m_chsLocalImgPath, imgNamestring.c_str());
			strcat(this->m_chsLocalImgPath, ";");
		}
		//��ȡͼƬ�ı���λ��
	}

	//imgFileName.Format(_T("%s"), mtask.m_chsFileName);
	return copyed_img_num;

}

int ImgprcTask::DeleteLocalCacheFile(Logger *pLogger)
{
	std::vector<std::string> image_pathes;
	int img_num = splitStrByChar(';', this->m_chsLocalImgPath, image_pathes);
	for (int i=0;i<img_num;i++)
	{
		int res = remove(image_pathes[i].c_str());
		if (res!=0)
		{
			pLogger->TraceWarning("Remove local cache image file Fail!");
		}
		else
		{
			this->m_chsLocalImgPath[0] = 0;
		}
	}


	return 1;
}

int ImgprcTask::GetPostcodeFromHandwrite(const std::string &localimgpath, bool isTopView, OcrAlgorithm_config *pOcrConifg, Logger *pLogger, std::vector<std::string> &detected_postcodes)
{
	std::string informor = std::string("ImageID[") + ImageID_str() + ":" + to_string(this->m_index_sub_image).c_str() + "]Handwrite box:";
	HWDigitsOCR hw_digits_ocr;


	if (m_isTopViewImage == 0)
	{
		pLogger->TraceInfo((informor + "ignore!").c_str());
		return 0;//����ͼ�����������ǰ����
	}


	cv::Mat srcMat;
	if (pSrcMat != NULL)
	{
		srcMat = pSrcMat->clone();
	}
	else
	{
		pLogger->TraceWarning((informor + "Mat is empty!").c_str());
		return 0;
	}
	//cv::Mat srcMat = cv::imread(localimgpath);
	if (srcMat.empty())
	{
		pLogger->TraceWarning((informor+"There is no parcel!").c_str());
		return 0;
	}

	
	
	std::string post_code_str;
	
	int  res = 0;
	try
	{
		if (pOcrConifg->is_test_model)
		{
			res = hw_digits_ocr.getPostCode2String_test(srcMat, post_code_str, pOcrConifg);
		}
		else
		{
			res = hw_digits_ocr.getPostCode2String(srcMat, post_code_str, pOcrConifg);
		}
		
	}
	catch (...)
	{
		pLogger->TraceWarning((informor + "handwrite OCR algorithm found exception").c_str());
		pLogger->TraceWarning((std::string("exception file:")+ localimgpath).c_str());
		//cv::imwrite("saved_file/exception.jpg", parcelMat);
	}
	

	if (res==1)
	{
		pLogger->TraceInfo((informor + "handwrite OCR find only destination postcode").c_str());
	}
	if (res == 0 )
	{
		return 0;
	}
	else
	{
		detected_postcodes.push_back(post_code_str);
		std::string info_Str = informor + "get hand write postcode:" + post_code_str;
		pLogger->TraceInfo(info_Str.c_str());
		return 1;
	}
	return 0;
}

int ImgprcTask::GetPostcodeFromArbitTag(const std::string &localimgpath, bool isTopView, OcrAlgorithm_config *pOcrConifg, Logger *pLogger, std::vector<std::string> &detected_postcodes)
{

	std::string informor = std::string("ImageID[") + ImageID_str() + ":" + to_string(this->m_index_sub_image).c_str() + "]Arbitrary tag:";
	
	if (m_isTopViewImage == 0)
	{
		pLogger->TraceInfo((informor + "ignore!").c_str());
		return 0;//����ͼ��������д���
	}

	cv::Mat srcMat;
	if (pSrcMat != NULL)
	{
		srcMat = pSrcMat->clone();
	}
	else
	{
		pLogger->TraceWarning((informor + "Mat is empty!").c_str());
		return 0;
	}
	//cv::Mat srcMat = cv::imread(localimgpath);
	if (srcMat.empty())
	{
		pLogger->TraceWarning((informor + "There is no parcel!").c_str());
		return 0;
	}
	
	//
	//
	//HWDigitsOCR hw_digits_ocr;
	//cv::Mat srcMat = cv::imread(localimgpath);
	//if (srcMat.empty())
	//{
	//	pLogger->TraceWarning((informor + "Image file is empty").c_str());
	//	return 0;
	//}
	//cv::Mat parcelMat;
	//CutParcelBox cutparcel;
	//if (isTopView)//��Զ���ͼ �Ͳ���ͼ �ֱ�������
	//{
	//	int isparcel = 0;
	//	try
	//	{
	//		hw_digits_ocr.getTrayMat(srcMat, parcelMat);
	//		isparcel = cutparcel.getMailBox_Mat(parcelMat, parcelMat);
	//	}
	//	catch (...)
	//	{
	//		pLogger->TraceWarning((informor + "Find Parcel Box exception!").c_str());
	//	}

	//	if (isparcel == 0) {
	//		pLogger->TraceInfo((informor + "Find no parcel in image").c_str());
	//		return 0;
	//	}
	//}
	//else
	//{
	//	return 0;//���Բ�����������ǩʶ��
	//	int isparcel = cutparcel.getMailBox_side(srcMat, parcelMat);
	//	if (isparcel == 0) {
	//		pLogger->TraceInfo((informor + "Find no parcel in image").c_str());
	//		return 0;
	//	}
	//}

	ArbitTagOCR arbtagocr;
	std::string post_code_str;

	int  res = 0;
	try
	{
		res = arbtagocr.getPostCodeString(srcMat, post_code_str, pOcrConifg);

	}
	catch (...)
	{
		pLogger->TraceWarning((informor + "Arbitary tag OCR algorithm found exception").c_str());
		pLogger->TraceWarning((std::string("exception file:") + localimgpath).c_str());
		//cv::imwrite("saved_file/exception.jpg", parcelMat);
	}

	if (res>0)
	{

		if (post_code_str.length() == 9 || post_code_str.length() == 10)
		{
			std::string::iterator it = post_code_str.begin() + 5;
			post_code_str.insert(it, '-');
		}

		detected_postcodes.push_back(post_code_str);
		return 1;
	}
	
	return 0;
}

BYTE ImgprcTask::GetNextProcessType(OcrAlgorithm_config *pOcrConifg)
{
	//���������޸�
	if (this->m_processType==ST_TASK_PROC_TAG)
	{
		if (pOcrConifg->Run_OCR_on_handwrite_box == 1)
		{
			return ST_TASK_PROC_HWBOX;
		}


		return ST_TASK_PROC_OVER;
	}
	if (this->m_processType == ST_TASK_PROC_HWBOX)
	{
		if (pOcrConifg->Run_OCR_on_unknown_tag == 1)
		{
			return ST_TASK_PROC_UNKNOWN_TAG;
		}


		return ST_TASK_PROC_OVER;
	}


	return ST_TASK_PROC_OVER;

}

int ImgprcTask::PreloadImage2Mat(Logger *pLogger)
{
	std::string informor = string("ImageID[") + ImageID_str() + ":" + to_string(this->m_index_sub_image).c_str() + "]:";
	if (pSrcMat!=NULL)
	{
		return 1;
	}
	std::vector<std::string> image_pathes;
	int img_num = splitStrByChar(';', this->m_chsLocalImgPath, image_pathes);
	if (img_num == 0)
	{
		pLogger->TraceWarning((informor + "Image path is empty!").c_str());
	}
	cv::Mat srcm = cv::imread(image_pathes[0]);
	if (srcm.empty())
	{
		pLogger->TraceWarning((informor + "Image loaded is empty! Image path: "+ image_pathes[0]).c_str());
		return 0;
	}

	pSrcMat = new Mat();
	Mat parcelMat;
	CutParcelBox cutparcel;
	if (m_isTopViewImage ==1 )//��Զ���ͼ �Ͳ���ͼ �ֱ�������
	{
		HWDigitsOCR hw_digits_ocr;
		int isparcel = 0;
		try
		{
			hw_digits_ocr.getTrayMat(srcm, parcelMat);
			isparcel = cutparcel.getMailBox_Mat(parcelMat, parcelMat);
		}
		catch (...)
		{
			pLogger->TraceWarning((informor + "Find Parcel Box exception!").c_str());
		}

		if (isparcel == 0) {
			pLogger->TraceInfo((informor + "Find no parcel in image").c_str());
			return 1;
		}
	}
	else
	{
		int isparcel = cutparcel.getMailBox_side(srcm, parcelMat);
		if (isparcel == 0) {
			pLogger->TraceInfo((informor + "Find no parcel in image").c_str());
			return 1;
		}
	}

	parcelMat.copyTo(*pSrcMat);

	return 1;

}

int ImgprcTask::ReleaseMat()
{
	if (pSrcMat==NULL)
	{
		return 1;
	}
	pSrcMat->release();
	delete pSrcMat;
	return 1;
}

std::string ImgprcTask::ImageID_str()
{
	unsigned int taskid=0;
	for (int i=0;i<6;i++)
	{
		unsigned int v = this->m_TaskID[i];
		v *= pow(256, 5-i);
		taskid += v;
	}
	return to_string(taskid);
}

int ImgprcTask::splitStrByChar(char sC, const char * srcString, std::vector<std::string> &rStrVec)
{
	if (srcString == NULL) return 0;
	if (srcString[0] == 0) return 0;
	std::string src_ = srcString;
	size_t pos;
	size_t count_ = 0;
	while (true)
	{
		pos = src_.find_first_of(sC);
		if (pos != std::string::npos)
		{
			std::string img_path = src_.substr(0, pos);
			if (!img_path.empty())
			{
				rStrVec.push_back(img_path);
				count_++;
			}
			//std::cout << img_path << std::endl;
			src_ = src_.substr(pos + 1);
		}
		else
		{
			if (!src_.empty())
			{
				rStrVec.push_back(src_);
				count_++;
			}
			break;
		}
	}
	return count_;
}

//DisplayRegion
DisplayRegion::DisplayRegion()
{
	memset(this, 0,sizeof(DisplayRegion));
	m_nMax = 150;
}

DisplayRegion::~DisplayRegion()
{

}


//functions
void CStringToCharArray(CString & cstrItem, char * pchs)
{
	int nchLen;
	nchLen = 0;
	nchLen = WideCharToMultiByte(CP_ACP, 0, cstrItem, cstrItem.GetLength(), NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, cstrItem, cstrItem.GetLength(), pchs, nchLen, NULL, NULL);
	pchs[nchLen] = NULL;
			
}


CString GetNowTime(BYTE bFlag)
{
	CTime t=CTime::GetCurrentTime();
	switch(bFlag)
	{
	case 0:
		return (t.Format("%Y-%m-%d %H:%M:%S"));
		break;
	case 1:
		return (t.Format("%Y%m%d%H%M%S"));
		break;
	case 2:
		return (t.Format("%H:%M:%S"));
		break;
	case 3:
		return (t.Format("%Y%m%d"));
		break;
	default:
		return (t.Format("%Y-%m-%d %H:%M:%S"));
		break;
	}	
}


//ImgprcStatistics
ImgprcStatistics::ImgprcStatistics(void)
{
	clear();
}

ImgprcStatistics::~ImgprcStatistics(void)
{
}

// ����
void ImgprcStatistics::clear(void)
{
	memset(this,0,sizeof(ImgprcStatistics));
}


//ImgprcError
ImgprcError::ImgprcError()
{
	clear();
}
ImgprcError::~ImgprcError()
{
}

void ImgprcError::clear()
{
	memset(this, 0, sizeof(ImgprcError));
}