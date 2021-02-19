
//#include "stdafx.h"
#include "CutParcelBox_thp.h"
#include <algorithm>
#include <iostream>
#include "ImageProcessFunc.h"
using namespace cv;

int CutParcelBox::getMailBox(std::string &srcImgPath, std::string &dstImgPath, int applid_rotate /*= 1*/, double boxSizeThreshold/*=50*/, double binaryThreshold/*=50*/)
{
	cv::Mat srcmat = cv::imread(srcImgPath);
	cv::Mat dstmat;
	int res = getMailBox_Mat(srcmat, dstmat, applid_rotate, boxSizeThreshold, binaryThreshold);
	if (res == 0) return 0;
	cv::imwrite(dstImgPath, dstmat);
	return 1;
}

int CutParcelBox::getMailBox_c(const char* pSrcImgPath, const char* pDstImgPath, int applid_rotate /*= 1*/, double boxSizeThreshold /*= 50*/, double binaryThreshold /*= 50*/)
{
	std::string srcimg(pSrcImgPath);
	std::string dstimg(pDstImgPath);
	return getMailBox(srcimg, dstimg, applid_rotate, boxSizeThreshold, binaryThreshold);
}

int CutParcelBox::getMailBox_RtRect(cv::Mat &srcMat, cv::RotatedRect &dstRtRect, double boxSizeThreshold /*= 50*/, double binaryThreshold /*= 50*/)
{
	cv:: Mat binaryImg, src_gray, rsd_img;//彩色图像转化成灰度图  
	//src_img = imread(srcImgPath);
	if (srcMat.empty()) return 0;
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
	cv::resize(src_gray, rsd_img, cv::Size(int(iw), int(ih)), 0, 0, cv::INTER_CUBIC);

	//imshow("original", src_gray);

	//
	cv::Mat edgeImg;
	////medianBlur(src_gray, src_gray, 3);
	cv::threshold(rsd_img, rsd_img, binaryThreshold, 255, cv::THRESH_BINARY);

#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_threshold", rsd_img);
#endif // CUT_PARCEL_BOX_DEBUG



	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(8, 8));
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
	cv::findContours(rsd_img, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

	//绘制轮廓图
	//cv::Mat dstImage = cv::Mat::zeros(rsd_img.size(), CV_8UC3);
	std::vector<double> vec_eare;
	cv::Rect m_Rect;
	cv::Rect best_Rect;
	std::vector<cv::Point> best_contour;
	bool isgoodRect = false;
	int index_hierc = -1;
	int index_contour = -1;

	//Scalar color = Scalar(rand() % 255, rand() % 255, rand() % 255);
	//drawContours(dstImage, contours, i, color, CV_FILLED, 8, hierarchy);
	for (int j = 0; j< contours.size(); j++)
	{
		double maera = cv::contourArea(contours[j]);
		if (maera > aera)
		{
			if (findRect(contours[j], m_Rect) == 0) continue;
			//cv::rectangle(dstImage, m_Rect, Scalar(rand() % 255, rand() % 255, rand() % 255));
			if (m_Rect.x > 2.0*iw / 3.0) continue;
			if (m_Rect.x + m_Rect.width < iw / 3) continue;
			if (m_Rect.y > 2 * ih / 3) continue;
			if (m_Rect.y + m_Rect.height < ih / 3) continue;
			//best_Rect = m_Rect;
			index_contour = j;
			isgoodRect = true;
			aera = maera;
		}
	}

	//cv::rectangle(dstImage, m_Rect, Scalar(rand() % 255, rand() % 255, rand() % 255));
	//imshow("rectangle", dstImage);
	//std::vector<cv::Point> best_contour;
	double aera_in_image = (aera / scal) / (srcMat.cols*srcMat.rows);
	aera_in_image *= 1000.0;//按照千分比
	if (index_contour>=0 && aera_in_image > boxSizeThreshold)
	{
		cv::RotatedRect rrc;
		rrc = cv::minAreaRect(contours[index_contour]);
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
		cv::cvtColor(resizedMat, resizedMat, cv::COLOR_BGR2GRAY);
	}
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

	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(8, 8));
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
	cv::findContours(sobelmat, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

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
	cv::Rect rect_;
	int res = getMailBox_side(srcMat, rect_, boxSizeThreshold, binaryThreshold);
	if (res==1)
	{
		 srcMat(rect_).copyTo(dstMat);
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
	cv::Rect cropRect = boxRec;
	int res = CutParcelBox::CropRect(cv::Rect(0, 0, src_mat.cols, src_mat.rows), cropRect);
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

float tray_flatten_score(const cv::Mat &srcm,const int tray_pix)
{
	cv::Mat smat;
	cv::medianBlur(srcm, smat, 3);
	cv::Scalar mean_p =  cv::mean(smat);
	double minv = 0.0, maxv = 0.0;
	double* minp = &minv;
	double* maxp = &maxv;
	cv::minMaxIdx(srcm, minp, maxp);
	double difm = std::abs(tray_pix - mean_p.val[0]);

	//std::cout << "meanp:" << mean_p.val[0] << std::endl;

	double range_scal = std::exp(-std::pow((maxv - minv + difm) / 40, 2) / 1);

	return range_scal;
}



int CutParcelBox::findTrayRect(cv::Mat srcm, cv::Rect &dstrct, float lf_edge, float rt_edge, int tray_pix)
{
	if (srcm.empty())
	{
		return 0;
	}

	lf_edge = (lf_edge < 0) ? 0 : lf_edge;
	lf_edge = (lf_edge > 1) ? 1 : lf_edge;
	rt_edge = (rt_edge < 0) ? 0 : rt_edge;
	rt_edge = (rt_edge > 1) ? 1 : rt_edge;

	if (lf_edge > rt_edge)
	{
		dstrct  = cv::Rect(0,0,srcm.cols,srcm.rows);
		return 1;
	}


	cv::Mat smat;

	const float hsize = 1024;
	float scal = hsize / srcm.cols;
	cv::resize(srcm, smat, cv::Size(), scal, scal);
	//cv::imshow("s", smat);

	if (smat.channels()==3)
	{
		cv::cvtColor(smat, smat, cv::COLOR_BGR2GRAY);
	}

	cv::Mat smp_mat_lf, smp_mat_rt;
	cv::Rect lf_rct, rt_rct;

	//确定左侧采样区域
	int tray_width = smat.cols*(rt_edge - lf_edge);
	lf_rct.x = smat.cols*(lf_edge + 0.02);
	lf_rct.width = tray_width * 0.1;
	lf_rct.height = tray_width * 0.4;
	lf_rct.y = smat.rows / 2 - lf_rct.height / 2;
	lf_rct.y = (lf_rct.y < 0) ? 0 : lf_rct.y;

	//确定右侧采样区域
	rt_rct = lf_rct;
	rt_rct.x = smat.cols*rt_edge - 20 - rt_rct.width;
	rt_rct.x = (rt_rct.x < 0) ? 0 : rt_rct.x;

	ImageProcessFunc::CropRect(cv::Rect(0, 0, smat.cols, smat.rows), lf_rct);
	ImageProcessFunc::CropRect(cv::Rect(0, 0, smat.cols, smat.rows), rt_rct);



	float lf_score = (lf_rct.width==0||lf_rct.height==0)?0:tray_flatten_score(smat(lf_rct), tray_pix);
	float rt_score = (rt_rct.width == 0 || rt_rct.height == 0) ? 0 : tray_flatten_score(smat(rt_rct), tray_pix);

	//std::cout << "lf_score:" << lf_score << std::endl;
	//std::cout << "rt_score:" << rt_score << std::endl;

// 	cv::imshow("lf", smat(lf_rct));
// 	cv::imshow("rt", smat(rt_rct));


	if (lf_score<0.01 && rt_score<0.01)
	{
		dstrct = cv::Rect(cv::Point(srcm.cols*lf_edge, 0), cv::Point(srcm.cols*rt_edge, srcm.rows - 1));
		return 1;
	}

	//
	cv::Rect srect = (lf_score < rt_score) ? rt_rct : lf_rct;
	cv::Scalar meanv = cv::mean(smat(srect));


	srect.y = 0;
	srect.height = smat.rows;
	std::vector<unsigned int> vect_pix;
	ImageProcessFunc::sumPixels(smat(srect), 0, vect_pix);
	int norm_sum = meanv[0] * srect.width;
	int over_thresh = 30 * srect.width;
	int start_y = 0;
	int end_y = smat.rows - 1;
	for (int i = vect_pix.size() / 2; i > 0; i--)
	{
		if (vect_pix[i]>(norm_sum + over_thresh))
		{
			start_y = i;
			break;
		}
	}
	for (int i = vect_pix.size() / 2; i < vect_pix.size(); i++)
	{
		if (vect_pix[i] > (norm_sum+ over_thresh))
		{
			end_y = i;
			break;
		}
	}


	srect.y = start_y + 20;
	srect.height = end_y - start_y - 40;
	if (srect.height <= 0) srect.height = 0;

	srect.x = lf_rct.x;
	srect.width = rt_rct.x + rt_rct.width - srect.x;
	if (srect.width <= 0) srect.width = 0;

	srect.x /= scal;
	srect.width /= scal;
	srect.y /= scal;
	srect.height /= scal;

	ImageProcessFunc::CropRect(cv::Rect(0, 0, srcm.cols, srcm.rows), srect);
	if (srect.width<=0 || srect.height<=0)
	{
		dstrct = cv::Rect(cv::Point(srcm.cols*lf_edge, 0), cv::Point(srcm.cols*rt_edge, srcm.rows - 1));
	}
	else
	{
		dstrct = srect;
		
	}
	return 1;


}

int CutParcelBox::_cutParcel_from_tray(const cv::Mat &srcm, cv::Mat &dstm, int tray_pix, int apply_rotate)
{
	cv::Mat smat;
	if (srcm.empty())
	{
		return 0;
	}
	const float hsize = 720;
	float scal = hsize / srcm.cols;
	cv::resize(srcm, smat, cv::Size(), scal, scal);
	if (smat.channels() == 3)
	{
		cv::cvtColor(smat, smat, cv::COLOR_BGR2GRAY);
	}
	cv::Mat blurmat;
	cv::medianBlur(smat, blurmat, 3);

#ifdef CUT_PARCEL_BOX_DEBUG
	cv::imshow("sblur", blurmat);
#endif // CUT_PARCEL_BOX_DEBUG

	//

	cv::Mat binary_up;
	cv::threshold(blurmat, binary_up,tray_pix + 30,255,cv::THRESH_BINARY);
	
	cv::Mat edge_mat;
	cv::Canny(blurmat, edge_mat,5,35);
	

	cv::Mat element = cv::getStructuringElement(MORPH_RECT, Size(3, 3));
	morphologyEx(binary_up, binary_up, MORPH_DILATE, element);

	morphologyEx(edge_mat, edge_mat, MORPH_DILATE, element);


	element = cv::getStructuringElement(MORPH_RECT, Size(5, 5));
	morphologyEx(edge_mat, edge_mat, MORPH_CLOSE, element);
	morphologyEx(binary_up, binary_up, MORPH_CLOSE, element);

#ifdef CUT_PARCEL_BOX_DEBUG
	cv::imshow("cut_bina", binary_up);
	cv::imshow("cut_edge", edge_mat);
#endif // CUT_PARCEL_BOX_DEBUG

	cv::Mat dmat = binary_up | edge_mat;


	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(dmat, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	int maxind = -1;
	double maxarea = 0;

	for (int i = 0; i < contours.size(); i++)
	{
		double marea = contourArea(contours[i]);
		if (maxarea < marea)
		{
			maxind = i;
			maxarea = marea;
		}
	}

	if (maxind == -1) return 0;
	if (maxarea < smat.cols*smat.rows*0.01)
	{
		return 0;
	}
	//bool apply_rotate = false;
	if (apply_rotate)
	{
		cv::RotatedRect rbox = cv::minAreaRect(contours[maxind]);
		rbox.center.x /= scal;
		rbox.center.y /= scal;
		rbox.size.width /= scal;
		rbox.size.height /= scal;
		ImageProcessFunc::getMatFromRotatedRect(srcm, dstm, rbox, 125);
	}
	else
	{
		cv::Rect rc;
		ImageProcessFunc::getContourRect(contours[maxind], rc);
		rc.x /= scal;
		rc.y /= scal;
		rc.width /= scal;
		rc.height /= scal;

		ImageProcessFunc::CropRect(cv::Rect(0, 0, srcm.cols, srcm.rows), rc);
		srcm(rc).copyTo(dstm);
	}

	return 1;
}

int CutParcelBox::cutParcelMat(cv::Mat srcm, cv::Mat &parcel, float lf_edge, float rt_edge,\
	int tray_pix, int image_view, int applid_rotate)
{
	int res = 0;
	if (image_view == 0) //top
	{
		cv::Rect tray_rc;
		res = CutParcelBox::_findTrayRect_gray(srcm, tray_rc, lf_edge, rt_edge, tray_pix);
		if (res)
		{
			cv::Mat tray_mat = srcm(tray_rc);
			//cv::imshow("tray", tray_mat);
			if (tray_rc.height < srcm.rows*0.95)//没有检测出托盘
			{
				res = CutParcelBox::_cutParcel_from_tray(tray_mat, parcel, tray_pix, applid_rotate);
			}
			else
			{
				tray_mat.copyTo(parcel);
				if (applid_rotate) res = 0;//如果应用旋转，必须要检测到托盘
			}
			
		}
	}
	else if(image_view == 1) //bottom
	{
		res = CutParcelBox::_cutParcel_bottom(srcm, parcel,applid_rotate);
	}
	else
	{
		res = CutParcelBox::_cutParcel_side(srcm, parcel);
	}

	return res;
}


int CutParcelBox::_findTrayRect_gray(const cv::Mat &srcm, cv::Rect &dstrct, float lf_edge, float rt_edge, int tray_pix)
{
	if (srcm.empty())
	{
		return 0;
	}

	lf_edge = (lf_edge < 0) ? 0 : lf_edge;
	lf_edge = (lf_edge > 1) ? 1 : lf_edge;
	rt_edge = (rt_edge < 0) ? 0 : rt_edge;
	rt_edge = (rt_edge > 1) ? 1 : rt_edge;

	if (lf_edge > rt_edge)
	{
		dstrct = cv::Rect(0, 0, srcm.cols, srcm.rows);
		return 1;
	}

	cv::Mat smat;

	const float hsize = 640;
	float scal = hsize / srcm.cols;
	cv::resize(srcm, smat, cv::Size(), scal, scal);
	//cv::imshow("s", smat);

	if (smat.channels() == 3)
	{
		cv::cvtColor(smat, smat, cv::COLOR_BGR2GRAY);
	}
	
	cv::medianBlur(smat, smat, 3);

	cv::Mat gradmat;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 3));

	cv::morphologyEx(smat, gradmat, cv::MORPH_GRADIENT, element, cv::Point(-1, -1), 1, cv::BORDER_REPLICATE);
	//
	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(11, 1));
	cv::morphologyEx(gradmat, gradmat, cv::MORPH_ERODE, element, cv::Point(-1, -1), 1, cv::BORDER_REPLICATE);
	cv::threshold(gradmat, gradmat, 10, 255, cv::THRESH_BINARY);
	cv::morphologyEx(gradmat, gradmat, cv::MORPH_ERODE, element, cv::Point(-1, -1), 1, cv::BORDER_REPLICATE);

	cv::Rect rc_sp(gradmat.cols*lf_edge,0,(rt_edge-lf_edge)*gradmat.cols, gradmat.rows);

#ifdef CUT_PARCEL_BOX_DEBUG
	cv::Mat showmat;
	gradmat.copyTo(showmat);
	cv::rectangle(showmat, rc_sp, cv::Scalar(125, 125, 125), 2);
	cv::imshow("sp_rec", showmat);
#endif // CUT_PARCEL_BOX_DEBUG

	std::vector<unsigned int> pixels;
	ImageProcessFunc::sumPixels(gradmat(rc_sp), 0, pixels);
	int low_x = 0;
	int high_x = pixels.size()-1;
	int thresh_pix = 120 * rc_sp.width;
	for (int i= pixels.size()/4;i>0;i--)
	{
		if (pixels[i]>thresh_pix)
		{
			low_x = i;
			break;
		}
	}
	for (int i = 3*pixels.size() / 4; i <pixels.size(); i++)
	{
		if (pixels[i] > thresh_pix)
		{
			high_x = i;
			break;
		}
	}
	cv::Rect rc_tray(gradmat.cols*lf_edge + 20, low_x + 10 , (rt_edge - lf_edge)*gradmat.cols - 40, high_x-low_x - 20);

	if (rc_tray.height < gradmat.rows*0.3)
	{
		rc_tray.y = 0;
		rc_tray.height = gradmat.rows;
	}

	rc_tray.x /= scal;
	rc_tray.y /= scal;
	rc_tray.height /= scal;
	rc_tray.width /= scal;
	ImageProcessFunc::CropRect(cv::Rect(0, 0, srcm.cols, srcm.rows), rc_tray);
	dstrct = rc_tray;
	return 1;

}

