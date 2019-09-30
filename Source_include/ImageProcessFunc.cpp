#include "ImageProcessFunc.h"




int ImageProcessFunc::adJustBrightness(cv::Mat& src, double alpha, double beta, double anchor)
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

int ImageProcessFunc::makeBoarderConstant(cv::Mat &srcMat, unsigned char boarder_value, int boarder_width)
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

void ImageProcessFunc::rotate_arbitrarily_angle(cv::Mat &src, cv::Mat &dst, float angle)
{
	using namespace cv;
	float radian = angle;//(float)(angle / 180.0 * CV_PI);     //填充图像
	float angle_dec = angle / CV_PI * 180;
	int maxBorder = (int)(max(src.cols, src.rows)* 1.414); //即为sqrt(2)*max   
	int dx = (maxBorder - src.cols) / 2;
	int dy = (maxBorder - src.rows) / 2;
	copyMakeBorder(src, dst, dy, dy, dx, dx, BORDER_CONSTANT, Scalar(0, 0, 0));     //旋转    
	Point2f center((float)(dst.cols / 2), (float)(dst.rows / 2));
	Mat affine_matrix = getRotationMatrix2D(center, angle_dec, 1.0);//求得旋转矩阵    
	warpAffine(dst, dst, affine_matrix, dst.size());     //计算图像旋转之后包含图像的最大的矩形    
	float sinVal = abs(sin(radian));
	float cosVal = abs(cos(radian));
	Size targetSize((int)(src.cols * cosVal + src.rows * sinVal),
		(int)(src.cols * sinVal + src.rows * cosVal));     //剪掉多余边框    
	int x = (dst.cols - targetSize.width) / 2.0;
	int y = (dst.rows - targetSize.height) / 2.0;
	Rect rect(x, y, targetSize.width, targetSize.height);
	if (rect.x + rect.width > dst.cols)
	{
		rect.width = dst.cols - rect.x;
	}
	if (rect.y + rect.height > dst.rows)
	{
		rect.height = dst.rows - rect.y;
	}

	dst = Mat(dst, rect);
}

int ImageProcessFunc::sumPixels(cv::Mat &srcimg, int axis, std::vector<unsigned int> &resultsVec)
{
	assert(srcimg.channels() == 1);
	int h = srcimg.rows;
	int w = srcimg.cols;
	unsigned int sump = 0;
	if (axis == 0)
	{
		for (int i = 0; i < h; i++)
		{
			sump = 0;
			for (int j = 0; j < w; j++)
			{
				sump += srcimg.at<uchar>(i, j);
			}
			resultsVec.push_back(sump);
		}
	}
	if (axis == 1)
	{
		for (int i = 0; i < w; i++)
		{
			sump = 0;
			for (int j = 0; j < h; j++)
			{
				sump += srcimg.at<uchar>(j, i);
			}
			resultsVec.push_back(sump);
		}
	}
	return 1;
}

double ImageProcessFunc::getAveragePixelInRect(cv::Mat& src, cv::Rect &mRect)
{
	cv::Mat tmat = src(mRect);
	int w = tmat.cols;
	int h = tmat.rows;
	int tatalpix = 0;
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			tatalpix = tatalpix + tmat.at<uchar>(j, i);
		}
	}
	return double(tatalpix) / (w*h);
}

double ImageProcessFunc::getAverageBrightness(cv::Mat src)
{
	int height = src.rows;
	int width = src.cols;
	double b = 0;
	if (src.channels() != 1)
	{
		return 0;
	}
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			float v = src.at<uchar>(row, col);
			b += v;
		}
	}
	b = b / (height*width);
	return b;
}

int ImageProcessFunc::getContourRect(std::vector<cv::Point2f> & points_vec, cv::Rect &mRect)
{
	cv::Point2f lf = points_vec[0];
	cv::Point2f rt = points_vec[0];
	cv::Point2f tp = points_vec[0];
	cv::Point2f bt = points_vec[0];
	for (auto pt : points_vec)
	{
		if (pt.x < lf.x) lf = pt;
		if (pt.x > rt.x) rt = pt;
		if (pt.y < tp.y) tp = pt;
		if (pt.y > bt.y) bt = pt;
	}
	float w = rt.x - lf.x;
	float h = bt.y - tp.y;
	mRect.x = lf.x;
	mRect.width = w;
	mRect.y = tp.y;
	mRect.height = h;

	return 1;



}

