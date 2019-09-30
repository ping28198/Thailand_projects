// OcrTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "RECOG.h"
#include "pch.h"
#include <iostream>
#include "../ImageAdjust/CommonFunc.h"
#include <iostream>
#include "cv.h"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "highgui.h"

#include "tchar.h"
int runOcr();
int main()
{
    std::cout << "Hello World!\n"; 
	runOcr();
}

int runOcr()
{
	std::string dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/tag_cut_img/barcode_img\\*.jpg";
	//string dir = "E:\\git\\text2image";
	std::vector<std::string> imgfiles;
	clock_t start_t, end_t;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);
	for (int i=0;i<imgfiles.size();i++)
	{
		cv::Mat srcimg = cv::imread(imgfiles[i]);
		cvtColor(srcimg, srcimg, cv::COLOR_BGR2GRAY);
		cv::imshow("img", srcimg);
		int w = srcimg.cols;
		int h = srcimg.rows;
		C_RECOG RECOG(srcimg.data, w, h);
		RECT R;
		// 识别范围设为整幅图像
		R.left = R.top = 0;
		R.right = w - 1;
		R.bottom = h - 1;
		RECOG.TRANS_HWPrint_Check(R);
		char repath[16] = "D:\\OCR\\";
		RECOG.RECOG_HWPrint_Check(repath);
		CString wstr;
		if (RECOG.bFlag_HWPrint == 3) {
			wstr = RECOG.GetRECOGRES();
		}
		RECOG.Reset_Check();  // 所有处理结束后重置
		USES_CONVERSION;
		string mstr = T2A(wstr.GetBuffer(0));
		std::cout << mstr << std::endl;
		cv::waitKey(0);
	}





	return 1;
}