int CutParcelBox::_cutParcel_side(const cv::Mat &srcmat, cv::Mat &dstm, int is_bottom)
{

	if (srcmat.empty())
	{
		std::cout << "图像为空" << std::endl;
		return 0;
	}

	float scal = 480.0f / srcmat.cols;


	cv::Mat resizedMat;
	cv::resize(srcmat, resizedMat, cv::Size(), scal, scal);
	if (resizedMat.channels() == 3)
	{
		cv::cvtColor(resizedMat, resizedMat, cv::COLOR_BGR2GRAY);
	}
	int iw = resizedMat.cols;
	int ih = resizedMat.rows;
	//imshow("resized", resizedMat);
#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("resized_src", resizedMat);
#endif // CUT_PARCEL_BOX_DEBUG

	//cv::medianBlur(resizedMat, resizedMat,3);
	

	cv::Mat sobelmat;
// 	if (is_bottom)
// 	{
// 		cv::blur(resizedMat, sobelmat, cv::Size(1, 5),cv::Point(-1,-1),cv::BORDER_REPLICATE);
// 	}
// 	else
// 	{
// 		cv::Sobel(resizedMat, sobelmat, CV_8U, 1,1);
// 	}
	
#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_side_sobel", resizedMat);
#endif // CUT_PARCEL_BOX_DEBUG

	cv::Mat element;

	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 3));
		
	cv::morphologyEx(resizedMat, sobelmat, cv::MORPH_GRADIENT, element, cv::Point(-1, -1), 1, cv::BORDER_REPLICATE);
	//imshow("grident", sobelmat);
	cv::threshold(sobelmat, sobelmat, 8, 255, cv::THRESH_BINARY);

	

