
//#include "stdafx.h"
#include "CutParcelBox.h"
#include <algorithm>
#include <ReadBarcode.h>
#include "TextUtfEncoding.h"
#include <phash.hpp>


int CutParcelBox::getMailBox(std::string &srcImgPath, std::string &dstImgPath, int applid_rotate /*= 1*/, double boxSizeThreshold/*=50*/, double binaryThreshold/*=50*/, int is_top)
{
	cv::Mat srcmat = cv::imread(srcImgPath);
#ifdef CUT_PARCEL_BOX_DEBUG
	if (srcmat.empty())
	{
		std::cout << "image file is empty or invalid:" << srcImgPath << std::endl;
	}
	
#endif // CUT_PARCEL_BOX_DEBUG

	cv::Mat dstmat;
	int res = 0;
	res = getMailBox_Mat(srcmat, dstmat, applid_rotate, boxSizeThreshold, binaryThreshold);
	//if (is_top)
	//{
	//	res = getMailBox_Mat(srcmat, dstmat, applid_rotate, boxSizeThreshold, binaryThreshold);
	//}
	//else
	//{
	//	res = getMailBox_side(srcmat, dstmat, boxSizeThreshold, binaryThreshold);
	//}

#ifdef CUT_PARCEL_BOX_DEBUG
	cv::waitKey(0);
#endif // CUT_PARCEL_BOX_DEBUG

	
	if (res == 0) return 0;
	cv::imwrite(dstImgPath, dstmat);
	return 1;
}

int CutParcelBox::getMailBox_c(const char* pSrcImgPath, const char* pDstImgPath, int applid_rotate /*= 1*/, double boxSizeThreshold /*= 50*/, double binaryThreshold /*= 50*/, int is_top)
{
	std::string srcimg(pSrcImgPath);
	std::string dstimg(pDstImgPath);
	return getMailBox(srcimg, dstimg, applid_rotate, boxSizeThreshold, binaryThreshold,is_top);
}

int CutParcelBox::getMailBox_RtRect(cv::Mat &srcMat, cv::RotatedRect &dstRtRect, double boxSizeThreshold /*= 50*/, double binaryThreshold /*= 50*/)
{
	cv:: Mat binaryImg, src_gray, rsd_img;//彩色图像转化成灰度图  
	//src_img = imread(srcImgPath);
	if (srcMat.empty()) {
#ifdef CUT_PARCEL_BOX_DEBUG
		std::cout << "image mat is empty!" << std::endl;
#endif // CUT_PARCEL_BOX_DEBUG

		return 0;
	}
	//imshow("原始图像", src_img);
	//waitKey(0);
	if (srcMat.channels() == 3)
	{
		cvtColor(srcMat, src_gray, cv::COLOR_BGR2GRAY);
	}
	int ih = src_gray.rows;
	int iw = src_gray.cols;
	//int maxsize = (iw > ih) ? iw : ih;
	float scal_max = 512.0 / std::max(src_gray.cols, src_gray.rows);
	float scal_min = 256.0 / std::min(src_gray.cols, src_gray.rows);

	float scal = std::max(scal_max, scal_min);

	//double scal = 1024.0 / maxsize;
	iw = iw * scal;
	ih = ih * scal;
	cv::resize(src_gray, binaryImg, cv::Size(int(iw), int(ih)), 0, 0, cv::INTER_CUBIC);

	//imshow("original", src_gray);
	cv::normalize(binaryImg, binaryImg, 0, 255, cv::NORM_MINMAX);


	int thresh = CutParcelBox::adptive_threshold(binaryImg, 0.01);
	

#ifdef CUT_PARCEL_BOX_DEBUG
	std::cout << "adptive thresh:" << thresh << std::endl;
#endif // CUT_PARCEL_BOX_DEBUG


	//
	cv::Mat edgeImg;
	////medianBlur(src_gray, src_gray, 3);
	cv::threshold(binaryImg, rsd_img, thresh, 255, CV_THRESH_BINARY);

#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_src", binaryImg);
	imshow("cut_parcel_threshold", rsd_img);
#endif // CUT_PARCEL_BOX_DEBUG



	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(13, 13));
	cv::morphologyEx(rsd_img, rsd_img, cv::MORPH_CLOSE, element);

	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(8, 8));
	morphologyEx(rsd_img, rsd_img, cv::MORPH_ERODE, element);

	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(4, 4));
	morphologyEx(rsd_img, rsd_img, cv::MORPH_DILATE, element);

