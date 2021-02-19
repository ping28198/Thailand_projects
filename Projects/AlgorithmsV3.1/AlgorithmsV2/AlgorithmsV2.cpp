// AlgorithmsV2.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <opencv2//opencv.hpp>
#include "CommonFunc.h"
#include "PostcodeAlgorithm_v1.h"
#include <algorithm>
#include <fstream>
using namespace std;
using namespace cv;


#define SHOW_IMAGE_TEST

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

	string dir = "E:\\datasets\\ThailandPost\\cuts_0_1\\*.jpg";
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
	if (tess.Init((exe_dir + "/tessdata").c_str(), "digits", tesseract::OcrEngineMode::OEM_LSTM_ONLY, nullptr, 0, NULL, NULL, false))
	{
#ifdef _DEBUG
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
#endif // _DEBUG
		return;
	}

	cfg.pTessEn = &tess;

	cfg.match_data.getMatchDataFromImg_tag_line("E:\\cpp_projects\\Thailand_project\\Projects\\_run_dir\\resource\\match_line_image.jpg");

	cfg.strict_mode = 0;
	cfg.support_postcode_10 = 0;

	int i = 0;
	for (i = 0; i < imgs.size(); i++)
	{
		cout << i << "/" << imgs.size() << endl;
		cv::Mat srcm = imread(imgs[i]);
		t0 = clock();
		string s = stocr.get_postcode_string(srcm, &cfg);
		t1 = clock();
		std::cout << "time consume:" << t1 - t0 << std::endl;
		cv::Mat dstm;
		//stocr.loacate_anchor_line(srcm, dstm, match_line_mat);

		cout << "结果："<<s << endl;
		cout << stocr.get_last_log() << endl;

#ifdef DEBUG_STD_TAG
		waitKey(0);
#endif // POSTCODE_ROI_DEBUG

		
	}


}










void test_tag_detection()
{
	const int image_num = 1;
	std::vector<std::string> model_names;
	model_names.push_back("efficientdetd0_one.onnx");
	model_names.push_back("efficientdetd0_two.onnx");
	model_names.push_back("efficientdetd0_three.onnx");
	model_names.push_back("efficientdetd0_four.onnx");
	std::string exe_dir = CommonFunc::get_exe_dir();
	std::string modelfile = CommonFunc::joinFilePath(exe_dir, model_names[image_num - 1]);
	
	//modelfile = "E:\\python_projects\\Yet-Another-EfficientDet-Pytorch\\efficientdetd0_one.onnx";
	modelfile = "E:\\cpp_projects\\Thailand_project\\Projects\\_run_dir\\model_file\\efficientdetd0_one.onnx";
	int cuda_ind = -1;

	std::cout << "model path:" << modelfile << std::endl;
	wchar_t modelfilew[512] = { 0 };
	CommonFunc::MCharToWChar(modelfile.c_str(), modelfilew);
	TagDetector parcelRecop(modelfilew, 0.75, cuda_ind, image_num);
	cout << "using cuda:" << cuda_ind << endl;
	//parcelRecop.set_transform(1., 1., 0, 0);
	parcelRecop.initial();

	std::string dir = CommonFunc::joinFilePath(exe_dir, "/images/*.jpg");
	//dir = "F:\\cpte_datasets\\SeperateParcel\\P20200410_images\\*.jpg";
	//dir = "E:\\datasets\\ThailandPost\\test_nobox\\CUTs\\*.jpg";
	dir = "E:\\datasets\\ThailandPost\\src_parcels\\*.jpg";

	string saving_dir = "E:\\datasets\\ThailandPost\\thailand_tag_cuts";


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
				string name = to_string(cls_inds[0][j]) + "_" + to_string(t0) + ".jpg";
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
	//string dir = "F:\\cpte_datasets\\Tailand_tag_detection_datasets\\tag_cut_img\\rotated_tag_3\\*.jpg";
	string dir = "E:\\datasets\\ThailandPost\\tag_cuts_arb\\*.jpg";
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
	for (i = 0; i < imgs.size(); i++)
	{
		cout << i << "/" << imgs.size() << endl;
		cv::Mat srcm = imread(imgs[i]);
		cv::imshow("src", srcm);


		t0 = clock();

		string ss;
		ss = stocr.get_postcode_string(srcm, &cfg);
		t1 = clock();
		std::cout << "time consume:" << t1 - t0<<std::endl;
		cv::Mat dstm;
		//stocr.loacate_anchor_line(srcm, dstm, match_line_mat);

		cout << "结果：" << ss << endl;
		waitKey(0);
#ifdef ARBITURARY_TAG_DEBUG
		waitKey(0);
#endif // POSTCODE_ROI_DEBUG
	}
	return 1;

}


