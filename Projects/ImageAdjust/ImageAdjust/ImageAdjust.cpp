// ImageAdjust.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "OcrAlgorithm.h"
#include <algorithm>
//#include "pch.h"
#include "CommonFunc.h"
#include <iostream>
#include "opencv/cv.h"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv/highgui.h"
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/xfeatures2d/nonfree.hpp"
#include <opencv2/ml.hpp>
#include "wavelet2dcpp.h"
#include "CutParcelBox.h"
#include "ImageProcessFunc.h"
#include <random>

//#include "CutParcelBoxDll.h"
#include "time.h"
#include "math.h"
//#include "loadPythonFunc.h"
#include "windows.h"

#include <omp.h>
using namespace cv;
using namespace std;
using namespace cv::xfeatures2d;


//int cutMailBox(std::string &srcImgPath, std::string &dstImgPath, double boxSizeThreshold, double binaryThreshold);
//int findRect(vector<Point> contour, Rect &mrect);
int mirrorImg();
int testROI();
void testAlignImages();
void alignImages(Mat &im1, Mat &im2, Mat &im1Reg, Mat &h);
int drawRotateRect(cv::Mat &srcmat, cv::RotatedRect RtRect);
int getWaveletCoef_v2(cv::Mat srcMat, cv::Mat &dstMat, double threshold);
int cvMat2Vector(cv::Mat srcMat, std::vector<std::vector<double>> &dstvector);
void cvHaarWavelet(Mat &src, Mat &dst, int NIter=1);
int testCutParcelBox();
int testSift();
int testRotateTag();
int Sift(string str2, cv::Mat &dstMat1, cv::Mat &dstMat2);
int getParcelBox(cv::Mat srcmat, cv::Mat &dstmat, int isTopView);
int query_match_count(std::vector<DMatch> &matches, DMatch & new_match); 
int HWdigitsOCR_for_test();
int testGetTray();
void resizeImages();
default_random_engine e;

int main()
{

	//cout << "误差：" << e << endl;

	//Mat m = imread("F:\\CommonDatasets\\ILSVRC2017Download\\tiny-imagenet-200\\test\\images\\test_0.JPEG");
	//cout << ImageProcessFunc::getAverageBrightness(m) << endl;
	//testCutParcelBox();

	//cout << a << endl;
	//QueryPerformanceCounter(&tima);
	//long long t1 = tima.QuadPart;
	//string a1 = to_string(t1);
	//cout << a1 << endl;
	//CString s;
	
	//testGetTray();
	//HWdigitsOCR_for_test();

	//cout << CommonFunc::get_exe_dir()<<endl;

	//for (int i=0;i<10;i++)
	//{
	//	cout << i << " ";
	//}
	//cout << endl;
	//for (int i = 0; i < 10; ++i)
	//{
	//	cout << i << " ";
	//}
	//cout << endl;
	//resizeImages();
	//testROI();
	//testAlignImages();
	//testRotateTag();
	//	
	testCutParcelBox();
	//testWavelet();
	//testSift();
	//	cv::Mat cof;
	//	clock_t start_t = clock();

	//	wavelet.wavelet_transform(resizedMat, cof);
		//getWaveletCoef(resizedMat, cof,2,4,"haar");
		//getWaveletCoef(resizedMat, cof,2,4,"haar");

		//cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
		//cv::morphologyEx(cof, cof, cv::MORPH_CLOSE, element);
		//element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 3));
		//cv::morphologyEx(cof, cof, cv::MORPH_ERODE, element);
		//element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 1));
		//cv::morphologyEx(cof, cof, cv::MORPH_ERODE, element);
		
		//Mat sumii = Mat::zeros(cof.rows + 1, cof.cols + 1, CV_32FC1);//CV_32FC1防止溢出
		//Mat sqsumii = Mat::zeros(cof.rows + 1, cof.cols + 1, CV_32FC1);//CV_32FC1防止溢出
		//integral(cof, sumii, sqsumii);

		//normalize(sqsumii, sqsumii, 0, 255, NORM_MINMAX, CV_8UC1, Mat());





		//clock_t end_t = clock();
		//double timeconsume = (double)(end_t - start_t) / CLOCKS_PER_SEC;
		//cout << "time comsume:" << timeconsume << endl;
		//imshow("back", cof);
		//imshow("src", resizedMat);

		//char tstr[128] = { 0 };
		//sprintf_s(tstr, "f:/resized/%d.jpg", t);
		//imwrite(tstr, resizedMat);
		//imshow("src", resizedMat);
		//Mat imgWave = wavelet.WDT(resizedMat, "haar", 1);
		//imshow("img", Mat_<uchar>(imgWave));
		//drawRotateRect(srcmat, r);
		//waitKey(0);
	//}
	//wavelet_coefTerminate();
	//testROI();
	return 1;
}


void resizeImages()
{
	string dir = "F:\\cpte_datasets\\Tailand_tag_detection_datasets\\20191204\\copy2_m\\*.jpg";
	string out_dir = "F:\\cpte_datasets\\Tailand_tag_detection_datasets\\20191204\\copy2_m_resize\\";
	vector<string> imgfiles;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);
	for (int i=0;i<imgfiles.size();i++)
	{
		cout << i << "/" << imgfiles.size()<<endl;
		Mat srcm = imread(imgfiles[i]);
		if (srcm.empty()) continue;
		int mlength = max(srcm.cols, srcm.rows);
		double scal = 1280.0 / mlength;
		resize(srcm, srcm, Size(), scal, scal,cv::INTER_AREA);
		string dir, f_name;
		CommonFunc::splitDirectoryAndFilename(imgfiles[i], dir, f_name);
		string newfname = CommonFunc::joinFilePath(out_dir, f_name);
		imwrite(newfname, srcm);
	}


}