#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_morphology", rsd_img);
#endif // CUT_PARCEL_BOX_DEBUG
	//imshow("二值化", rsd_img);
	//waitKey(0);

	std::vector<std::vector<cv::Point>>contours;
	std::vector<cv::Vec4i>hierarchy;
	std::vector<cv::Point> contour;
	double aera = 0;
	//src_gray = src_gray > 100;
	cv::findContours(rsd_img, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

	//绘制轮廓图
	//cv::Mat dstImage = cv::Mat::zeros(rsd_img.size(), CV_8UC3);
	std::vector<double> vec_eare;
	cv::Rect m_Rect;
	cv::Rect best_Rect;
	std::vector<cv::Point> best_contour;
	bool isgoodRect = false;
	int index_hierc = -1;
	int index_contour = -1;

	int index_second_contour = -1;
	double second_aera = 0;

	//Scalar color = Scalar(rand() % 255, rand() % 255, rand() % 255);
	//drawContours(dstImage, contours, i, color, CV_FILLED, 8, hierarchy);
	for (int j = 0; j< contours.size(); j++)
	{
		double maera = cv::contourArea(contours[j]);
		if (maera > aera)
		{
			if (findRect(contours[j], m_Rect) == 0) continue;
			
			float raio = float(m_Rect.width) / m_Rect.height;

			if (raio < 0.5 || raio > 2) continue;

			//cv::rectangle(dstImage, m_Rect, Scalar(rand() % 255, rand() % 255, rand() % 255));
			//if (m_Rect.x > 2.0*iw / 3.0) continue;
			//if (m_Rect.x + m_Rect.width < iw / 3) continue;
			//if (m_Rect.y > 2 * ih / 3) continue;
			//if (m_Rect.y + m_Rect.height < ih / 3) continue;
			//best_Rect = m_Rect;
			index_second_contour = index_contour;
			index_contour = j;
			isgoodRect = true;
			second_aera = aera;
			aera = maera;
		}
	}


	if (index_second_contour!=-1 && second_aera > 100)
	{
		if (second_aera/aera > 0.5)
		{
#ifdef CUT_PARCEL_BOX_DEBUG
			std::cout << "find two tags"<<std::endl;
#endif // CUT_PARCEL_BOX_DEBUG
			return 0;
		}
	}


	//cv::rectangle(dstImage, m_Rect, Scalar(rand() % 255, rand() % 255, rand() % 255));
	//imshow("rectangle", dstImage);
	//std::vector<cv::Point> best_contour;
	double aera_in_image = (aera / scal) / (srcMat.cols*srcMat.rows);
	aera_in_image *= 1000.0;//按照千分比

#ifdef CUT_PARCEL_BOX_DEBUG
	if (index_contour <0 || aera_in_image <= boxSizeThreshold)
	{
		std::cout << "box aera percent:" << aera_in_image << std::endl;
		std::cout << "no box, or box area is smaller than threshold"<< std::endl;
	}
	else
	{
		std::cout << "box aera percent:" << aera_in_image << std::endl;
	}
#endif // CUT_PARCEL_BOX_DEBUG

	if (index_contour>=0 && aera_in_image > boxSizeThreshold)
	{
		cv::RotatedRect rrc;
		rrc = cv::minAreaRect(contours[index_contour]);
		m_Rect.width += 12;
		m_Rect.height += 12;

		rrc.center.x /= scal;
		rrc.center.y /= scal;
		rrc.size.width /= scal;
		rrc.size.height /= scal;
		dstRtRect = rrc;
		return 1;
	}
	else
	{
		return 0;
	}
	
}

