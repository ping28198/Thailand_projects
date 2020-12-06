// AlgorithmsV2.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <opencv2//opencv.hpp>
#include "CommonFunc.h"
#include "PostcodeAlgorithm.h"
#include <algorithm>
#include <fstream>
using namespace std;
using namespace cv;


//#define SHOW_IMAGE_TEST

int format_tess_config_file(string cfg_file_path)
{
	ifstream ifs;
	ifs.open(cfg_file_path);
	if (!ifs.is_open())
	{
		return 0;
	}
	std::string linstr;
	std::vector<std::string> cfg_lines;
	while (getline(ifs, linstr))
	{
		string tmp;
		size_t pos = linstr.find('\r');
		if (pos==linstr.npos)
		{
			tmp = linstr;
			if (tmp.empty())
			{
				continue;
			}
			tmp += "\r\n";
			cfg_lines.push_back(tmp);
		}
		else
		{
			tmp = linstr.substr(0,pos);
			if (tmp.empty())
			{
				continue;
			}
			tmp += "\r\n";
			cfg_lines.push_back(tmp);
		}

	}
	ifs.close();
	ofstream ofs;
	ofs.open(cfg_file_path);
	if (!ofs.is_open())
	{
		return 0;
	}
	for (int i=0;i<cfg_lines.size();i++)
	{
		ofs << cfg_lines[i];
	}

	ofs.close();


	return 1;
}



void test_tag_ocr()
{

	string dir = "F:\\cpte_datasets\\Tailand_tag_detection_datasets\\tag_cut_img\\rotated_tag\\*.jpg";
	string match_line_img = "E:\\cpp_projects\\Thailand_projects\\Projects\\_run_dir\\resource\\match_line_image.jpg";
	string exe_dir = CommonFunc::get_exe_dir();
	std::cout << "image dir:" << dir << std::endl;
	std::vector<std::string> imgs;
	CommonFunc::getAllFilesNameInDir(dir, imgs, false, true);
	std::cout << "file nums:" << imgs.size() << std::endl;
	double total_consume = 0;
	clock_t t0;
	clock_t t1;
	vector<double> time_range;
	OCRStandardTag stocr;
	OcrAlgorithm_config cfg;
	string ocr_parrerns_file = CommonFunc::joinFilePath(exe_dir, "tag_patterns_config.txt");
	string config_file = CommonFunc::joinFilePath(exe_dir, "en_ocr_config.txt");


	//format_tess_config_file(ocr_parrerns_file);
	//format_tess_config_file(config_file);

	FILE *pattern_file = fopen(ocr_parrerns_file.c_str(), "rb");
	if (pattern_file != nullptr)
	{
		cout << "pattern_file_Read_ok" << endl;
		fclose(pattern_file);
	}
	


	char *configs[1];
	//configs[1] = (char*)(ocr_parrerns_file.c_str());
	configs[0] = (char*)(config_file.c_str());

	int configs_size = 1;
	tesseract::TessBaseAPI tess;
	if (tess.Init((exe_dir + "/tessdata").c_str(), "digits", tesseract::OcrEngineMode::OEM_LSTM_ONLY, configs, configs_size, NULL, NULL, false))
	{
#ifdef _DEBUG
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
#endif // _DEBUG
		return;
	}

	cfg.pTessEn = &tess;

	cfg.match_data.getMatchDataFromImg_tag_line("E:\\cpp_projects\\Thailand_projects\\Projects\\_run_dir\\resource\\match_line_image.jpg");





	int i = 0;
	for (i = 0; i < imgs.size(); i++)
	{
		cout << i << "/" << imgs.size() << endl;
		cv::Mat srcm = imread(imgs[i]);
		string s = stocr.get_postcode_string(srcm, &cfg);
		cv::Mat dstm;
		//stocr.loacate_anchor_line(srcm, dstm, match_line_mat);

		cout << "结果："<<s << endl;

#ifdef POSTCODE_ROI_DEBUG
		waitKey(0);
#endif // POSTCODE_ROI_DEBUG

		
	}


}





