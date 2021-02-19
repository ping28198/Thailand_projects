// ConsoleMainFrameV2.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "opencv2/opencv.hpp"
#include "stl/time.hpp"
#include <iostream>
#include "MainWorker.h"
#include "cc_util.hpp"
#include "CutParcelBox_thp.h"
// 




void test_cutparcel()
{
	std::vector<std::string> flist = ccutil::findFiles("E:\\datasets\\ThailandPost\\test_nobox\\CUTs","*.jpg",false,true);
	CutParcelBox parcel;
	std::string savedir = "E:\\datasets\\ThailandPost\\tag_cuts_hwrt";
	for (int i=0;i<flist.size();i++)
	{
		//if (i<14) continue;
		std::cout << flist[i] << std::endl;
		cv::Mat m = cv::imread(flist[i]);
		cv::Mat showm;
		cv::resize(m, showm, cv::Size(), 0.1, 0.1);
		cv::imshow("s", showm);
		cv::Mat pm;
		size_t t0 = stl::time::tick();
		int res = parcel.cutParcelMat(m, pm,0.14,0.88,30,2);
		size_t t1 = stl::time::tick();
		std::cout << "time:"<<t1 - t0 << std::endl;
		if (res)
		{
			cv::Mat sm;
			cv::resize(pm, sm, cv::Size(), 0.4, 0.4);
			cv::imwrite(savedir + "\\" + std::to_string(i) + ".jpg", sm);
			//cv::imshow("parcel", sm);
			
		}
		else
		{
			std::cout << "no parcel" << std::endl;
		}
		//cv::waitKey(0);
	}
}

void test_direction()
{
	std::string dir = "E:\\datasets\\ThailandPost\\test_direction";
	std::vector<std::string> flist = ccutil::findFiles(dir, "*.jpg", false, true);
	for (int i = 0; i < flist.size(); i++)
	{




	}



}

int test_mainworker()
{
	OCR_MainWoker worker;
	worker.m_param.image_server_ip = "127.0.0.1";
	worker.m_param.image_server_port = 6006;
	worker.initial();
	while (true)
	{
		stl::time::sleep(1000);

	}
	return 0;
}

int main()
{

	//auto rotating_logger = spdlog::daily_logger_mt("daily_logger", "logs/daily.txt", 2, 30);
	test_mainworker();
	//test_cutparcel();
	return 0;


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
