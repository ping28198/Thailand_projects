// TestOCRAPI.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "API_define_OCR_hand.h"
#include <opencv2/opencv.hpp>
#include "CommonFunc.h"

int Hwr_Recg(cv::Mat srcimg, BYTE *bHwrOcrResult);
int main()
{
    std::cout << "Hello World!\n"; 
	WZJ_Hwr_Init((char*)".\\Wei");

	std::string dir = "F:/手写框/box/*.jpg";
	//string dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/Image[2019-8-2]/*.jpg";
	std::vector<std::string> imgfiles;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);


	for (int i=0;i<imgfiles.size();i++)
	{
		BYTE OCR_RST[5] = { 0 };
		cv::Mat m_ = cv::imread(imgfiles[i]);
		cv::imshow("src", m_);
		Hwr_Recg(m_, OCR_RST);
		std::cout << OCR_RST[0]<<" " << OCR_RST[1] << " " << OCR_RST[2] << " " << std::endl;
		cv::waitKey(0);
	}


	
	




	








}
int adJustBrightness(cv::Mat& src, double alpha, double beta, double anchor)
{
	int height = src.rows;
	int width = src.cols;
	if (src.channels() != 1)
	{
		return 0;
	}
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			float v = src.at<uchar>(row, col);
			src.at<uchar>(row, col) = cv::saturate_cast<uchar>(v*alpha + (1 - alpha)*anchor + beta);
		}
	}
	return 1;
}

int makeBoarderConstant(cv::Mat &srcMat, unsigned char boarder_value, int boarder_width)
{
	if (srcMat.empty()) return 0;
	int w = srcMat.cols;
	int h = srcMat.rows;
	for (int i = 0; i < boarder_width; i++)
	{
		line(srcMat, cv::Point(i, i), cv::Point(i, h - i - 1),
			cv::Scalar(boarder_value, boarder_value, boarder_value), 1);
		line(srcMat, cv::Point(i, h - i - 1), cv::Point(w - i - 1, h - i - 1),
			cv::Scalar(boarder_value, boarder_value, boarder_value), 1);
		line(srcMat, cv::Point(w - i - 1, h - i - 1), cv::Point(w - i - 1, i),
			cv::Scalar(boarder_value, boarder_value, boarder_value), 1);
		line(srcMat, cv::Point(w - i - 1, i), cv::Point(i, i),
			cv::Scalar(boarder_value, boarder_value, boarder_value), 1);
	}

	return 1;
}
int Hwr_Recg(cv::Mat srcimg, BYTE *bHwrOcrResult)
{

	if (srcimg.channels() == 3)
	{
		cv::cvtColor(srcimg, srcimg, cv::COLOR_BGR2GRAY);
	}

	//adJustBrightness(srcimg, 10, 0, 40);
	cv::threshold(srcimg, srcimg, 40, 255, cv::THRESH_BINARY);
	
	//srcimg = ~srcimg;
	makeBoarderConstant(srcimg, 255, 1);
	srcimg = ~srcimg;
	cv::imshow("binary0", srcimg);
	///cv::imshow("ssrc", srcimg);
	


	//cv::threshold(srcimg, srcimg, 50, 255, cv::THRESH_BINARY);
	
	//cv::imshow("binary", srcimg);

	cv::Mat mat_40_48,mat_80_96;
	cv::resize(srcimg, mat_40_48, cv::Size(40, 48));
	cv::resize(srcimg, mat_80_96, cv::Size(80, 96));
	//cv::transpose(mat_80_96, mat_80_96);
	//cv::transpose(mat_40_48, mat_40_48); 


	bHwrOcrResult[0] = XF_Hwr_Recg_One(mat_80_96.data, 1, 0, 0, 0, 0, 43, 33);

	// Hanwang's result;
	bHwrOcrResult[1] = HWNR_RecognizeVcc(mat_80_96.data, 1, 0, 0, 0, 0);

	// Cheng Qi's OCR.
	//bHwrOcrResult[2] = CHQ_Hwr_Recg(mat_40_48.data, 0, 48,
	//	48, 0, Flag_4_1, value1, value2);

	// Xu ling's OCR.
	XL_Hwr_Recg(mat_40_48.data, &bHwrOcrResult[2], 0,
		48, 0, 40);

	// Wei Zhujun's OCR.
	//bHwrOcrResult[4] = WZJ_Hwr_Recg(pbImg_40_48, ppp_40_48, pbImg_80_96, ovcs_up,
	//	ovcs_down, ovcs_right, ovcs_left, Flag_4_1);

	return 1;
}