int CutParcelBox::getMailBox_Mat(cv::Mat &srcMat, cv::Mat &dstMat, int applid_rotate /*= 1*/, double boxSizeThreshold /*= 50*/, double binaryThreshold /*= 50*/)
{
	cv::RotatedRect rRc;
	int res = getMailBox_RtRect(srcMat, rRc, boxSizeThreshold, binaryThreshold);
	if (res == 0) return 0;
	cv::Mat rMat;

	if (applid_rotate)
	{
		getMatFromRotateRect(srcMat, rMat, rRc);
	}
	else       
	{
		cv::Rect mR = rRc.boundingRect();
		CropRect(cv::Rect(0, 0, srcMat.cols, srcMat.rows), mR);
		rMat = srcMat(mR);
	}
	rMat.copyTo(dstMat);

	return check_parcel(dstMat);

	return 1;
}




int CutParcelBox::getMailBox_side(cv::Mat &srcmat, cv::Rect &dstRect, double boxSizeThreshold /*= 50*/, double binaryThreshold /*= 5*/)
{

	if (srcmat.empty())
	{
		std::cout << "图像为空" << std::endl;
		return 0;
	}

	float scal_max = 512.0 / std::max(srcmat.cols, srcmat.rows);
	float scal_min = 256.0 / std::min(srcmat.cols, srcmat.rows);

	float scal = std::max(scal_max, scal_min);
	cv::Mat resizedMat;
	cv::resize(srcmat, resizedMat, cv::Size(), scal, scal);
	if (resizedMat.channels() == 3)
	{
		cv::cvtColor(resizedMat, resizedMat, CV_BGR2GRAY);
	}
	cv::normalize(resizedMat, resizedMat, 0, 255, cv::NORM_MINMAX);


	int iw = resizedMat.cols;
	int ih = resizedMat.rows;
	//imshow("resized", resizedMat);
	cv::Mat sobelmat;
	cv::Sobel(resizedMat, sobelmat, CV_8U, 1, 1);
#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_side_sobel", sobelmat);
#endif // CUT_PARCEL_BOX_DEBUG

	cv::threshold(sobelmat, sobelmat, 5, 255, cv::THRESH_BINARY);

#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_side_threshold", sobelmat);
#endif // CUT_PARCEL_BOX_DEBUG

	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(13, 13));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_CLOSE, element);
	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 8));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_ERODE, element);
	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_ERODE, element);
	//imshow("sobel", sobelmat);

#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_side_morphology", sobelmat);
#endif // CUT_PARCEL_BOX_DEBUG


	std::vector<std::vector<cv::Point>>contours;
	std::vector<cv::Vec4i> hierarchy;
	std::vector<cv::Point> contour;
	double aera = 0;
	//src_gray = src_gray > 100;
	cv::findContours(sobelmat, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

	std::vector<double> vec_eare;
	cv::Rect m_Rect;
	cv::Rect best_Rect;
	//std::vector<cv::Point> best_contour;
	bool isgoodRect = false;
	int index_hierc = -1;
	int index_contour = -1;

	//Scalar color = Scalar(rand() % 255, rand() % 255, rand() % 255);
	//drawContours(dstImage, contours, i, color, CV_FILLED, 8, hierarchy);
	for (int j = 0; j < contours.size(); j++)
	{
		if (contours[j].size() <= 2) continue;
		double maera = cv::contourArea(contours[j]);
		if (maera > aera)
		{
			index_contour = j;
			isgoodRect = true;
			aera = maera;
		}
	}
	

	double area_in_image = (aera / scal) / (srcmat.rows*srcmat.cols);
	area_in_image *= 1000;//按照千分比
	//std::cout << "面积================" << area_in_image << std::endl;


	//cv::rectangle(dstImage, m_Rect, Scalar(rand() % 255, rand() % 255, rand() % 255));
	//imshow("rectangle", dstImage);
	//std::vector<cv::Point> best_contour;
	if (index_contour >= 0 && area_in_image >= boxSizeThreshold)//需要目标面积大于图片面积的千分之一。
	{

		findRect(contours[index_contour], m_Rect);
		m_Rect.width += 24;
		m_Rect.height += 24;
		cv::Rect sRec;
		sRec.x = m_Rect.x / scal;
		sRec.y = m_Rect.y / scal;
		sRec.width = m_Rect.width / scal;
		sRec.height = m_Rect.height / scal;
		dstRect = sRec;
		return 1;
	}
	else
	{
		return 0;
	}



	//绘制轮廓图
	//std::vector<double> vec_eare;
	//cv::Rect m_Rect;
	//cv::Rect best_Rect;
	//std::vector<cv::Point> best_contour = contours[0];
	//findRect(best_contour, m_Rect);
	//cv::rectangle(resizedMat, m_Rect, cv::Scalar(255, 255, 255));

	//imshow("box", resizedMat);
	//dstMat = resizedMat;

}