int testTimeConsume()
{
	uniform_int_distribution<unsigned> u(0, 1000);
	int randNum = u(e);
	float ae = 0;
	std::vector<int> points;
	for (int i = 0; i < 4; i++)
	{
		points.push_back(u(e));
	}
	clock_t time_st, time_end;
	time_st = clock();

	LARGE_INTEGER freq_;
	QueryPerformanceFrequency(&freq_);
	LARGE_INTEGER begin_time;
	LARGE_INTEGER end_time;

	Sleep(100);



	QueryPerformanceCounter(&begin_time);
	for (int j = 0; j < 1000000; j++)
	{

		float a = abs(points[1] - points[0]);
		float b = abs(points[3] - points[2]);
		float c1 = sqrt(a*a + b * b);
		float c2 = sqrt(a*a + b * b);
		float c3 = sqrt(a*a + b * b);
		float c4 = sqrt(a*a + b * b);
		float c5 = sqrt(a*a + b * b);
		float c6 = sqrt(a*a + b * b);
		float c7 = sqrt(a*a + b * b);
		float c8 = sqrt(a*a + b * b);
		float c9 = sqrt(a*a + b * b);
		float c10 = sqrt(a*a + b * b);
	}
	QueryPerformanceCounter(&end_time);
	double ns_time = (end_time.QuadPart - begin_time.QuadPart) * 1000000.0 / freq_.QuadPart;
	std::cout << "time comsume:" << ns_time << endl;
	QueryPerformanceCounter(&begin_time);

	for (int j = 0; j < 1000000; j++)
	{
		int a = abs(points[1] - points[0]);
		int b = abs(points[3] - points[2]);
		int d1 = a + b - min(a, b) / 2;
		int d2 = a + b - min(a, b) / 2;
		int d3 = a + b - min(a, b) / 2;
		int d4 = a + b - min(a, b) / 2;
		int d5 = a + b - min(a, b) / 2;
		int d6 = a + b - min(a, b) / 2;
		int d7 = a + b - min(a, b) / 2;
		int d8 = a + b - min(a, b) / 2;
		int d9 = a + b - min(a, b) / 2;
		int d10 = a + b - min(a, b) / 2;

	}
	QueryPerformanceCounter(&end_time);
	ns_time = (end_time.QuadPart - begin_time.QuadPart) * 1000000.0 / freq_.QuadPart;
	std::cout << "time comsume:" << ns_time << endl;

	return 0;
}
int testRotateTag()
{
	//string dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/tag_cut_img/tag3_r\\*.jpg";
	string dir = "F:\\detected_data\\tag_1\\*.jpg";
	vector<string> imgfiles;
	clock_t start_t, end_t;
	double alltime = 0;
	int ncount = 0;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);
	//Mat referenceImg = imread("E:\\cpp_projects\\Thailand_projects\\_resource_file/sampleA3.jpg");
	//Mat referenceImg2 = imread("E:\\cpp_projects\\Thailand_projects\\_resource_file/sampleA1.jpg");
	OcrAlgorithm_config mconfig;
	string refer1 = "E:\\cpp_projects\\Thailand_projects\\_resource_file/sampleA3.jpg";
	string refer2 = "E:\\cpp_projects\\Thailand_projects\\_resource_file/sampleA1.jpg";
	mconfig.match_data.getMatchDataFromImg_tagRotate_SURF(refer1, refer2);
	OcrAlgorithm ocralg;
	
	
	for (int i = 0; i < imgfiles.size(); i++)
	{
		
		Mat src_img = imread(imgfiles[i]);
		imshow("原图", src_img);
		cout << imgfiles[i] << endl;
		// Align images
		Mat dstImg1, dstImg2;
		start_t = clock();
		ocralg.rotateImg_SURF(src_img, dstImg1, dstImg2, &mconfig);
		end_t = clock();
		double timeconsume = (double)(end_t - start_t) / CLOCKS_PER_SEC;
		alltime += timeconsume;
		ncount++;
		printf("本次耗时:%fs 平均耗时%f\n", timeconsume, alltime / ncount);

		imshow("dstimg1", dstImg1);
		imshow("dstimg2", dstImg2);

		waitKey(0);
	}






	return 1;
}


int testSift()
{

	string dir = "F:/手写框/ok/*.jpg";
	//string dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/Image[2019-8-2]/*.jpg";
	vector<string> imgfiles;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);

	//cv::Mat postcode_line_ref = imread("");
	Mat postcode_line_ref = imread("E:/cpp_projects/Thailand_projects/_resource_file/handwriterange3.jpg");
	tesseract::TessBaseAPI tess;
	if (tess.Init("./tessdata", "eng"))
	{
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
		return 0;
	}
	OcrAlgorithm_config ocrCfg;
	HWDigitsRecog hwdr;
	int res = hwdr.initial("E:/python_projects/Digits_recog_cnn/HDRdigits_v8_dper.pb");
	if (res ==0)
	{
		std::cout << "Handwrite Digits recognition initial fail" << std::endl;
		return 0;
	}
	ocrCfg.pHWDigitsRecog = &hwdr;
	ocrCfg.pTess = &tess;
	res = ocrCfg.match_data.getMatchDataFromImg_handwrite_addr("E:/cpp_projects/Thailand_projects/_resource_file/handwriteRange.jpg");
	if (res == 0)
	{
		std::cout << "load ref image fail" << std::endl;
		return 0;
	}
	HWDigitsOCR boxocr;

	vector<Mat> box_imgs;
	for (int i = 0; i < imgfiles.size(); i++)
	{
		//Mat m1, m2;
		//Sift(imgfiles[i], m1, m2);
		//if (i < 6) continue;
		cout << "图片：" << i << endl;
		string post_str;

		cv::Mat srcImg = cv::imread(imgfiles[i]);
		float scal = 1000.0 / max(srcImg.rows, srcImg.cols);
		cv::Mat reszMat;
		cv::resize(srcImg, reszMat, cv::Size(), scal, scal);
		imshow("PostcodeBox_srcImg", reszMat);




		clock_t start_t = clock();
		boxocr.getPostCode2String(srcImg, post_str, &ocrCfg);

		clock_t end_t = clock();
		double timeconsume = (double)(end_t - start_t) / CLOCKS_PER_SEC;
		cout << "time comsume:" << timeconsume << endl;

		cout << "邮编为：" << post_str << endl;
		//box_imgs.push_back(cv::imread(imgfiles[i]));

		waitKey(0);
	}

	//vector<int> c_;
	//vector<float> cf_;
	//string ocrstr;
	//float confd;
	//boxocr.getPostCodeFromBoxMats(box_imgs, ocrstr, confd,&ocrCfg);
	//cout << ocrstr << endl;
	//cout << confd << endl;

	return 1;


}