int getMatFromRotateRect(const cv::Mat &src_mat, cv::Mat &dst_mat, cv::RotatedRect rRc)
{
	cv::Rect boxRec = rRc.boundingRect();
	//cv::Rect boxRec_n(0, 0, boxRec.width, boxRec.height);

	cv::Mat padMat = cv::Mat::zeros(boxRec.size(), src_mat.type());
	padMat = ~padMat;
	cv::Rect cropRect = boxRec;
	int res = ImageProcessFunc::CropRect(cv::Rect(0, 0, src_mat.cols, src_mat.rows), cropRect);
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

	cv::Mat affine_matrix = cv::getRotationMatrix2D(cv::Point2f(padMat_e.cols / 2.0f, padMat_e.rows / 2.0f), rRc.angle, 1.0);//求得旋转矩阵    
	cv::warpAffine(padMat_e, padMat_e, affine_matrix, padMat_e.size());     //计算图像旋转之后包含图像的最大的矩形    

	cv::Rect cropRec;
	cropRec.x = padMat_e.cols / 2 - rRc.size.width / 2;
	cropRec.y = padMat_e.rows / 2 - rRc.size.height / 2;
	cropRec.width = rRc.size.width;
	cropRec.height = rRc.size.height;
	ImageProcessFunc::CropRect(cv::Rect(0, 0, padMat_e.cols, padMat_e.rows), cropRec);

	dst_mat = padMat_e(cropRec);

	return 1;
}




void test_tag_detection()
{
	const int image_num = 1;
	std::vector<std::string> model_names;
	model_names.push_back("yolov5_one.onnx");
	model_names.push_back("yolov5_two.onnx");
	model_names.push_back("efficientdetd0_three.onnx");
	model_names.push_back("efficientdetd0_four.onnx");
	std::string exe_dir = CommonFunc::get_exe_dir();
	std::string modelfile = CommonFunc::joinFilePath(exe_dir, model_names[image_num - 1]);
	
	//modelfile = "E:\\python_projects\\Yet-Another-EfficientDet-Pytorch\\efficientdetd0_one.onnx";
	//modelfile = "E:\\python_projects\\Tailand_tag_detect\\RotatedBoundingBox\\efficientdetd0_one.onnx";
	modelfile = "E:\\python_projects\\Thailand_projects\\RBbox_yolov5\\weights\\last.onnx";
	int cuda_ind = 0;

	std::cout << "model path:" << modelfile << std::endl;
	wchar_t modelfilew[512] = { 0 };
	CommonFunc::MCharToWChar(modelfile.c_str(), modelfilew);
	TagDetector parcelRecop(modelfilew, 0.75, cuda_ind, image_num);
	cout << "using cuda:" << cuda_ind << endl;
	//parcelRecop.set_transform(1., 1., 0, 0);
	parcelRecop.initial();

	std::string dir = CommonFunc::joinFilePath(exe_dir, "/images/*.jpg");
	//dir = "F:\\cpte_datasets\\SeperateParcel\\P20200410_images\\*.jpg";
	dir = "E:\\datasets\\thailand_tag\\*.jpg";


	string saving_dir = "E:\\datasets\\thailand_tag_cuts";


	std::cout << "image dir:" << dir << std::endl;
	std::vector<std::string> imgs;
	CommonFunc::getAllFilesNameInDir(dir, imgs, false, true);
	std::cout << "file nums:" << imgs.size() << std::endl;
	double total_consume = 0;
	clock_t t0;
	clock_t t1;
	vector<double> time_range;
	int i = 0;
	for (i = 0; i < imgs.size(); i++)
	{
		double time_csum = 0;
		double time_csum1 = 0;
		std::cout << endl;
		std::cout << i << std::endl;
		cv::Mat srcm = cv::imread(imgs[i]);
		//cv::Mat srcm2 = cv::imread(imgs[i+1]);

		vector<Mat> mats;
		for (int j = 0; j < image_num; j++)
		{
			mats.push_back(srcm);
		}
		//mats.push_back(srcm);
		//mats.push_back(srcm2);

		t0 = clock();
		std::vector<std::vector<cv::RotatedRect>> rrects(image_num);
		std::vector<std::vector<int>> cls_inds(image_num);
		for (int j = 0; j < image_num; j++)
		{
			rrects[j].reserve(MAX_BOX_NUM);
			cls_inds[j].reserve(MAX_BOX_NUM);
		}
		//parcelRecop.detect_mats(mats, points);




		parcelRecop.detectParcels(mats, rrects, cls_inds);

		t1 = clock();
		time_csum = t1 - t0;
		std::cout << "model time comsume:" << time_csum << " ms" << std::endl;
		total_consume += time_csum;
		time_range.push_back(time_csum);


		

		for (int j = 0; j < cls_inds[0].size(); j++)
		{

			Mat tag_mat;
			ImageProcessFunc::getMatFromRotatedRect(mats[0], tag_mat, rrects[0][j]);
			if (!tag_mat.empty())
			{
				t0 = clock();
				string name = to_string(cls_inds[0][j]) +"_"+ to_string(t0) + ".jpg";
				name = CommonFunc::joinFilePath(saving_dir, name);
				cv::imwrite(name, tag_mat);
			}
			
		}




		t0 = clock();

		//std::vector<std::array<cv::Point2f, 4>> merged_points;
		//merge_contours_pos(points[0], points[1], merged_points);
		//sort_contours(merged_points);
		//points[0] = merged_points;
		
		time_csum1 = t0 - t1;
		std::cout << "merge time consume:" << time_csum1 << " ms" << std::endl;
		total_consume += time_csum1;

#ifdef SHOW_IMAGE_TEST
		for (int i = 0; i < mats.size(); i++)
		{
			for (int n = 0; n < rrects[i].size(); n++)
			{
				cv::Point2f pts[4];
				rrects[i][n].points(pts);
				cv::Point pt1;
				cv::Point pt2;
				for (int m = 0; m < 4; m++)
				{
					pt1 = pts[m];
					if (m == 3)
					{
						pt2 = pts[0];
					}
					else
					{
						pt2 = pts[m + 1];
					}
					cv::line(mats[i], pt1, pt2, cv::Scalar(0, 0, 255), 3);

				}
				cv::putText(mats[i], std::to_string(cls_inds[i][n]), pt2, cv::FONT_HERSHEY_PLAIN, 5, Scalar(100, 255, 0),3);
			}
			cv::resize(mats[i], mats[i], cv::Size(), 0.3, 0.3);
			cv::imshow(to_string(i), mats[i]);
		}
		cv::waitKey(0);
#else
		//if (i == 200)
		//{
		//	break;
		//}
#endif // SHOW_IMAGE_TEST
		//Sleep(50);


	}
	std::cout << "平均时间消耗:" << total_consume / (i) << " ms" << std::endl;

	std::sort(time_range.begin(), time_range.end());
	std::cout << "时间波动范围" << time_range[0] << "-" << time_range[time_range.size() - 1] << "ms" << endl;

	system("Pause");




}


