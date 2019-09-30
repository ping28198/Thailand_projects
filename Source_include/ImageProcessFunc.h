#pragma once
#include "opencv2/opencv.hpp"
#include <vector>
#include <string>

//////////////////////////////////////////////////////////////////////////
//���ļ����ṩ��������opencv�ĳ��ú���

class ImageProcessFunc
{
public:
	//�������ȶԱȶ�,alpha���Աȶ�ϵ����beta����ϵ����anchor�ԱȶȵĻ�׼��
	//************************************
	// ����:    adJustBrightness		
	// ȫ��:  ImageProcessFunc::adJustBrightness		
	// ����ֵ:   int		#������
	// ����: cv::Mat & src			#����ͼ��
	// ����: double alpha			#�Աȶȵ���ϵ��
	// ����: double beta				#���ȵ�������
	// ����: double anchor			#�Աȶȵ���ê��
	//************************************
	static int adJustBrightness(cv::Mat& src, double alpha, double beta, double anchor);


	//************************************
	// ����:    makeBoarderConstant		
	// ���ã�	ΪͼƬ��ߣ�����ı�ͼ��ߴ磬
	// ȫ��:  ImageProcessFunc::makeBoarderConstant		
	// ����ֵ:   int		#
	// ����: cv::Mat & srcMat			#
	// ����: unsigned char boarder_value			#
	// ����: int boarder_width			#
	//************************************
	static int makeBoarderConstant(cv::Mat &srcMat, unsigned char boarder_value, int boarder_width);
	
	//��������Ƕ���ת������
	static void rotate_arbitrarily_angle(cv::Mat &src, cv::Mat &dst, float angle);

	//axis = 0 ,����x��ͶӰ��y �ᣬaxis=1,�෴
	static int sumPixels(cv::Mat &srcimg, int axis, std::vector<unsigned int> &resultsVec);

	static double getAveragePixelInRect(cv::Mat& src, cv::Rect &mRect);
	static double getAverageBrightness(cv::Mat src);


	//************************************
	// ����:    getContourRect		
	// ���ã�	����������������
	// ȫ��:  ImageProcessFunc::getContourRect		
	// ����ֵ:   int		#
	// ����: std::vector<cv::Point2f> & points_vec			#
	// ����: cv::Rect & mRect			#
	//************************************
	int getContourRect(std::vector<cv::Point2f> & points_vec, cv::Rect &mRect);

};