int Sift(string str2,cv::Mat &dstMat1, cv::Mat &dstMat2)
{
	const int MAX_FEATURES = 512;
	const float GOOD_MATCH_PERCENT = 0.15f;
	Mat src_im1 = imread("E:/cpp_projects/Thailand_projects/_resource_file/handwriteRange.jpg");
	Mat src_im2 = imread(str2);

	float scal_max1 = 300.0 / max(src_im1.cols, src_im1.rows);
	float scal_min1 = 150.0 / min(src_im1.cols, src_im1.rows);

	float scal1 = max(scal_max1, scal_min1);

	float scal_max2 = 1000.0 / max(src_im2.cols, src_im2.rows);
	float scal_min2 = 500.0 / min(src_im2.cols, src_im2.rows);

	float scal2 = max(scal_max2, scal_min2);

	Mat im1, im2;
	cv::resize(src_im2, im2, cv::Size(), scal2, scal2,cv::INTER_AREA);
	cv::resize(src_im1, im1, cv::Size(), scal1, scal1, cv::INTER_AREA);

	Mat im1Gray, im2Gray;
	cvtColor(im1, im1Gray, CV_BGR2GRAY);
	cvtColor(im2, im2Gray, CV_BGR2GRAY);

	//cv::Canny(im1Gray, im1Gray, 40, 255);
	//cv::Canny(im2Gray, im2Gray, 40, 255);


	// Variables to store keypoints and descriptors
	std::vector<KeyPoint> keypoints1, keypoints2;
	Mat descriptors1, descriptors2;

	// Detect ORB features and compute descriptors.
	Ptr<Feature2D> sift1 = SIFT::create(MAX_FEATURES);
	//Ptr<Feature2D> sift2 = SIFT::create(MAX_FEATURES*2);
	clock_t t_start, t_end;
	t_start = clock();
	sift1->detectAndCompute(im1Gray, Mat(), keypoints1, descriptors1);
	sift1->detectAndCompute(im2Gray, Mat(), keypoints2, descriptors2);
	t_end = clock();
	double timeconsume = (double)(t_end - t_start) / CLOCKS_PER_SEC;
	cout << "time comsume:" << timeconsume << endl;

	Mat img_keypoints_1, img_keypoints_2;	
	drawKeypoints(im1Gray, keypoints1, img_keypoints_1, Scalar::all(-1), 0);
	drawKeypoints(im2Gray, keypoints2, img_keypoints_2, Scalar::all(-1), 0);
	imshow("img_keypoints_1",img_keypoints_1);	
	imshow("img_keypoints_2",img_keypoints_2);



	//waitKey(0);
	// Match features.
	vector<vector<DMatch>> m_knnMatches;
	vector<DMatch>m_Matches;
	vector<DMatch>n_Matches;
	std::vector<DMatch> best_matches;

	std::vector<DMatch> matches;
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(6);//经过测试，6最好
	//matcher->match(descriptors2, descriptors1, matches, Mat());
	matcher->knnMatch(descriptors2, descriptors1, m_knnMatches, 4);
	//// Sort matches by score
	for (int i=0;i<m_knnMatches.size();i++)
	{
		double dist_rate1 = m_knnMatches[i][0].distance / m_knnMatches[i][1].distance;
		double dist_rate2 = m_knnMatches[i][1].distance / m_knnMatches[i][2].distance;
		double dist_rate3 = m_knnMatches[i][2].distance / m_knnMatches[i][3].distance;
		//if (dist_rate1< 0.5 && dist_rate2<0.5)
		//{
		//	continue;
		//}
		//if (dist_rate2 < 0.7)
		//{
		//	best_matches.push_back(m_knnMatches[i][0]);
		//}
		if (dist_rate1 < 0.6)
		{
			best_matches.push_back(m_knnMatches[i][0]);
		}
	}
	//排序

	//std::sort(matches.begin(), matches.end());
	//std::vector<DMatch>::iterator it;
	//it = (matches.size() < 50) ? matches.end() : matches.begin() + 50;
	//best_matches.insert(best_matches.end(), matches.begin(), it);




	std::sort(best_matches.begin(), best_matches.end());

	//将出现大于2个对应点的match 删除多余对应点
	std::vector<DMatch> best_matches_2;
	for (int i=0;i<best_matches.size();i++)
	{
		int res = query_match_count(best_matches_2, best_matches[i]);
		if (res<=1)
		{
			best_matches_2.push_back(best_matches[i]);
		}
	}
	if (best_matches_2.size() < 10) return 0;
	
	//进行2分类聚类
	cv::Mat pt_kmeans(best_matches_2.size(), 1, CV_32FC2);
	for (int i=0;i<best_matches_2.size();i++)
	{
		pt_kmeans.at<Vec2f>(i,0)[0] = keypoints2[best_matches_2[i].queryIdx].pt.x;
		pt_kmeans.at<Vec2f>(i,0)[1] = keypoints2[best_matches_2[i].queryIdx].pt.y;
	}
	std::vector<int> labels;
	std::vector<Point2f> centers;
	int clusters_num = 2;
	cv::kmeans(pt_kmeans, clusters_num, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER,10, 1.0), 3, KMEANS_PP_CENTERS, centers);

	std::vector<DMatch> best_matches_class1;
	std::vector<DMatch> best_matches_class2;
	for (int i=0;i<labels.size();i++)
	{
		if (labels[i]==0)
		{
			best_matches_class1.push_back(best_matches_2[i]);
		}
		else
		{
			best_matches_class2.push_back(best_matches_2[i]);
		}
	}
	if (best_matches_class1.size() < 5 || best_matches_class2.size() < 5)
	{
		return 0;
	}



	//std::sort(matches.begin(), matches.end());



	// Remove not so good matches
	

	// Draw top matches
	Mat imMatches1,imMatches2;
	drawMatches(im2, keypoints2, im1, keypoints1, best_matches_class1, imMatches1);
	drawMatches(im2, keypoints2, im1, keypoints1, best_matches_class2, imMatches2);

	imshow("m1", imMatches1);
	imshow("m2", imMatches2);




	// Extract location of good matches
	std::vector<Point2f> points_class1, points_class2, points_ref1, points_ref2;

	for (size_t i = 0; i < best_matches_class1.size(); i++)
	{
		points_class1.push_back(keypoints2[best_matches_class1[i].queryIdx].pt);
		points_ref1.push_back(keypoints1[best_matches_class1[i].trainIdx].pt);
	}
	for (size_t i = 0; i < best_matches_class2.size(); i++)
	{
		points_class2.push_back(keypoints2[best_matches_class2[i].queryIdx].pt);
		points_ref2.push_back(keypoints1[best_matches_class2[i].trainIdx].pt);
	}




	/*
	FindHomography_2d findhg;
	Mat ms1, ms2;
	cv::RNG rng;
	cv::Mat m1(points_class1);
	cv::Mat m2(points_ref1);
	cv::Mat m3(points_class2);
	cv::Mat m4(points_ref2);
	//findhg.getSubset(m1, m2, ms1, ms2, rng);
	cout << "参与1的点数：" << points_class1.size() << endl;
	cout << "参与2的点数：" << points_class2.size() << endl;
	Mat h1,h2, mask1, mask2;
	bool r = findhg.run(m1, m2, h1, mask1);
	r = findhg.run(m3, m4, h2, mask2);
	if (h1.empty()||h2.empty())
	{
		return 0;
	}
	//cout << mask2 << endl;
	cv::transpose(h1, h1);
	cv::transpose(h2, h2);
	//cout << h1 << endl;


	std::vector<Point2f> points_class1_0, points_class2_0, points_ref1_0, points_ref2_0;
	uchar* maskptr = mask1.ptr<uchar>();
	int n = mask1.total();
	for (int i=0;i<n;i++)
	{
		if (maskptr[i]==1)
		{
			points_class1_0.push_back(points_class1[i]);
			points_ref1_0.push_back(points_ref1[i]);
		}
		
	}
	maskptr = mask2.ptr<uchar>();
	n = mask2.total();
	for (int i = 0; i < n; i++)
	{
		if (maskptr[i] == 1)
		{
			points_class2_0.push_back(points_class2[i]);
			points_ref2_0.push_back(points_ref2[i]);
		}

	}
	cv::Mat m1_0(points_class1_0);
	cv::Mat m2_0(points_ref1_0);
	cv::Mat m3_0(points_class2_0);
	cv::Mat m4_0(points_ref2_0);
	cv::Point2f scal_xy_1, scal_xy_2;
	scal_xy_1.x = scal2; scal_xy_1.y = scal2;
	scal_xy_2.x = scal2; scal_xy_2.y = scal2;
	Mat h1_0, h2_0;
	findhg.runKernel_NoZoom(m1_0, m2_0, h1_0, scal_xy_1);
	findhg.runKernel_NoZoom(m3_0, m4_0, h2_0, scal_xy_2);
	cv::transpose(h1_0, h1_0);
	cv::transpose(h2_0, h2_0);

	cv::Size result1_size = im1.size();
	cv::Size result2_size = im1.size();
	float zoom_s = 1.1;
	result1_size.width *= (scal_xy_1.x*zoom_s);
	result1_size.height *= (scal_xy_1.y*zoom_s);
	result2_size.width *= (scal_xy_2.x*zoom_s);
	result2_size.height *= (scal_xy_2.y*zoom_s);
	*/

	cv::Mat maskm;
	Mat h1 = findHomography(points_class1, points_ref1, maskm, RANSAC);
	Mat h2 = findHomography(points_class2, points_ref2, maskm, RANSAC);

	//if (h1.empty() | h2.empty()) return 0;



	//cout << h1.at<double>(0, 0) << h1.at<double>(0, 1) << h1.at<double>(0, 2) << endl;
	//cout << h1.at<double>(1, 0) << h1.at<double>(1, 1) << h1.at<double>(1, 2) << endl;
	//cout << h1.at<double>(2, 0) << h1.at<double>(2, 1) << h1.at<double>(2, 2) << endl;

	//cout << h2.at<double>(0, 0) << h2.at<double>(0, 1) << h2.at<double>(0, 2) << endl;
	//cout << h2.at<double>(1, 0) << h2.at<double>(1, 1) << h2.at<double>(1, 2) << endl;
	//cout << h2.at<double>(2, 0) << h2.at<double>(2, 1) << h2.at<double>(2, 2) << endl;
	//h1.at<double>(2, 0) = 0;
	//h1.at<double>(2, 1) = 0;
	//h2.at<double>(0, 2) = 0;
	//h2.at<double>(1, 2) = 0;

	//cout << h1 << endl;
	//cout << h2 << endl;

	int tt = 0;
	//// Find homography
	//Mat h = findHomography(points2,points1, RANSAC);
	Mat im1Reg, im2Reg;
	//// Use homography to warp image
	if (!h1.empty() && !h2.empty())
	{
		//warpPerspective(im2, im1Reg, h1, im1.size());
		//warpPerspective(im2, im2Reg, h2, im1.size());

		warpPerspective(im2, im1Reg, h1, im1Gray.size());
		warpPerspective(im2, im2Reg, h2, im1Gray.size());
		dstMat1 = im1Reg;
		dstMat2 = im2Reg;
		imshow("检测到1", im1Reg);
		imshow("检测到2", im2Reg);
		return 1;
	}

	
	//waitKey(0);

	return 0;

}