int test_arb_ocr()
{
	string dir = "F:\\cpte_datasets\\Tailand_tag_detection_datasets\\tag_cut_img\\rotated_tag_3\\*.jpg";
	string match_line_img = "E:\\cpp_projects\\Thailand_projects\\Projects\\_run_dir\\resource\\match_line_image.jpg";
	string exe_dir = CommonFunc::get_exe_dir();
	std::cout << "image dir:" << dir << std::endl;
	std::vector<std::string> imgs;
	CommonFunc::getAllFilesNameInDir(dir, imgs, false, true);
	std::cout << "file nums:" << imgs.size() << std::endl;
	double total_consume = 0;
	clock_t t0;
	clock_t t1;
	vector<double> time_range;
	OCRArbitaryTag stocr;
	OcrAlgorithm_config cfg;
	string ocr_parrerns_file = CommonFunc::joinFilePath(exe_dir, "tag_patterns_config.txt");
	string config_file = CommonFunc::joinFilePath(exe_dir, "en_ocr_config.txt");

	//format_tess_config_file(ocr_parrerns_file);
	//format_tess_config_file(config_file);

	FILE *pattern_file = fopen(ocr_parrerns_file.c_str(), "rb");
	if (pattern_file != nullptr)
	{
		cout << "pattern_file_Read_ok" << endl;
		fclose(pattern_file);
	}



	char *configs[1];
	//configs[1] = (char*)(ocr_parrerns_file.c_str());
	configs[0] = (char*)(config_file.c_str());

	int configs_size = 1;
	tesseract::TessBaseAPI tess;
	if (tess.Init((exe_dir + "/tessdata").c_str(), "tha", tesseract::OcrEngineMode::OEM_LSTM_ONLY))
	{
#ifdef _DEBUG
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
#endif // _DEBUG
		return 1;
	}

	cfg.pTessThld = &tess;

	//cfg.match_data.getMatchDataFromImg_tag_line("E:\\cpp_projects\\Thailand_projects\\Projects\\_run_dir\\resource\\match_line_image.jpg");





	int i = 0;
	for (i = 77; i < imgs.size(); i++)
	{
		cout << i << "/" << imgs.size() << endl;
		cv::Mat srcm = imread(imgs[i]);
		cv::imshow("src", srcm);
		string ss;
		ss = stocr.get_postcode_string(srcm, &cfg);
		cv::Mat dstm;
		//stocr.loacate_anchor_line(srcm, dstm, match_line_mat);

		cout << "结果：" << ss << endl;

#ifdef ARBITURARY_TAG_DEBUG
		waitKey(0);
#endif // POSTCODE_ROI_DEBUG
	}
	return 1;

}





int main()
{
    std::cout << "Hello World!\n"; 
	cv::Mat m;
	test_tag_detection();
	//test_tag_ocr();
	//test_arb_ocr();




}

