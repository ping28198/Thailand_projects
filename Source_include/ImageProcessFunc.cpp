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

int ImageProcessFunc::getContourRect(std::vector<cv::Point> & points_vec, cv::Rect &mRect)
{
	cv::Point lf = points_vec[0];
	cv::Point rt = points_vec[0];
	cv::Point tp = points_vec[0];
	cv::Point bt = points_vec[0];
	for (auto pt : points_vec)
	{
		if (pt.x < lf.x) lf = pt;
		if (pt.x > rt.x) rt = pt;
		if (pt.y < tp.y) tp = pt;
		if (pt.y > bt.y) bt = pt;
	}
	int w = rt.x - lf.x+1;
	int h = bt.y - tp.y+1;
	mRect.x = lf.x;
	mRect.width = w;
	mRect.y = tp.y;
	mRect.height = h;

	return 1;
}

int ImageProcessFunc::CropRect(cv::Rect main_rect, cv::Rect &to_crop_rect)
{
	if ((to_crop_rect.x + to_crop_rect.width) <= main_rect.x)
	{
		to_crop_rect = cv::Rect();
		return 0;
	}
	if ((to_crop_rect.y + to_crop_rect.height) <= main_rect.y)
	{
		to_crop_rect = cv::Rect();
		return 0;
	}
	if (to_crop_rect.x >= (main_rect.width + main_rect.x))
	{
		to_crop_rect = cv::Rect();
		return 0;
	}
	if (to_crop_rect.y >= (main_rect.height + main_rect.y))
	{
		to_crop_rect = cv::Rect();
		return 0;
	}
	int tl_x = to_crop_rect.x;
	int tl_y = to_crop_rect.y;
	int br_x = to_crop_rect.x + to_crop_rect.width;
	int br_y = to_crop_rect.y + to_crop_rect.height;

	if (main_rect.x > tl_x) tl_x = main_rect.x;
	if (main_rect.y > tl_y) tl_y = main_rect.y;
	if (main_rect.x + main_rect.width < br_x) br_x = main_rect.x + main_rect.width;
	if (main_rect.y + main_rect.height < br_y) br_y = main_rect.y + main_rect.height;

	cv::Rect tmp(tl_x, tl_y, br_x - tl_x, br_y - tl_y);
	to_crop_rect = tmp;
	return 1;
}

bool ImageProcessFunc::IsPointInRect(cv::Point pt, cv::Rect rc)
{
	if (pt.x <= (rc.x+rc.width) && pt.x >= rc.x)
	{
		if (pt.y <= (rc.y + rc.height) && pt.y >= rc.y)
		{
			return true;
		}
	}
	return false;
}

int ImageProcessFunc::getMatFromRotatedRect(const cv::Mat &src_mat, cv::Mat &dst_mat, cv::RotatedRect rRc)
{
	
	cv::Rect boxRec = rRc.boundingRect();
	//cv::Rect boxRec_n(0, 0, boxRec.width, boxRec.height);

	cv::Mat padMat = cv::Mat::zeros(boxRec.size(), src_mat.type());
	cv::Rect cropRect = boxRec;
	int res = CropRect(cv::Rect(0, 0, src_mat.cols, src_mat.rows), cropRect);
	if (!res) return 0;
	cv::Mat boxMat = src_mat(cropRect);
	cv::Rect copyRect(cropRect.x - boxRec.x, cropRect.y - boxRec.y, cropRect.width, cropRect.height);
	boxMat.copyTo(padMat(copyRect));
	int max_lenth = std::max(padMat.cols, padMat.rows);

	cv::Mat padMat_e = cv::Mat::zeros(cv::Size(max_lenth, max_lenth), src_mat.type());
	cv::Rect rec_e;
	rec_e.x = (padMat.cols > padMat.rows) ? 0 : (padMat.rows - padMat.cols) / 2;
	rec_e.y = (padMat.cols > padMat.rows) ? (padMat.cols - padMat.rows) / 2 : 0;
	rec_e.width = padMat.cols;
	rec_e.height = padMat.rows;

	padMat.copyTo(padMat_e(rec_e));

	cv::Mat affine_matrix = cv::getRotationMatrix2D(cv::Point2f(padMat_e.cols / 2.0f, padMat_e.rows / 2.0f), rRc.angle, 1.0);//求得旋转矩阵    
	cv::warpAffine(padMat_e, padMat_e, affine_matrix, padMat_e.size());     //计算图像旋转之后包含图像的最大的矩形    

	cv::Rect cropRec;
	cropRec.x = padMat_e.cols / 2 - rRc.size.width / 2;
	cropRec.y = padMat_e.rows / 2 - rRc.size.height / 2;
	cropRec.width = rRc.size.width;
	cropRec.height = rRc.size.height;
	CropRect(cv::Rect(0, 0, padMat_e.cols, padMat_e.rows), cropRec);

	padMat_e(cropRec).copyTo(dst_mat);

	return 1;
	
}