int query_match_count(std::vector<DMatch> &matches, DMatch & new_match)
{
	int match_count = 0;
	std::vector<DMatch>::iterator it;
	for (it=matches.begin();it!=matches.end();it++)
	{
		if (new_match.trainIdx == it->trainIdx) match_count++;
	}
	return match_count;
}



/*
int getWaveletCoef(cv::Mat srcMat, cv::Mat &dstMat,double threshold, int coef_direc,const std::string wavelet_name)
{
	//if (srcMat.empty())
	//{
	//	return 0;
	//}
	//if (srcMat.channels()==3)
	//{
	//	cv::cvtColor(srcMat, srcMat, CV_BGR2GRAY);
	//}
	//int w = srcMat.cols;
	//int h = srcMat.rows;
	////mwSize dims[2] = { h, w }; 
	////mxArray *pMat = NULL;
	////UINT8 *input = NULL;
	//mwArray mMat(h,w,mxUINT8_CLASS,mxREAL);
	////pMat = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);
	//cv::transpose(srcMat, srcMat);
	//mMat.SetData(srcMat.data, w*h);
 //
	//mwArray mThreshold(1, 1, mxDOUBLE_CLASS, mxREAL);
	////double m_threshold[1];
	////m_threshold[0] = threshold;
	////mThreshold.SetData(m_threshold,1);
	//mwArray outMat;
	//if (wavelet_name.empty()) return 0;
	//mwArray mWaveletName(wavelet_name.c_str());
	//uint coef_index[1];
	//coef_index[0] = coef_direc;
	//mwArray mcoef_index(1, 1, mxUINT8_CLASS, mxREAL);
	//mcoef_index.SetData(coef_index, 1);



	////cout << "into wavelet_coef" << endl;
	//wavelet_coef(1, outMat,mMat,mcoef_index,mWaveletName);
	//
	////cout << "out wavelet_coef" << endl;

	//if (outMat.IsEmpty()) /// 是否为空
	//{
	//	cout << "返回为空" << endl;
	//	return 0;
	//}


	//mwArray dimn = outMat.GetDimensions();
	////cout << dimn << endl;
	////cout << "get dim out" << endl;
	//int oh = dimn.Get(1, 1);
	//int ow = dimn.Get(1, 2);
	////cout << "output h:" << oh << endl;
	////cout << "output w:" << ow << endl;
	////std::ofstream testfile("d:/test.txt", std::ios::out);
	//cv::Mat dst = cv::Mat::zeros(oh, ow, CV_8UC1);
	//uchar* pdata = dst.data;
	//for (int j(0); j < oh; ++j)
	//{
	//	//uchar* pdata = dst.ptr<uchar>(j);
	//	for (int i(0); i < ow; ++i)
	//	{
	//		//double a = ;
	//		dst.at<uchar>(j, i) = ((double)outMat(j + 1, i + 1) > threshold) ? 255 : 0; /// 元素访问（行号，列号）
	//		//testfile << outMat(j + 1, i + 1) << " ";
	//	}
	//	//testfile << endl;
	//}
	////testfile.flush();
	////testfile.close();
	//dstMat = dst;
	return 1;
}
*/
int testCutParcelBox()
{
	//CutMailBox cutMailBox;
	string dir = "F:\\cpte_datasets\\AlignCenter\\15/*.jpg";
	string out_dir = "F:\\cpte_datasets\\AlignCenter\\temp/";
	//string dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/Image[2019-8-2]/*.jpg";
	vector<string> imgfiles;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, true, true);
	for (int i = 0; i < imgfiles.size(); i++)
	{
		//res = OcrAlgorithm::getParcelBoxFromSick(imgfiles[i], r);
		//if (res==0)
		//{
		//	cout << "不符合要求" << endl;
		//	continue;
		//}
		//if(i<40) continue;

		cout << "图片:" << i <<"/"<<imgfiles.size()<< endl;
		std::string imgfile = imgfiles[i];
		//imgfile = imgfile.replace(imgfile.length() - 3, 3, "jpg");
		cv::Mat srcmat;
		//cout << "read a file:" << imgfile << endl;
		srcmat = cv::imread(imgfile);
		if (srcmat.empty())
		{
			cout << "图像为空" << endl;
			continue;
		}
		//cv::Mat show_src;
		//cv::resize(srcmat, show_src, Size(), 0.15, 0.15);
		//imshow("src", show_src);
		//clock_t start_t = clock();
		CutParcelBox cutbox;
		//CutParcelBoxDll cutbox;
		cv::Mat parcelMat;
		//int res = cutbox.getMailBox_Mat(srcmat, resizedMat,0);  

		//int res = cutbox.getMailBox_side(srcmat, parcelMat, 0.5, 120);
		int res = cutbox.getMailBox_Mat(srcmat, parcelMat,1, 0.5, 120);
		
		if ( res== 0)
		{
			cout << "截取包裹为空" << endl;
			
		}
		else
		{
			cout << "OK!!!" << endl;
		}
		cv::waitKey(0);
		if (res == 0) continue;;
		string mfilename;
		CommonFunc::splitDirectoryAndFilename(imgfile, string(), mfilename);
		string newfilepath = CommonFunc::joinFilePath(out_dir, mfilename);
		imwrite(newfilepath, parcelMat);


		/*clock_t end_t = clock();
		double timeconsume = (double)(end_t - start_t) / CLOCKS_PER_SEC;
		cout << "time comsume:" << timeconsume << endl;
		if (res!=0)
		{
			cv::resize(resizedMat, resizedMat, Size(), 0.2, 0.2);
			imshow("res", resizedMat);

		}*/

		


		//imshow("results", resizedMat);

		//waitKey(0);

	}



	return 1;
}
int getParcelBox(cv::Mat srcmat, cv::Mat &dstmat, int isTopView)
{
	if (srcmat.empty())
	{
		cout << "图像为空" << endl;
		return 0;
	}

	float scal_max = 512.0 / max(srcmat.cols, srcmat.rows);
	float scal_min = 256.0 / min(srcmat.cols, srcmat.rows);

	float scal = max(scal_max, scal_min);
	cv::Mat resizedMat;
	cv::resize(srcmat, resizedMat, cv::Size(), scal, scal);
	if (resizedMat.channels() == 3)
	{
		cv::cvtColor(resizedMat, resizedMat, CV_BGR2GRAY);
	}
	cv::Mat sobelmat;
	Sobel(resizedMat, sobelmat, CV_8U, 1, 1);

	cv::threshold(sobelmat, sobelmat, 5, 255, THRESH_BINARY);

	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(8, 8));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_CLOSE, element);
	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 8));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_ERODE, element);
	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::morphologyEx(sobelmat, sobelmat, cv::MORPH_ERODE, element);
	imshow("sobel", sobelmat);

	std::vector<std::vector<cv::Point>>contours;
	std::vector<cv::Vec4i>hierarchy;
	std::vector<cv::Point> contour;
	double aera = 0;
	//src_gray = src_gray > 100;
	cv::findContours(sobelmat, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

	//绘制轮廓图
	std::vector<double> vec_eare;
	cv::Rect m_Rect;
	cv::Rect best_Rect;
	std::vector<cv::Point> best_contour;





	//bool isgoodRect = false;
	//int index_hierc = -1;
	//int index_contour = -1;

	////Scalar color = Scalar(rand() % 255, rand() % 255, rand() % 255);
	////drawContours(dstImage, contours, i, color, CV_FILLED, 8, hierarchy);
	//for (int j = 0; j < contours.size(); j++)
	//{
	//	double maera = cv::contourArea(contours[j]);
	//	if (maera > aera)
	//	{
	//		if (findRect(contours[j], m_Rect) == 0) continue;

	//		//best_Rect = m_Rect;
	//		index_contour = j;
	//		isgoodRect = true;
	//		aera = maera;
	//	}
	//}

	////cv::rectangle(dstImage, m_Rect, Scalar(rand() % 255, rand() % 255, rand() % 255));
	////imshow("rectangle", dstImage);
	////std::vector<cv::Point> best_contour;
	//if (index_contour >= 0)
	//{
	//	cv::RotatedRect rrc;
	//	rrc = cv::minAreaRect(contours[index_contour]);
	//	rrc.center.x /= scal;
	//	rrc.center.y /= scal;
	//	rrc.size.width /= scal;
	//	rrc.size.height /= scal;
	//	dstRtRect = rrc;
	//	return 1;
	//}
	//else
	//{
	//	return 0;
	//}










	return 1;
}