int CutParcelBox::getMailBox_side(cv::Mat &srcMat, cv::Mat &dstMat, double boxSizeThreshold /*= 1*/, double binaryThreshold /*= 5*/)
{
	cv::Mat parcel;
	cv::Rect rect_;
#ifdef CUT_PARCEL_BOX_DEBUG
	cv::Mat sm;
	cv::resize(srcMat, sm, cv::Size(), 0.2, 0.2);
	cv::imshow("ssrc", sm);
#endif // CUT_PARCEL_BOX_DEBUG

	int res = getMailBox_side(srcMat, rect_, boxSizeThreshold, binaryThreshold);
	if (res!=0)
	{
		cv::Rect mainrc;
		mainrc.height = srcMat.rows;
		mainrc.width = srcMat.cols;
		if (CropRect(mainrc, rect_))
		{
			srcMat(rect_).copyTo(parcel);
			res = getMailBox_Mat(parcel, dstMat, 1, boxSizeThreshold, binaryThreshold);
		}

	}
	return res;
}

CutParcelBox::CutParcelBox()
{
	
}

int CutParcelBox::getMailBox_Rect_Raw(const void *pImgData, int iw, int ih, int ic, 
	int *RectPoints, int applid_rotate/*=1*/, double boxSizeThreshold /*= 50*/, double binaryThreshold /*= 50*/)
{
	cv::Mat srcmat; 
	if (ic==0)
	{
		srcmat = cv::Mat(ih, iw, CV_8U, (void*)pImgData);
	}
	else if (ic==3)
	{
		srcmat = cv::Mat(ih, iw, CV_8UC3, (void*)pImgData);
	}
	else
	{
		return 0;
	}
	cv::RotatedRect rRc;
	int res = getMailBox_RtRect(srcmat, rRc, boxSizeThreshold, boxSizeThreshold);
	if (res == 0) return 0;
	if (applid_rotate)
	{
		cv::Point2f pts[4];
		rRc.points(pts);
		for (int i = 0; i < 4; i++)
		{
			RectPoints[2 * i + 0] = pts[i].x;
			RectPoints[2 * i + 1] = pts[i].y;
		}
	}
	else
	{
		cv::Rect mr = rRc.boundingRect();
		RectPoints[0] = mr.x;
		RectPoints[1] = mr.y;
		RectPoints[2] = mr.x + mr.width;
		RectPoints[3] = mr.x + mr.height;
	}

	return 1;
}