void test_ocr_hand_write_box()
{
	//string dir = "F:\\cpte_datasets\\Tailand_tag_detection_datasets\\tag_cut_img\\rotated_tag_3\\*.jpg";
	std::string dir = "E:\\datasets\\ThailandPost\\tag_cuts_hwrt\\*.jpg";
	//string match_line_img = "E:\\cpp_projects\\Thailand_projects\\Projects\\_run_dir\\resource\\match_line_image.jpg";
	string exe_dir = CommonFunc::get_exe_dir();
	std::cout << "image dir:" << dir << std::endl;
	std::vector<std::string> imgs;
	CommonFunc::getAllFilesNameInDir(dir, imgs, false, true);
	std::cout << "file nums:" << imgs.size() << std::endl;
	double total_consume = 0;
	clock_t t0;
	clock_t t1;
	vector<double> time_range;
	OCRHandWriteBox hwocr;
	OcrAlgorithm_config cfg;
	
	std::string modelfile = "E:\\cpp_projects\\Thailand_project\\Projects\\_run_dir\\model_file\\last.onnx";
	int cuda_ind = 0;

	std::cout << "model path:" << modelfile << std::endl;
	wchar_t modelfilew[512] = { 0 };
	CommonFunc::MCharToWChar(modelfile.c_str(), modelfilew);
	hwocr.initial_model(modelfilew, 0.9);



	tesseract::TessBaseAPI tess;
	if (tess.Init((exe_dir + "/tessdata").c_str(), "eng", tesseract::OcrEngineMode::OEM_LSTM_ONLY))
	{
#ifdef _DEBUG
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
#endif // _DEBUG
		return;
	}

	std::vector<std::string> keywords = {"to","10"};
	//ifstream ifs;
	//ifs.open((exe_dir + "/resource/key words.txt").c_str());
	
// 	if (ifs.is_open())
// 	{
// 		while (ifs.peek() != EOF)
// 		{
// 			char tmpstr[32] = { 0 };
// 			ifs.getline(tmpstr, 511);
// 			std::string kstr(tmpstr);
// 			if (!kstr.empty())
// 			{
// 				keywords.push_back(kstr);
// 				std::cout << "key words:" << kstr << std::endl;
// 			}
// 		}
// 	}


	int i = 0;
	for (i = 0; i < imgs.size(); i++)
	{
		cout << i << "/" << imgs.size() << endl;
		cv::Mat srcm = imread(imgs[i]);
#ifdef DEBUG_HAND_WRITE_BOX
		cv::imshow("src", srcm);
#endif // DEBUG_HAND_WRITE_BOX
		
		string ss;
		std::vector<cv::Mat> pbox;
		t0 = clock();
		ss = hwocr.get_postcode_string(srcm);
		t1 = clock();
		//stocr.loacate_anchor_line(srcm, dstm, match_line_mat);
		std::cout << "time consume:" << t1 - t0 << std::endl;
		cout << "结果：" << ss << endl;
		std::cout << hwocr.get_last_log() << std::endl;

		int res = hwocr.find_key_words(srcm, keywords, &tess);
		
		std::cout <<"find key words:"<< to_string(res) << std::endl;



#ifdef DEBUG_HAND_WRITE_BOX
		waitKey(0);
#endif // DEBUG_HAND_WRITE_BOX

		
	}
	return ;

}



void test_handwirte_box_test()
{

	//string dir = "F:\\cpte_datasets\\Tailand_tag_detection_datasets\\tag_cut_img\\rotated_tag_3\\*.jpg";
	std::string dir = "E:\\datasets\\ThailandPost\\tag_cuts_hwrt_box\\*.jpg";
	//string match_line_img = "E:\\cpp_projects\\Thailand_projects\\Projects\\_run_dir\\resource\\match_line_image.jpg";
	string exe_dir = CommonFunc::get_exe_dir();
	std::cout << "image dir:" << dir << std::endl;
	std::vector<std::string> imgs;
	CommonFunc::getAllFilesNameInDir(dir, imgs, false, true);
	std::cout << "file nums:" << imgs.size() << std::endl;
	double total_consume = 0;
	clock_t t0;
	clock_t t1;
	vector<double> time_range;
	OCRHandWriteBox hwocr;
	OcrAlgorithm_config cfg;

	std::string modelfile = "E:\\cpp_projects\\Thailand_project\\Projects\\_run_dir\\model_file\\last.onnx";
	int cuda_ind = 0;

	std::cout << "model path:" << modelfile << std::endl;
	wchar_t modelfilew[512] = { 0 };
	CommonFunc::MCharToWChar(modelfile.c_str(), modelfilew);
	hwocr.initial_model(modelfilew, 0.9);



	tesseract::TessBaseAPI tess;
	if (tess.Init((exe_dir + "/tessdata").c_str(), "eng", tesseract::OcrEngineMode::OEM_LSTM_ONLY))
	{
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
		return;
	}

	std::vector<std::string> keywords = { "to","10" };


	int i = 0;
	for (i = 0; i < imgs.size(); i++)
	{
		cout << i << "/" << imgs.size() << endl;
		cv::Mat srcm = imread(imgs[i]);
		//cv::Mat srcm = imread("E:\\datasets\\ThailandPost\\thailand_tag_cuts\\3_3416.jpg");//2651
		//cv::Mat srcm = imread("E:\\datasets\\ThailandPost\\thailand_tag_cuts\\3_2651.jpg");//
#ifdef DEBUG_HAND_WRITE_BOX
		cv::imshow("src", srcm);
#endif // DEBUG_HAND_WRITE_BOX
		int types = hwocr.identify_handbox_type(srcm);
		string ss;
		std::vector<cv::Mat> pbox;
		t0 = clock();
		if (types==1)
		{
			ss = hwocr.get_postcode_string_test_v2(srcm);
		}
		else
		{
			ss = hwocr.get_postcode_string(srcm);
		}
		
		t1 = clock();
		//stocr.loacate_anchor_line(srcm, dstm, match_line_mat);
		std::cout << "time consume:" << t1 - t0 << std::endl;
		cout << "结果：" << ss << endl;
		std::cout << hwocr.get_last_log() << std::endl;

// 		int res = hwocr.find_key_words(srcm, keywords, &tess);
// 
// 		std::cout << "find key words:" << to_string(res) << std::endl;



#ifdef DEBUG_HAND_WRITE_BOX
		waitKey(0);
#endif // DEBUG_HAND_WRITE_BOX


	}
	return;





}







int main()
{
    std::cout << "Hello World!\n"; 
	cv::Mat m;
	//test_tag_detection();
	//test_tag_ocr();
	//test_arb_ocr();
	//test_ocr_hand_write_box();
	test_handwirte_box_test();



}