int getWaveletCoef_v2(cv::Mat srcMat, cv::Mat &dstMat, double threshold)
{
	int w = srcMat.cols;
	int h = srcMat.rows;
	int iw = (w < 128) ? w : 128;
	int ih = (h < 128) ? h : 128;;
	int grid_x = (w / iw == 0) ? 1 : (w / iw);
	int grid_y = (h / ih == 0) ? 1 : (h / ih);
	const int flat_size = grid_x * grid_y;
	int res_x = w % iw;
	int res_y = h % ih;
	cv::Rect rc;
	//assert(coef_direc >= 0 && coef_direc < 4);
	
	std::vector<cv::Mat> gridMats(flat_size);
	std::vector<cv::Mat> coefMats(flat_size);

	//std::vector<cv::Mat> mats(grid_x*grid_y);
	for (int i = 0; i < flat_size; i++)
	{
		int iw_ = ((i%grid_x) != (grid_x - 1)) ? iw : (iw + res_x);
		int ih_ = ((i / grid_x) != (grid_y - 1)) ? ih : (ih + res_y);
		rc = cv::Rect((i % grid_x)*iw, (i / grid_x)*ih, iw_, ih_);
		srcMat(rc).convertTo(gridMats[i], CV_32FC1);
		int coef_h = ih_;// >> 1;
		int coef_w = iw_;// >> 1;
		coefMats[i] = cv::Mat(coef_h, coef_w, CV_32FC1);
	}


#pragma omp parallel for
	for (int i=0;i< flat_size;i++)
	{
		//cv::Mat diagMat = coefMats[i];
		cvHaarWavelet(gridMats[i], coefMats[i]);
		int diag_h = gridMats[i].rows >> 1;
		int diag_w = gridMats[i].cols >> 1;
		cv::Rect coef_diag_rect(diag_w, diag_h, diag_w, diag_h);
		coefMats[i] = coefMats[i](coef_diag_rect);//取对角线系数
		cv::threshold(coefMats[i], coefMats[i], threshold, 255, THRESH_BINARY);
		coefMats[i].convertTo(coefMats[i], CV_8UC1);

	}

	int dstMat_w = 0;
	for (int i=0;i<grid_x;i++)
	{
		dstMat_w += coefMats[i].cols;
	}
	int dstMat_h = 0;
	for (int i = 0; i < grid_y; i++)
	{
		dstMat_h += coefMats[i*grid_x].rows;
	}
	cv::Mat dstMat_tmp(dstMat_h, dstMat_w, CV_8UC1);
	int anc_x = 0;
	int anc_y = 0;
	for (int i = 0; i < flat_size; i++)
	{
		cv::Rect dst_g(anc_x,anc_y, coefMats[i].cols, coefMats[i].rows);
		coefMats[i].copyTo(dstMat_tmp(dst_g));
		anc_x += coefMats[i].cols;
		if (i%grid_x==(grid_x-1))
		{
			anc_y += coefMats[i].rows;
			anc_x = 0;
		}
		//delete mats[i];
	}
	dstMat = dstMat_tmp;


	//std::vector<double> mdwt_out;
	//std::vector<double> midwt_coef;
	//std::vector<int> mlength;
	//Wavelet2d wavelet;
	////wavelet.dwt_2d_OMP(src_data, 1, wavelet_name,mdwt_out,midwt_coef,mlength);
	//int wavecoef_w = mlength[1];
	//int wavecoef_h = mlength[0];


	return 1;

}
void cvHaarWavelet(Mat &src, Mat &dst, int NIter)
{
	float c, dh, dv, dd;
	assert(src.type() == CV_32FC1);
	assert(dst.type() == CV_32FC1);
	int width = src.cols;
	int height = src.rows;
	for (int k = 0; k < NIter; k++)
	{
		for (int y = 0; y < (height >> (k + 1)); y++)
		{
			for (int x = 0; x < (width >> (k + 1)); x++)
			{
				c = (src.at<float>(2 * y, 2 * x) + src.at<float>(2 * y, 2 * x + 1) + src.at<float>(2 * y + 1, 2 * x) + src.at<float>(2 * y + 1, 2 * x + 1))*0.5;
				dst.at<float>(y, x) = c;

				dh = (src.at<float>(2 * y, 2 * x) + src.at<float>(2 * y + 1, 2 * x) - src.at<float>(2 * y, 2 * x + 1) - src.at<float>(2 * y + 1, 2 * x + 1))*0.5;
				dst.at<float>(y, x + (width >> (k + 1))) = dh;

				dv = (src.at<float>(2 * y, 2 * x) + src.at<float>(2 * y, 2 * x + 1) - src.at<float>(2 * y + 1, 2 * x) - src.at<float>(2 * y + 1, 2 * x + 1))*0.5;
				dst.at<float>(y + (height >> (k + 1)), x) = dv;

				dd = (src.at<float>(2 * y, 2 * x) - src.at<float>(2 * y, 2 * x + 1) - src.at<float>(2 * y + 1, 2 * x) + src.at<float>(2 * y + 1, 2 * x + 1))*0.5;
				dst.at<float>(y + (height >> (k + 1)), x + (width >> (k + 1))) = dd;
			}
		}
		dst.copyTo(src);
	}
}

