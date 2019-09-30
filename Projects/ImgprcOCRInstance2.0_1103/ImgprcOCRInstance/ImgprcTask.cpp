#include "StdAfx.h"
#include "ImgprcTask.h"
#include "atlconv.h"
#include <random>
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
	if (mfConfidence1 > mfConfidence_Lowest)//需要确定这里的框和tag类型的对应关系??，这里假设第一个框对应含有邮编的tag
	{
		roi.x = mwTop1;
		roi.y = mwBottom1;
		roi.width = mwLeft1 - mwTop1;
		roi.height = mwRight1 - mwBottom1;
		mat_tag1 = cvImg(roi);
	}
	if (mfConfidence2 > mfConfidence_Lowest)//需要确定这里的框和tag类型的对应关系??，这里假设第一个框对应含有邮编的tag
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
	pLogger->TraceWarning("处理一个新的任务！");
	string localimgpath = string(this->m_chsLocalImgPath);
	if (localimgpath.empty())
	{
		pLogger->TraceWarning("图片为空！");
		this->m_postcodeNum = 0;
		return 0;
	}
	mtime = clock();
	Mat mtag1, mtag2;
	_GetTagROIs(localimgpath,pOcrConifg->detect_model_file_path, mtag1, mtag2); //获得标签位置
	//默认mtag1 为含有邮编的框，根据实际情况调整
	end_t = clock();
	double timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	sprintf_s(logprint, "detect消耗时间：%.4fs", timeconsume);
	pLogger->TraceInfo(logprint);

	if (mtag1.empty())
	{
		pLogger->TraceWarning("未识别到标签！");
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
	int res = ocr_algo.getOcrResultString(mtag1,post_str,pOcrConifg);//获得邮编位置
	end_t = clock();
	timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	sprintf_s(logprint, "OCR消耗时间：%.4fs", timeconsume);
	pLogger->TraceInfo(logprint);
	if (res==0)
	{
		pLogger->TraceWarning("OCR未识别到结果！");
		this->m_postcodeNum = 0;
		return 0;
	}

	ofstream out;//输出文件
	out.open(".\\test.txt", std::ios::app);
	if (out.is_open())
	{
		out << mstr << endl;
		out << "消耗时间:" << timeconsume <<endl;
		out << post_str << endl;
		out.close();
	}
	string logstr = "OCR识别结果为:";
	logstr = logstr + post_str;
	pLogger->TraceInfo(logstr.c_str());

	if (post_str.length() == IS_POSTCODE_LEN*2)//获得两个5位邮编，起始位计算
	{
		memcpy(this->m_chsOcrPostcode1, post_str.c_str(), IS_POSTCODE_LEN);
		memcpy(this->m_chsOcrPostcode2, post_str.c_str() + IS_POSTCODE_LEN, IS_POSTCODE_LEN);
		this->m_postcodeNum = 2;
		return 2;
	}
	if (post_str.length() == IS_POSTCODE_LEN)//获得一个5位邮编
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
	clock_t end_t, mtime;
	char logprint[128];
	//pLogger->TraceInfo("处理一个新的任务！");
	//std::string localimgpath = std::string(this->m_chsLocalImgPath);
	if (localimgpath.empty())
	{
		pLogger->TraceWarning("图片路径为空！");
		this->m_postcodeNum = 0;
		return 0;
	}
	//pLogger->TraceInfo(localimgpath.c_str());
	mtime = clock();
	cv::Mat src_img = cv::imread(localimgpath);
	if (src_img.empty())
	{
		pLogger->TraceWarning("图片为空！");
		this->m_postcodeNum = 0;
		return 0;
	}

	CutParcelBox cutparcel;
	if (isTopView)//针对顶视图 和侧视图 分别做处理
	{
		int isparcel = cutparcel.getMailBox_Mat(src_img, src_img,0);
		if (isparcel == 0) {
			pLogger->TraceInfo("没有包裹");
			return 0;
		}
	}
	else
	{
		int isparcel = cutparcel.getMailBox_side(src_img, src_img);
		if (isparcel == 0) {
			pLogger->TraceInfo("没有包裹");
			return 0;
		}
	}


	std::vector<cv::Rect> detected_boxes;
	std::vector<int> detected_classes;
	std::vector<float> detected_scores;
	int status = ((tag_detector*)(pOcrConifg->pTagDetector))->detect_mat(src_img, detected_boxes, 
		detected_classes, detected_scores, pOcrConifg->TagDetectConfidence);
	
	double timeconsume;
	//end_t = clock();
	//timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	//sprintf_s(logprint, "detect消耗时间：%.4fs", timeconsume);
	//pLogger->TraceInfo(logprint);

	//if (detected_boxes.empty())
	//{
	//	pLogger->TraceInfo("未识别到标签！");
	//	this->m_postcodeNum = 0;
	//	return 0;
	//}
	char mstr[128] = { 0 };
	//OCR, 考虑到可能检测到多个同类别tag
	//std::vector<std::string> detected_postcodes;
	int post_count = 0;
	for (int i=0;i<detected_classes.size();i++)
	{
		if(detected_classes[i]!=0) continue; //只处理标签R标签
		cv::Mat tag_roi;
		src_img(detected_boxes[i]).copyTo(tag_roi);

		//////////////////////////////////////////////////////////////////////////
		//测试
		//string imgfile = "./saved_file/";
		////string imgfilecode = "./saved_file_code/";
		//char mstr[32] = { 0 };
		//int mid = mtime % 1000;
		//sprintf_s(mstr, "%d.jpg", mtime);
		//imgfile.append(mstr);
		//imwrite(imgfile, tag_roi);
		//pLogger->TraceInfo((std::string("图片id：")+mstr).c_str());
		//结束测试

		OcrAlgorithm ocr_algo;
		std::string post_str;
		//mtime = end_t;
		tesseract::TessBaseAPI *pTess = (tesseract::TessBaseAPI *)(pOcrConifg->pTess);
		int res = 0;
		try
		{
			res = ocr_algo.getOcrResultString(tag_roi, pTess, post_str, pOcrConifg);//获得邮编
		}
		catch (...)
		{
			pLogger->TraceWarning("捕捉到ocr算法异常！");
			continue;
		}
		 
		if (res == 0)
		{
			pLogger->TraceInfo("OCR未识别到结果！");
			this->m_postcodeNum = 0;
			string save_ng_file = "./saved_ng_file/";
			int mid = mtime % 5000;
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
		//调试
		//std::string logstr = "OCR识别结果为:";
		//logstr = logstr + post_str;
		//pLogger->TraceInfo(logstr.c_str());
		//结束调试

	}
	//end_t = clock();
	//timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	//sprintf_s(logprint, "算法消耗时间：%.4fs", timeconsume);
	//pLogger->TraceInfo(logprint);

	return post_count;
	//_GetTagROIs(localimgpath, pOcrConifg->detect_model_file_path, mtag1, mtag2); //获得标签位置
	//默认mtag1 为含有邮编的框，根据实际情况调整


}

int ImgprcTask::PorcessTask(OcrAlgorithm_config *pOcrConifg, Logger *pLogger)
{
	//获取图片路径
	std::vector<std::string> image_pathes;
	int img_num = splitStrByChar(';', this->m_chsLocalImgPath, image_pathes);
	char loginfo[512] = { 0 };

	if (img_num != this->m_image_num)
	{
		sprintf_s(loginfo, "任务指定的图片数量和路径数量不相符！指定数量为：%d,路径数量为：%d", this->m_image_num, img_num);
		this->m_image_num = img_num;
		pLogger->TraceWarning(loginfo);
	}

	int post_index_image = 0; //邮编所在图像索引
	clock_t end_t, mtime;
	//对每张图片运行算法(目前仅有一张图片)
	mtime = clock();
	for (int i=0;i< this->m_image_num;i++)
	{
		std::vector<std::string> post_code_vec;
		int post_num = 0;
		bool isTopView = (this->m_isTopViewImage == 1 && i == 0) ? true : false;
		switch (this->m_processType)
		{
		case ST_TASK_PROC_TAG:
			post_num = GetPostcode_v2(image_pathes[i], isTopView, pOcrConifg, pLogger, post_code_vec);
			break;
		case ST_TASK_PROC_HWBOX:
			post_num = GetPostcodeFromHandwrite(image_pathes[i], isTopView, pOcrConifg, pLogger, post_code_vec);
			break;
		default:
			break;
		}
		this->m_postcodeNum += post_num;
		for (int j=0;j< post_num;j++)
		{
			post_index_image = i+1;
			strcat(this->m_chsOcrPostcode, post_code_vec[j].c_str());
			strcat(this->m_chsOcrPostcode, ";");
		}
	}
	this->m_PostcodeIndextImage = post_index_image; //最后一个

	if (this->m_postcodeNum == 0) //识别到0个邮编
	{
		this->m_resultState = 12;
	}
	else if(this->m_postcodeNum == 1) //正确识别到1个邮编
	{
		this->m_resultState = 1;
	}
	else if (this->m_postcodeNum > 1)
	{
		this->m_resultState = 13;//多个邮编
	}
	end_t = clock();
	double timeconsume = (double)(end_t - mtime) / CLOCKS_PER_SEC;
	sprintf_s(loginfo, "算法消耗时间：%.4fs", timeconsume);

	pLogger->TraceInfo(loginfo);
	std::string logstr = "OCR识别结果为:";
	logstr = logstr + this->m_chsOcrPostcode;
	pLogger->TraceInfo(logstr.c_str());

	return this->m_postcodeNum;
}

int ImgprcTask::CopyImageFilesFromServer(const std::string &serverIP, const CString &localImgDir, Logger *pLogger, void* pRand, bool appliedCopy)
{
	//从服务器拷贝图像文件到本地
	
	std::vector<std::string> image_pathes;
	int img_num = splitStrByChar(';', this->m_chsFileName, image_pathes);
	char loginfo[512] = { 0 };
	if (img_num != this->m_image_num)
	{
		sprintf_s(loginfo, "任务指定的图片数量和路径数量不相符！指定数量为：%d,路径数量为：%d", this->m_image_num, img_num);
		pLogger->TraceWarning(loginfo);
	}
	int copyed_img_num=0;
	USES_CONVERSION;
	for (int i=0;i<img_num;i++)
	{
		//准备服务器图片地址
		CString imgFileName = A2T(image_pathes[i].c_str());
		int pos = imgFileName.Find(_T(":"));
		imgFileName.Delete(0, pos + 2);//删除本地盘符
		CString ipAddressServer;
		ipAddressServer.Format(_T("\\\\%s\\"), A2T(serverIP.c_str()));
		imgFileName.Insert(0, ipAddressServer);//加入网络地址
		if (appliedCopy)//是否拷贝文件到本地
		{
			//准备本地图片地址
			CString localImgDir_tmp;//本地地址
			localImgDir_tmp.Format(_T("%s"), localImgDir);
			CString localImgName;
			DWORD vv = (this->m_dwHImageID * (*(default_random_engine*)pRand)() + this->m_dwLImageID) / 10 % 500;
			localImgName.Format(_T("%03lu.jpg"), vv);
			localImgDir_tmp.Append(localImgName);
			int re = CopyFile(imgFileName.GetBuffer(0), localImgDir_tmp.GetBuffer(0), FALSE);
			if (re == FALSE)
			{
				pLogger->TraceKeyInfo("从远程拷贝文件失败！");
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
		//获取图片的本地位置
	}

	//imgFileName.Format(_T("%s"), mtask.m_chsFileName);
	return copyed_img_num;

}

int ImgprcTask::GetPostcodeFromHandwrite(const std::string &localimgpath, bool isTopView, OcrAlgorithm_config *pOcrConifg, Logger *pLogger, std::vector<std::string> &detected_postcodes)
{
	cv::Mat srcMat = cv::imread(localimgpath);
	if (srcMat.empty())
	{
		pLogger->TraceWarning("Image file is empty");
		return 0;
	}
	
	CutParcelBox cutparcel;
	if (isTopView)//针对顶视图 和侧视图 分别做处理
	{
		int isparcel = cutparcel.getMailBox_Mat(srcMat, srcMat, 0);
		if (isparcel == 0) {
			pLogger->TraceInfo("Find no parcel in image");
			return 0;
		}
	}
	else
	{
		int isparcel = cutparcel.getMailBox_side(srcMat, srcMat);
		if (isparcel == 0) {
			pLogger->TraceInfo("Find no parcel in image");
			return 0;
		}
	}
	
	HWDigitsOCR hw_digits_ocr;
	std::string post_code_str;
	
	int  res = 0;
	try
	{
		res = hw_digits_ocr.getPostCode2String(srcMat, post_code_str, pOcrConifg);
	}
	catch (...)
	{
		pLogger->TraceWarning("handwrite OCR algorithm found exception");
	}
	
	if (res!=0)
	{
		detected_postcodes.push_back(post_code_str);
	}
	if (res==1)
	{
		pLogger->TraceInfo("handwrite OCR find only destination postcode");
	}
	if (res == 0 )
	{
		return 0;
	}
	else
	{
		std::string info_Str = "get hand write postcode:" + post_code_str;
		pLogger->TraceInfo(info_Str.c_str());
		return 1;
	}
	return 0;
}

BYTE ImgprcTask::GetNextProcessType(OcrAlgorithm_config *pOcrConifg)
{
	//按照流程修改
	if (this->m_processType==ST_TASK_PROC_TAG)
	{
		if (pOcrConifg->Run_OCR_on_handwrite_box==1)
		{
			return ST_TASK_PROC_HWBOX;
		}


		return ST_TASK_PROC_OVER;
	}
	if (this->m_processType == ST_TASK_PROC_HWBOX)
	{
		if (pOcrConifg->Run_OCR_on_unknown_tag==1)
		{
			return ST_TASK_PROC_UNKNOWN_TAG;
		}


		return ST_TASK_PROC_OVER;
	}


	return ST_TASK_PROC_OVER;

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

// 清零
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