int CutParcelBox::findRect(std::vector<cv::Point> contour, cv::Rect &mrect)
{
	int mtop = 0;
	int mbottom = 0;
	int mleft = 0;
	int mright = 0;
	if (contour.size() < 1) return 0;
	mleft = contour[0].x;
	mright = mleft;
	mtop = contour[0].y;
	mbottom = mtop;
	int x = 0;
	int y = 0;
	for (int i = 0; i < contour.size(); i++)
	{
		x = contour[i].x;
		y = contour[i].y;

		mleft = (mleft < x) ? mleft : x;
		mright = (mright < x) ? x : mright;

		mtop = (mtop < y) ? mtop : y;
		mbottom = (mbottom < y) ? y : mbottom;
	}
	mrect = cv::Rect(cv::Point(mleft, mtop), cv::Point(mright, mbottom));
	return 1;
}

int CutParcelBox::getMatFromRotateRect(const cv::Mat &src_mat, cv::Mat &dst_mat, cv::RotatedRect rRc)
{
	cv::Rect boxRec = rRc.boundingRect();
	//cv::Rect boxRec_n(0, 0, boxRec.width, boxRec.height);
	
	cv::Mat padMat = cv::Mat::zeros(boxRec.size(),src_mat.type());
	padMat = ~padMat;
	cv::Rect cropRect = boxRec;
	int res = CutParcelBox::CropRect(cv::Rect(0, 0, src_mat.cols, src_mat.rows), cropRect);
	if (!res) return 0;
	cv::Mat boxMat = src_mat(cropRect);
	cv::Rect copyRect(cropRect.x - boxRec.x, cropRect.y - boxRec.y, cropRect.width, cropRect.height);
	boxMat.copyTo(padMat(copyRect));
	int max_lenth = std::max(padMat.cols, padMat.rows);
	
	cv::Mat padMat_e = cv::Mat::zeros(cv::Size(max_lenth, max_lenth), src_mat.type());
	padMat_e = ~padMat_e;
	cv::Rect rec_e;
	rec_e.x = (padMat.cols > padMat.rows) ? 0 : (padMat.rows - padMat.cols) / 2;
	rec_e.y = (padMat.cols > padMat.rows) ? (padMat.cols - padMat.rows) / 2 : 0;
	rec_e.width = padMat.cols;
	rec_e.height = padMat.rows;

	padMat.copyTo(padMat_e(rec_e));

	cv::Mat affine_matrix = cv::getRotationMatrix2D(cv::Point2f(padMat_e.cols/2.0f, padMat_e.rows/2.0f), rRc.angle, 1.0);//求得旋转矩阵    
	cv::warpAffine(padMat_e, padMat_e, affine_matrix, padMat_e.size());     //计算图像旋转之后包含图像的最大的矩形    

	cv::Rect cropRec;
	cropRec.x = padMat_e.cols / 2 - rRc.size.width / 2;
	cropRec.y = padMat_e.rows / 2 - rRc.size.height / 2;
	cropRec.width = rRc.size.width;
	cropRec.height = rRc.size.height;
	CutParcelBox::CropRect(cv::Rect(0, 0, padMat_e.cols, padMat_e.rows), cropRec);

	dst_mat = padMat_e(cropRec);

	return 1;
}