int cvMat2Vector(cv::Mat srcMat, std::vector<std::vector<double>> &dstvector)
{
	cv::Mat srcMat_double;
	srcMat.convertTo(srcMat_double, CV_64FC1);
	int w = srcMat_double.cols;
	int h = srcMat_double.rows;
	for (int i = 0; i < h; i++)
	{
		double* phdata = srcMat_double.ptr<double>(i);
		std::vector<double> hdata(phdata, phdata + w);
		dstvector.push_back(hdata);
	}
	return 1;

}
int mirrorImg()
{
	vector<string> imgfiles;
	string dir = "F:\\shared_data\\*.jpg";
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);
	for (int i = 0; i < imgfiles.size(); i++)
	{
		cv::Mat src_m;
		src_m=cv::imread(imgfiles[i]);
		cv::Mat rize_m;
		cv::resize(src_m, rize_m, cv::Size(800, 800));
		cv::imshow("srcimg", rize_m);
		int key = waitKey(0);
		if (key==114)
		{
			std::cout << "旋转图片" << endl;
			cv::flip(src_m, src_m, 0);
			//cv::imshow("srcimg", src_m);
			cv::imwrite(imgfiles[i],src_m);
			//waitKey(0);
		}
		

	}

	return 0;
}


int testROI()
{
	//string dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/tag_cut_img/hard_work\\*.jpg";
	//string dir = "F:\\detected_data\\ng\\*.jpg";
	string dir = "E:\\cpp_projects\\Thailand_projects\\Projects\\_run_dir\\saved_file\\*.jpg";
	vector<string> imgfiles;
	clock_t start_t, end_t;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);
	printf("共找到文件%zd个\n", imgfiles.size());
	OcrAlgorithm mAlgo;
	tesseract::TessBaseAPI tess;
	if (tess.Init("./tessdata", "eng"))
	{
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
		return 0;
	}
	char ocrResults[64] = { 0 };
	double alltime = 0;
	int ncount = 0;
	OcrAlgorithm_config ocrConfig;
	ocrConfig.ORB_template_img1_path = "E:/cpp_projects/code/Image Alignment Adnan/sampleA1.jpg";
	ocrConfig.ORB_template_img2_path = "E:/cpp_projects/code/Image Alignment Adnan/sampleA3.jpg";
	//ocrConfig.match_data.getMatchDataFromImg(ocrConfig.ORB_template_img1_path, ocrConfig.ORB_template_img2_path);
	ocrConfig.match_data.loadMatchData(".\\matchdata.xml");


	for (int i = 0; i < imgfiles.size(); i++)
	{
		//if (i<19) continue;
		cout << endl;
		start_t = clock();
		cout << "图片：" << i << endl;
		
		string dstpath = imgfiles[i];
		dstpath.replace(dstpath.find_last_of("tag1"), 4, "rotate");
		Mat src_img = imread(imgfiles[i]);
		imshow("原图", src_img);

		string ocrresultstr;
		int res = mAlgo.getOcrResultString(src_img, &tess, ocrresultstr, &ocrConfig);
		if (res != 0)
		{
			cout << "邮编为：" << ocrresultstr << endl;
		}
		else
		{
			cout << "未识别到邮编" << endl;
		}
		end_t = clock();
		double timeconsume = (double)(end_t - start_t) / CLOCKS_PER_SEC;
		alltime += timeconsume;
		ncount++;
		printf("本次耗时:%fs 平均耗时%f\n", timeconsume, alltime / ncount);


		//mAlgo.rotateImg_SURF(src_img, refm1,refm2, dstm);


		//start_t = clock();
		//Mat rotated_img;
		//imshow("原始图像", src_img);
		////waitKey(0);
		//printf("第%d幅图\n",i);
		//int res = rotateImg(src_img, rotated_img);
		//if (res==0)
		//{
		//	printf("矫正失败\n");
		//	waitKey(0);
		//	continue;
		//}
		//imshow("矫正图像", rotated_img);
		//Rect mRect;
		//res = getBarcodePos(rotated_img, mRect);
		//if (res == 0)
		//{
		//	printf("定位条码失败\n");
		//	waitKey(0);
		//	continue;
		//}
		//Mat roimat;
		//res = getOcrRoi(rotated_img, roimat, mRect);
		//if (res == 0)
		//{
		//	printf("定位邮编失败\n");
		//	waitKey(0);
		//	continue;
		//}
		//end_t = clock();
		//printf("共耗时:%fs\n", (double)(end_t - start_t)/ CLOCKS_PER_SEC);
		//imshow("邮编定位", roimat);

		waitKey(0);
	}
	return 0;
}
void testAlignImages()
{
	//string dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/tag_cut_img/tag3_r\\*.jpg";
	string dir = "F:\\detected_data\\tag_1\\*.jpg";
	vector<string> imgfiles;
	clock_t start_t, end_t;
	double alltime = 0;
	int ncount = 0;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);
	Mat referenceImg = imread("E:\\cpp_projects\\Thailand_projects\\_resource_file/sampleA3.jpg");
	Mat referenceImg2 = imread("E:\\cpp_projects\\Thailand_projects\\_resource_file/sampleA1.jpg");
	for (int i = 0; i < imgfiles.size(); i++)
	{
		start_t = clock();
		Mat src_img = imread(imgfiles[i]);
		imshow("原图", src_img);
		Mat imReg, h;
		cout << imgfiles[i] << endl;
		// Align images
		cout << "Aligning images ..." << endl;
		alignImages(src_img, referenceImg, imReg, h);
		imshow("旋转图像", imReg);
		alignImages(src_img, referenceImg2, imReg, h);
		imshow("旋转图像2", imReg);
		end_t = clock();
		double timeconsume = (double)(end_t - start_t) / CLOCKS_PER_SEC;
		alltime += timeconsume;
		ncount++;
		printf("本次耗时:%fs 平均耗时%f\n", timeconsume, alltime / ncount);
		waitKey(0);
	}

}
void alignImages(Mat &im1, Mat &im2, Mat &im1Reg, Mat &h)

