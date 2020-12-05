// DLL_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include <iostream>
#include "opencv2/opencv.hpp"
#include "HWDigitsRecogDll.h"
#include "CommonFunc.h"
#include <vector>
#include <string>
#include "ImageProcessFunc.h"
#include "cutParcel.h"
using namespace std;
using namespace cv;
int test_HDR_digits();
int adJustBrightness(cv::Mat& src, double alpha, double beta, double anchor);
int makeBoarderConstant(cv::Mat &srcMat, unsigned char boarder_value, int boarder_width);
int test_cut_parcelbox();




int main()
{
    std::cout << "Hello World!\n"; 


	//test_HDR_digits();
	test_cut_parcelbox();










}


int test_cut_parcelbox()
{
	string src_img = "F:\\cpte_datasets\\AlignCenter\\15\\769XG-PS001_20201009153016044_01_0555_0000000000_GR_QR608370986242,608370986242_Top.jpg";
	
	std::vector<std::string> flist;
	CommonFunc::getAllFilesNameInDir("F:\\cpte_datasets\\parcelCut", flist, true, true);
	for (int i = 0; i < flist.size(); i++)
	{
		string dst_img = flist[i] + ".jpg";
		cutParcel((char*)flist[i].c_str(), (char*)dst_img.c_str(), 1, 0.5, 100, 1);
	}

	

	return 1;
}






int test_HDR_digits()
{
	std::string dir = "F:/CommonDatasets/MNIST_ext/*.jpg";
	//string dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/Image[2019-8-2]/*.jpg";
	std::vector<std::string> imgfiles;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);
	HWDigitsRecog hwdr;
	//int res = hwdr.initial_avg("E:/python_projects/Digits_recog_cnn/HDRdigits_avg.pb");
	int res = hwdr.initial("E:/python_projects/Digits_recog_cnn/HDRdigits_v8_dper.pb");
	if (res == 0)
	{
		std::cout << "initial error" << std::endl;
		return 0;
	}
	std::cout << "图片数量：" << imgfiles.size() << endl;
	
	int low_confid_count = 0;
	int diff_count = 0;
	for (int i = 0; i < imgfiles.size(); i++)
	{
		char cc[2] = { 0 };
		int a = -1;
		size_t pos = imgfiles[i].rfind('_');
		if (pos!= imgfiles[i].npos)
		{
			cc[0] = imgfiles[i][pos - 1];
			a = atoi(cc);
		}
		


		cv::Mat srcm = cv::imread(imgfiles[i]);
		if (srcm.channels()==3)
		{
			cv::cvtColor(srcm, srcm, cv::COLOR_BGR2GRAY);
		}
		if (srcm.cols != 28 || srcm.rows != 28)
		{
			cv::resize(srcm, srcm, cv::Size(28, 28), 0, 0, cv::INTER_AREA);
		}
		cv::threshold(srcm, srcm, 50, 0, CV_THRESH_TOZERO);

		//ImageProcessFunc::makeBoarderConstant(srcm, 0, 1);
		Mat resized_mat;
		resize(srcm(cv::Rect(1, 1, 26, 26)), resized_mat, Size(28, 28), cv::INTER_AREA);

		Mat mm;
		resized_mat.copyTo(mm);

		std::vector<cv::Mat> imgs;
		imgs.push_back(mm);

		//imshow("test_srcm", mm);

		std::vector<int> class_; 
		std::vector<float> confd_;
		//hwdr.detect_mat_avg(imgs, class_, confd_);
		int ind1 = 0;
		int ind2 = 0;
		//for (int j = 0; j < class_.size(); j++)
		//{
		//	std::cout << "数字avg：" << class_[j] << "@" << confd_[j] << std::endl;
		//	if (confd_[j]<0.9)
		//	{
		//		cout << "################" << endl;
		//		
		//	}
		//	ind1 = class_[j];

		//}
		hwdr.detect_mat(imgs, class_, confd_);
		for (int j = 0; j < class_.size(); j++)
		{
			std::cout << "数字：" << class_[j] << "@" << confd_[j] << std::endl;
			if (confd_[j] < 0.9)
			{
				cout << "#####################" << endl;
				low_confid_count++;
			}
			ind2 = class_[j];
		}
		//cout << srcm << endl ;
		//waitKey(0);
		if (ind2!=a)
		{
			cout << "$$$$$$$$$$$$$$$$$$$&&:"<< a << endl;
			diff_count++;
		}
		//waitKey(0);
	}
	float diff_rate = diff_count / float(imgfiles.size());
	float low_rate = low_confid_count / float(imgfiles.size());
	cout << "低于阈值的比率：" << low_rate << endl;
	cout << "差异率：" << diff_rate << endl;
	//if (srcm.channels() == 3)
	//{
	//	cv::cvtColor(srcm, srcm, cv::COLOR_BGR2GRAY);
	//}

	//adJustBrightness(srcm, 10, 0, 40);
	//imshow("ssrc", srcm);
	////cv::threshold(srcm, srcm, 40, 255,cv::THRESH_BINARY);
	//srcm = ~srcm;
	//makeBoarderConstant(srcm, 0, 1);

	//imshow("srcm", srcm);
	//imgs.push_back(srcm);


	cv::waitKey(0);
	




	return 1;
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