int CutParcelBox::CropRect(cv::Rect main_rect, cv::Rect &to_crop_rect)
{
	if ((to_crop_rect.x + to_crop_rect.width) <= main_rect.x) return 0;
	if ((to_crop_rect.y + to_crop_rect.height) <= main_rect.y) return 0;
	if (to_crop_rect.x >= (main_rect.width + main_rect.x)) return 0;
	if (to_crop_rect.y >= (main_rect.height + main_rect.y)) return 0;
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

unsigned int getHash(const cv::Mat& image) {
	int table_size = 64;
	//Create a 8x8 version of our image.
	//Don't worry about it scaling correctly.
	cv::Mat smaller, gray;
	cv::resize(image, smaller, cv::Size(8, 8));
	cv::cvtColor(smaller, gray, CV_BGR2GRAY);
	// Calculate the scalar average of the pixels;
	double avg = mean(gray)[0];
	// The largest possible size is somewhere bigger than 32 bits
	// and smaller than 64 bits.
	unsigned long long hash_val = 0;
	// If the
	for (int x = 0; x < 8; x++) {
		for (int y = 0; y < 8; y++) {
			hash_val <<= 1;
			// If the greyscale value of a pixel is greater than the average
			// Put a 1 in that bit. Otherwise have it be a 0
			hash_val |= 1 * (static_cast<double>(gray.at<uchar>(x, y)) >= avg);
		}
	}
	return hash_val;
}



int CutParcelBox::check_parcel(cv::Mat &srcm)
{
	using namespace ZXing;
	
	if (srcm.empty()) return 0;
	if (srcm.rows < srcm.cols)
		cv::rotate(srcm, srcm, cv::ROTATE_90_CLOCKWISE);
	
	float hw_ratio = float(srcm.rows) / srcm.cols;
	

#ifdef CUT_PARCEL_BOX_DEBUG
	std::cout << "hw_ratio:" << hw_ratio << std::endl;
	std::cout << "max size:" << std::max(srcm.cols, srcm.rows) << std::endl;
#endif // CUT_PARCEL_BOX_DEBUG
	int mlength = std::max(srcm.cols, srcm.rows);
	if ((hw_ratio < 1. || hw_ratio > 2) || (mlength < 500 || mlength>1800))
	{
#ifdef CUT_PARCEL_BOX_DEBUG
		std::cout << "warning, tag ratio is exception" << std::endl;
#endif // CUT_PARCEL_BOX_DEBUG
		return 0;
	}


	cv::Mat img = srcm.clone();
	cv::Mat eimg=img;
	if(eimg.channels()==3)
		cv::cvtColor(eimg,eimg,cv::COLOR_BGR2GRAY);

#ifdef CUT_PARCEL_BOX_DEBUG
	cv::imshow("checker", eimg);
#endif // CUT_PARCEL_BOX_DEBUG


    // ZXing barcode detect
	/*
	ZXing::DecodeHints hints;
	hints.setTryRotate(true);
	hints.setTryHarder(true);
	hints.setBinarizer(ZXing::Binarizer::GlobalHistogram);
	hints.setIsPure(false);
	try
	{
		auto result = ZXing::ReadBarcode({ srcm.data, srcm.cols, srcm.rows, ZXing::ImageFormat::BGR }, hints);
		
		if (std::string(ZXing::ToString(result.status())) == "NoError")
		{
			
#ifdef CUT_PARCEL_BOX_DEBUG
			std::cout << result. << std::endl;
			std::cout << "条码检测成功" << std::endl;
#endif // CUT_PARCEL_BOX_DEBUG
			return 1;
		}
	}
	catch (...)
	{
#ifdef CUT_PARCEL_BOX_DEBUG
		std::cout << "条码检测出现异常" << std::endl;
#endif // CUT_PARCEL_BOX_DEBUG
	}

	*/


	cv::Mat cm;
	float scal_ = 512.0 / std::max(eimg.rows, eimg.cols);
	cv::resize(eimg, eimg, cv::Size(), scal_, scal_);
	cv::medianBlur(eimg, eimg, 3);
	cv::normalize(eimg, eimg, 0, 255, cv::NORM_MINMAX);



	cv::Scalar     mean;
	cv::Scalar     stddev;

	cv::meanStdDev(eimg, mean, stddev);
	double       mean_pxl = mean.val[0];
	double       stddev_pxl = stddev.val[0];



	//cv::Scalar sc = cv::mean(eimg);


#ifdef CUT_PARCEL_BOX_DEBUG
	std::cout << "pix mean:" << mean_pxl << std::endl;
	std::cout << "pix stdvar:" << stddev_pxl << std::endl;
#endif // CUT

	if (mean_pxl < 100)
	{
#ifdef CUT_PARCEL_BOX_DEBUG
		std::cout << "warning, pix mean is lower than 100:" << std::endl;
#endif // CUT
		return 0;
	}
	if (stddev_pxl< 45)
	{
#ifdef CUT_PARCEL_BOX_DEBUG
		std::cout << "warning, pix stddev is lower than 45:" << std::endl;
#endif // CUT
		return 0;
	}



	cv::Canny(eimg,cm, 50, 250);

	cv::meanStdDev(cm, mean, stddev);
	mean_pxl = mean.val[0];
	stddev_pxl = stddev.val[0];

#ifdef CUT_PARCEL_BOX_DEBUG
	std::cout << "canny mean:" << mean_pxl << std::endl;
	std::cout << "canny dev:" << stddev_pxl << std::endl;
#endif // CUT_PARCEL_BOX_DEBUG

	

	if (mean_pxl < 18 || mean_pxl > 45)
	{
#ifdef CUT_PARCEL_BOX_DEBUG
		std::cout << "warning, canny mean is exception:" << std::endl;
#endif // CUT
		return 0;
	}

	if (stddev_pxl< 65)
	{
#ifdef CUT_PARCEL_BOX_DEBUG
		std::cout << "warning, canny dev is exception:" << std::endl;
#endif // CUT
		return 0;
	}


	//方差
	//


	//cv::imshow("canny_pre", eimg);
	//cv::imshow("canny", cm);









	//std::cout << getHash(img)<<std::endl;

	////cv::Mat hsv;
	////cv::cvtColor(srcm, hsv, cv::COLOR_BGR2HSV);
	//// Quantize the hue to 30 levels
	//// and the saturation to 32 levels
	//int hbins = 30, sbins = 32;
	//int histSize[] = { hbins, sbins };
	//// hue varies from 0 to 179, see cvtColor
	//float hranges[] = { 0, 256 };
	//// saturation varies from 0 (black-gray-white) to
	//// 255 (pure spectrum color)
	//float sranges[] = { 0, 256 };
	//const float* ranges[] = { hranges, sranges };
	//cv::MatND hist;
	//// we compute the histogram from the 0-th and 1-st channels
	//int channels[] = { 0, 1 };
	//calcHist(&srcm, 1, channels, cv::Mat(), // do not use mask
	//	hist, 2, histSize, ranges,
	//	true, // the histogram is uniform
	//	false);

	//std::cout << hist.size << std::endl;
	//std::cout << cv::typeToString(hist.type()) << std::endl;

	//cv::normalize(hist, hist, 0, 512, cv::NORM_MINMAX, -1, cv::Mat());



	//cv::FileStorage fs("hist.xml", cv::FileStorage::READ);
	//cv::Mat prmat;
	//fs["hist"] >> prmat;
	//fs.release();

	//cv::Scalar difm = cv::mean(cv::abs(prmat - hist));
	//std::cout << difm << std::endl;
	
	//hist = 0.8*prmat + 0.2*hist;
	//
	//cv::FileStorage fs2("hist.xml", cv::FileStorage::WRITE);
	//fs2 << "hist" << hist;
	//fs2.release();
	//


}

int CutParcelBox::adptive_threshold(cv::Mat &srcm, float _percent)
{
	cv::Mat m = srcm;
	if (srcm.channels() == 3)
	{
		cv::cvtColor(srcm, m, cv::COLOR_BGR2GRAY);
	}

	int histSize[1] = { 256 };
	float hranges[2] = { 0,256 };
	const float* ranges[1] = { hranges };
	int channels[1] = { 0 };



	cv::Mat hist;
	cv::calcHist(&m,
		1,//仅为一个图像的直方图
		channels,//使用的通道
		cv::Mat(),//不使用掩码
		hist,//作为结果的直方图
		1,//这时一维的直方图
		histSize,//箱子数量
		ranges//像素值的范围
	);
	float whole_pix = srcm.cols*srcm.rows;
	float acc_pix = 0;
	int thresh_ = 255;
	for (int i=255;i>=0;i--)
	{
		float val = hist.at<float>(i);
		acc_pix += val;
		if (acc_pix > whole_pix*_percent)
		{
			thresh_ = i;
			break;
		}

	}
	return thresh_;

}