{
	const int MAX_FEATURES = 512;
	const float GOOD_MATCH_PERCENT = 0.15f;
	// Convert images to grayscale
	Mat im1Gray, im2Gray;
	cvtColor(im1, im1Gray, CV_BGR2GRAY);
	cvtColor(im2, im2Gray, CV_BGR2GRAY);

	// Variables to store keypoints and descriptors
	std::vector<KeyPoint> keypoints1, keypoints2;
	Mat descriptors1, descriptors2;

	// Detect ORB features and compute descriptors.
	Ptr<Feature2D> orb = SIFT::create(MAX_FEATURES);
	orb->detectAndCompute(im1Gray, Mat(), keypoints1, descriptors1);
	orb->detectAndCompute(im2Gray, Mat(), keypoints2, descriptors2);

	// Match features.
	std::vector<DMatch> matches;
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(1);
	matcher->match(descriptors1, descriptors2, matches, Mat());

	// Sort matches by score
	std::sort(matches.begin(), matches.end());

	// Remove not so good matches
	const int numGoodMatches = matches.size() * GOOD_MATCH_PERCENT;
	matches.erase(matches.begin() + numGoodMatches, matches.end());


	// Draw top matches
	Mat imMatches;
	drawMatches(im1, keypoints1, im2, keypoints2, matches, imMatches);
	imshow("immatches", imMatches);
	//imwrite("matches.jpg", imMatches);


	// Extract location of good matches
	std::vector<Point2f> points1, points2;

	for (size_t i = 0; i < matches.size(); i++)
	{
		points1.push_back(keypoints1[matches[i].queryIdx].pt);
		points2.push_back(keypoints2[matches[i].trainIdx].pt);
	}

	// Find homography
	h = findHomography(points1, points2, RANSAC);

	// Use homography to warp image
	warpPerspective(im1, im1Reg, h, im2.size());

}