#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_side_threshold", sobelmat);
#endif // CUT_PARCEL_BOX_DEBUG
	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_DILATE, element);

	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_CLOSE, element);

	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 5));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_ERODE, element);

	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 1));
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
	cv::findContours(sobelmat, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	std::vector<double> vec_eare;
	cv::Rect m_Rect;
	cv::Rect best_Rect;
	//std::vector<cv::Point> best_contour;
	bool isgoodRect = false;
	int index_contour = -1;

	//Scalar color = Scalar(rand() % 255, rand() % 255, rand() % 255);
	//drawContours(dstImage, contours, i, color, CV_FILLED, 8, hierarchy);
	for (int j = 0; j < contours.size(); j++)
	{
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

	if (index_contour == -1) return 0;
	if (area_in_image < 1) return 0;

	if (is_bottom)
	{
		cv::RotatedRect rbox = cv::minAreaRect(contours[index_contour]);

		rbox.center.x /= scal;
		rbox.center.y /= scal;
		rbox.size.width /= scal;
		rbox.size.height /= scal;

		ImageProcessFunc::getMatFromRotatedRect(srcmat, dstm, rbox, 0);

	}
	else
	{
		CutParcelBox::findRect(contours[index_contour], m_Rect);
		cv::Rect sRec;
		sRec.x = m_Rect.x / scal;
		sRec.y = m_Rect.y / scal;
		sRec.width = m_Rect.width / scal;
		sRec.height = m_Rect.height / scal;

		ImageProcessFunc::CropRect(cv::Rect(0, 0, srcmat.cols, srcmat.rows), sRec);
		srcmat(sRec).copyTo(dstm);
	}

	return 1;

}

int CutParcelBox::_cutParcel_bottom(const cv::Mat &srcm, cv::Mat &dstm,int apply_rotate)
{
	if (srcm.empty())
	{
		std::cout << "图像为空" << std::endl;
		return 0;
	}

	float scal = 480.0f / srcm.cols;


	cv::Mat resizedMat;
	cv::resize(srcm, resizedMat, cv::Size(), scal, scal);
	if (resizedMat.channels() == 3)
	{
		cv::cvtColor(resizedMat, resizedMat, cv::COLOR_BGR2GRAY);
	}
	int iw = resizedMat.cols;
	int ih = resizedMat.rows;

	//imshow("resized", resizedMat);
#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("resized_src", resizedMat);
#endif // CUT_PARCEL_BOX_DEBUG

	//cv::medianBlur(resizedMat, resizedMat,3);


	cv::Mat sobelmat;
	//cv::blur(resizedMat, sobelmat, cv::Size(1, 3), cv::Point(-1, -1), cv::BORDER_REPLICATE);

#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_side_sobel", resizedMat);
#endif // CUT_PARCEL_BOX_DEBUG

	cv::Mat element;


	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 3));

	cv::morphologyEx(resizedMat, sobelmat, cv::MORPH_GRADIENT, element, cv::Point(-1, -1), 1, cv::BORDER_REPLICATE);
	//imshow("grident", sobelmat);
	cv::threshold(sobelmat, sobelmat, 10, 255, cv::THRESH_BINARY);



