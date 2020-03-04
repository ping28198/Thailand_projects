// Test_tesseract.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "baseapi.h"
#include "CommonFunc.h"
#include <string>
#include "opencv2/opencv.hpp"
using namespace cv;
using namespace std;
int main()
{
    std::cout << "Hello World!\n"; 
	tesseract::TessBaseAPI tess;
	tesseract::TessBaseAPI*pTess = &tess;
	std::string exedir = CommonFunc::get_exe_dir();
	if (tess.Init(exedir.c_str(), "eng"))
	{
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
		return 0;
	}
	pTess->SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_LINE);
	pTess->SetVariable("save_best_choices", "T");
	//pTess->SetVariable("classify_bln_numeric_mode", "1");
	string dir = "F:\\cpte_datasets\\Tailand_tag_detection_datasets\\tag_cut_img\\barcode_img\\*.jpg";
	vector<string> imgfiles;
	clock_t start_t, end_t;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);
	for (int i=0;i<imgfiles.size();i++)
	{
		Mat srcm = imread(imgfiles[i]);

		if (srcm.channels()==3)
		{
			cvtColor(srcm, srcm, cv::COLOR_BGR2GRAY);
		}
		int sh = 30;
		float scal_ = sh / float(srcm.rows);

		resize(srcm, srcm, cv::Size(), scal_, scal_);
		//threshold(srcm, srcm, 150, 250, cv::THRESH_BINARY);
		imshow("src", srcm);
		int w = srcm.cols;
		int h = srcm.rows;
		pTess->SetImage(srcm.data, w, h, srcm.channels(), srcm.step1());
		pTess->Recognize(0);
		cout << pTess->GetUTF8Text() << endl;
		waitKey(0);

	}









}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