int drawRotateRect(cv::Mat &srcmat, cv::RotatedRect RtRect)
{
	//cv::Mat image = cv::Mat::zeros(img_width, img_height, CV_8UC3);
	Point2f vertices[4];
	RtRect.points(vertices);
	for (int i = 0; i < 4; i++)
	{
		cv::line(srcmat, vertices[i], vertices[(i + 1) % 4], cv::Scalar(0, 255, 0), 10);
	}
	float scal = 1000.0 / max(srcmat.cols, srcmat.rows);
	cv::Mat resizedMat;
	cv::resize(srcmat, resizedMat, cv::Size(), scal, scal);
	cv::imshow("框框", resizedMat);
	return 1;
}

int HWdigitsOCR_for_test()
{
	vector<string> imgfiles;
	clock_t start_t, end_t;
	double alltime = 0;
	int ncount = 0;
	//string dir = "F:\\cpte_datasets\\Tailand_tag_detection_datasets\\王港图像6.28\\顶\\*.jpg";
	string dir = "F:\\泰国ocr\\pic2\\*.jpg";
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, true, true);

	tesseract::TessBaseAPI tess;
	if (tess.Init("./tessdata", "eng"))
	{
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
		return 0;
	}
	OcrAlgorithm_config ocrCfg;
	HWDigitsRecog hwdr;
	int res = hwdr.initial("E:/python_projects/Digits_recog_cnn/HDRdigits_v9_dper.pb");
	if (res == 0)
	{
		std::cout << "Handwrite Digits recognition initial fail" << std::endl;
		return 0;
	}
	ocrCfg.pHWDigitsRecog = &hwdr;
	ocrCfg.pTess = &tess;
	ocrCfg.HandwriteDigitsConfidence = 0.85;
	HWDigitsOCR boxocr;
	int i = 0;
	int ocr_ok_count = 0;
	for (i=0;i<imgfiles.size();i++)
	{
		//if (i<112) continue;
		cout << "图片：" << i<<endl;
		
		
		cv::Mat srcm,cutm;
		srcm = imread(imgfiles[i]);
		cv::Mat traymat;
		boxocr.getTrayMat(srcm, traymat);
		CutParcelBox cutb;
		cutb.getMailBox_Mat(traymat, cutm);
		//string xmlfile = imgfiles[i];
		//size_t pp = xmlfile.find_last_of('.');
		//if (pp!=xmlfile.npos)
		//{
		//	xmlfile = xmlfile.substr(0, pp);
		//	xmlfile = xmlfile + ".xml";
		//	cv::RotatedRect r;
		//	OcrAlgorithm::getParcelBoxFromSick(xmlfile, r);
		//	cv::Mat sickm;
		//	ImageProcessFunc::getMatFromRotatedRect(srcm, sickm, r);
		//	resize(sickm, sickm, cv::Size(), 0.2, 0.2);
		//	imshow("sick", sickm);
		//}


		Mat sowm;
		if (cutm.empty())
		{
			cout << "图像为空" << endl;
			continue;
		}
		resize(cutm, sowm, cv::Size(), 0.2, 0.2);
		imshow("cutm",sowm);
		//resize(traymat, sowm, cv::Size(), 0.2, 0.2);
		//imshow("traym", sowm);
		//resize(srcm, sowm, cv::Size(), 0.2, 0.2);
		//imshow("srcm", sowm);
		ArbitTagOCR artag;
		string ocrstr;
		int res=0;
		Mat tagMat;
		try
		{
			
			res = artag.getPostCodeString(cutm, ocrstr, &ocrCfg);
			//res = boxocr.getPostCode2String_test(cutm, ocrstr, &ocrCfg);
		}
		catch (...)
		{
			cout << "hand write recog exception" << endl;
		}
		//imshow("tag", tagMat);
		if (res!=0)
		{
			ocr_ok_count++;
			cout << "邮编" << ocrstr << endl;
			waitKey(0);
		}
		else
		{
			waitKey(0);
		}
	}

	cout << "识别率:" << ocr_ok_count/float(i) << endl;

	return 1;
}

int testGetTray()
{

	vector<string> imgfiles;
	string dir = "F:\\泰国ocr\\pic2\\1\\*.jpg";
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, true, true);
	ArbitTagOCR art;
	cout << imgfiles.size() << endl;
	for (int i=0;i<imgfiles.size();i++)
	{
		//if (i<112) continue;

		cout << imgfiles[i] << endl;
		Mat m = imread(imgfiles[i]);
		Mat dm;
		art.getTag(m, dm);
		resize(dm, dm, cv::Size(), 0.2, 0.2);
		imshow("dm", dm);

		waitKey(0);
	}

	//double angle_ = -90;
	//for (int i=0;i<18;i++)
	//{
	//	Mat m = Mat::zeros(Size(500, 500), CV_8U);
	//	Rect r(90, 125, 330, 250);
	//	rectangle(m,r, Scalar(255, 255, 255), -1);
	//	circle(m, Point(300, 250), 5, Scalar(0, 0, 0), -1);
	//	imshow("src", m);
	//	ImageProcessFunc::rotate_arbitrarily_angle(m, m, angle_/180*CV_PI);
	//	//cout << "angle:" << angle_ << endl;
	//	angle_ += 10;
	//	
	//	vector<vector<Point>> contours;
	//	findContours(m, contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);
	//	double a = 0;
	//	int ind = -1;
	//	for (int j=0;j<contours.size();j++)
	//	{
	//		double marea = contourArea(contours[j]);
	//		if (marea>a)
	//		{
	//			ind = j;
	//			a = marea;
	//		}
	//	}
	//	vector<Point> maxcontour = contours[ind];
	//	putText(m, "1", maxcontour[0], 0, 1, Scalar(120, 0, 0));
	//	putText(m, "2", maxcontour.back(), 0, 1, Scalar(120, 0, 0));
	//	imshow("rm", m);
	//	RotatedRect rt;
	//	rt = minAreaRect(maxcontour);
	//	Mat tag;
	//	ImageProcessFunc::getMatFromRotatedRect(m, tag, rt);
	//	imshow("tag", tag);
	//	cout << "rangle:" << rt.angle << endl;




	//	waitKey(0);
	//	if (i==17)
	//	{
	//		i = -1;
	//	}

	//}



	return 1;

}