#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_side_threshold", sobelmat);
#endif // CUT_PARCEL_BOX_DEBUG

	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_DILATE, element);

	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_CLOSE, element);
	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 1));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_ERODE, element);
// 	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
// 	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_ERODE, element);
	//imshow("sobel", sobelmat);


#ifdef CUT_PARCEL_BOX_DEBUG
	imshow("cut_parcel_side_morphology", sobelmat);
#endif // CUT_PARCEL_BOX_DEBUG


	std::vector<std::vector<cv::Point>>contours;
	std::vector<cv::Point> contour;
	double aera = 0;
	//src_gray = src_gray > 100;
	cv::findContours(sobelmat, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	std::vector<double> vec_eare;
	cv::Rect m_Rect;
	cv::Rect best_Rect;
	//std::vector<cv::Point> best_contour;
	bool isgoodRect = false;
	int index_contour = -1;

	//Scalar color = Scalar(rand() % 255, rand() % 255, rand() % 255);
	//drawContours(dstImage, contours, i, color, CV_FILLED, 8, hierarchy);
	for (int j = 0; j < contours.size(); j++)
	{
		double maera = cv::contourArea(contours[j]);
		if (maera > aera)
		{
			index_contour = j;
			isgoodRect = true;
			aera = maera;
		}
	}

	if (index_contour == -1) return 0;

	if (apply_rotate) //
	{
		cv::RotatedRect rbox = cv::minAreaRect(contours[index_contour]);

		rbox.center.x /= scal;
		rbox.center.y /= scal;
		rbox.size.width /= scal;
		rbox.size.height /= scal;

		ImageProcessFunc::getMatFromRotatedRect(srcm, dstm, rbox,125);

	}
	else
	{
		CutParcelBox::findRect(contours[index_contour], m_Rect);
		cv::Rect sRec;
		sRec.x = m_Rect.x / scal;
		sRec.y = m_Rect.y / scal;
		sRec.width = m_Rect.width / scal;
		sRec.height = m_Rect.height / scal;
		float min_ratio = 0.1;
		if (sRec.width < srcm.cols*min_ratio || sRec.width < srcm.cols*min_ratio)
		{
			return 0;
		}

		ImageProcessFunc::CropRect(cv::Rect(0, 0, srcm.cols, srcm.rows), sRec);
		srcm(sRec).copyTo(dstm);
	}

	return 1;
}

