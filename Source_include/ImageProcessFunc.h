#pragma once
#include "opencv2/opencv.hpp"
#include <vector>
#include <string>

//////////////////////////////////////////////////////////////////////////
//该文件中提供仅仅依赖opencv的常用函数

class ImageProcessFunc
{
public:
	//调整亮度对比度,alpha，对比度系数，beta亮度系数，anchor对比度的基准点
	//************************************
	// 函数:    adJustBrightness		
	// 全名:  ImageProcessFunc::adJustBrightness		
	// 返回值:   int		#无意义
	// 参数: cv::Mat & src			#输入图像
	// 参数: double alpha			#对比度调整系数
	// 参数: double beta				#亮度调整像素
	// 参数: double anchor			#对比度调整锚点
	//************************************
	static int adJustBrightness(cv::Mat& src, double alpha, double beta, double anchor);


	//************************************
	// 函数:    makeBoarderConstant		
	// 作用：	为图片描边，不会改变图像尺寸，
	// 全名:  ImageProcessFunc::makeBoarderConstant		
	// 返回值:   int		#
	// 参数: cv::Mat & srcMat			#
	// 参数: unsigned char boarder_value			#
	// 参数: int boarder_width			#
	//************************************
	static int makeBoarderConstant(cv::Mat &srcMat, unsigned char boarder_value, int boarder_width);
	
	//沿着任意角度旋转，幅度
	static void rotate_arbitrarily_angle(cv::Mat &src, cv::Mat &dst, float angle);

	//axis = 0 ,沿着x轴投影到y 轴，axis=1,相反
	static int sumPixels(cv::Mat &srcimg, int axis, std::vector<unsigned int> &resultsVec);

	static double getAveragePixelInRect(cv::Mat& src, cv::Rect &mRect);
	static double getAverageBrightness(cv::Mat src);


	//************************************
	// 函数:    getContourRect		
	// 作用：	获得轮廓点的最大外框
	// 全名:  ImageProcessFunc::getContourRect		
	// 返回值:   int		#
	// 参数: std::vector<cv::Point2f> & points_vec			#
	// 参数: cv::Rect & mRect			#
	//************************************
	int getContourRect(std::vector<cv::Point2f> & points_vec, cv::Rect &mRect);

};