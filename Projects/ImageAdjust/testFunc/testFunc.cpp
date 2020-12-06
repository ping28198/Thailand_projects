// testFunc.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;


void merge_contours(std::vector<std::vector<cv::Point2f>> &cam1_parcels,
	std::vector<std::vector<cv::Point2f>> &cam2_parcels,
	std::vector<std::vector<cv::Point2f>> &out_parcels, float centor_y = 770.0)
{
	if (cam1_parcels.size() == 0 || cam2_parcels.size() == 0)
	{
		out_parcels.insert(out_parcels.begin(), cam1_parcels.begin(), cam1_parcels.end());
		out_parcels.insert(out_parcels.begin(), cam2_parcels.begin(), cam2_parcels.end());
		return;
	}
	std::vector<std::vector<cv::Point2f>>::iterator it;
	std::vector<cv::RotatedRect> cam1_rt_parcels;
	std::vector<cv::RotatedRect> cam2_rt_parcels;
	for (it = cam1_parcels.begin(); it != cam1_parcels.end(); it++)
	{
		cv::RotatedRect rt((*it)[0], (*it)[1], (*it)[2]);
		cam1_rt_parcels.push_back(rt);
	}
	for (it = cam2_parcels.begin(); it != cam2_parcels.end(); it++)
	{
		cv::RotatedRect rt((*it)[0], (*it)[1], (*it)[2]);
		cam2_rt_parcels.push_back(rt);
	}

	//std::sort(rt_parcels.begin(), rt_parcels.end(), sort_parcels_area);
	std::vector<cv::RotatedRect>::iterator it0;
	std::vector<cv::RotatedRect>::iterator it1;
	for (it0 = cam1_rt_parcels.begin(); it0 != cam1_rt_parcels.end();)
	{
		bool is_erased = false;
		for (it1 = cam2_rt_parcels.begin(); it1 != cam2_rt_parcels.end();)
		{
			std::vector<cv::Point2f> contour;
			cv::rotatedRectangleIntersection(*it0, *it1, contour);
			if (contour.size() < 3)
			{
				it1++;
			}
			else
			{
				double contour_area = cv::contourArea(contour);
				if (contour_area / (min(it1->size.area(), it0->size.area()) + 1e-6) > 0.3)
				{
					if ((centor_y - it0->center.y) > (it1->center.y - centor_y))
					{
						it1 = cam2_rt_parcels.erase(it1);
						continue;
					}
					else
					{
						it0 = cam1_rt_parcels.erase(it0);
						is_erased = true;
						break;
					}
				}
				else
				{
					it1++;
				}
			}
		}
		if (is_erased == false)
		{
			it0++;
		}

	}
	std::vector<std::vector<cv::Point2f>>().swap(out_parcels);
	cv::Point2f pts[4];
	for (it0 = cam1_rt_parcels.begin(); it0 != cam1_rt_parcels.end(); it0++)
	{
		it0->points(pts);
		std::vector<cv::Point2f> parcelpt(pts, pts + 4);
		out_parcels.push_back(parcelpt);
	}
	for (it1 = cam1_rt_parcels.begin(); it1 != cam1_rt_parcels.end(); it1++)
	{
		it1->points(pts);
		std::vector<cv::Point2f> parcelpt(pts, pts + 4);
		out_parcels.push_back(parcelpt);
	}
}





int main()
{
	vector<vector<cv::Point2f>> points1{ {cv::Point2f(-83.0852203,147.818878),
		cv::Point2f(-269.850372,141.306488),cv::Point2f(-261.677399,-93.0807800),
		cv::Point2f(-74.9122925,-86.5683594)} };
	vector<vector<cv::Point2f>> points2{ {cv::Point2f(-83.0852203,147.818878),
		cv::Point2f(-269.850372,141.306488),cv::Point2f(-261.677399,-93.0807800),
		cv::Point2f(-74.9122925,-86.5683594)} };
	vector<vector<cv::Point2f>> points;
	cv::Mat m(cv::Size(1000, 1000), CV_32FC3);
	for (int i = 0; i < points1[0].size(); i++)
	{
		cv::drawMarker(m, points1[0][i]+cv::Point2f(500,500), cv::Scalar(255, 255, 0));
		cv::putText(m, to_string(i), points1[0][i] + cv::Point2f(500, 500), CV_FONT_HERSHEY_COMPLEX, 2, cv::Scalar(0, 255, 255));

	}
	cv::imshow("a", m);
	waitKey(0);



	//merge_contours(points1, points2, points);
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
