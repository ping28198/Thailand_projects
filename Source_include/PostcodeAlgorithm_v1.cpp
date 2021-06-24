#include "PostcodeAlgorithm_v1.h"
#include <onnxruntime_cxx_api.h>
#ifdef USE_CUDA_DEVICE
#include <cuda_provider_factory.h>
#endif // USE_CUDA_DEVICE
//
#include <array>
#include <iostream>
#include "time.h"
#include <string>
#include <assert.h>
#include "opencv2/opencv.hpp"
//#include "opencv2/cudawarping.hpp"
#include <vector>
#include "ImageProcessFunc.h"
#include <ThreadPool.h>
#include "opencv2/features2d.hpp"
//#include "opencv2/features2d/nonfree.hpp"
#include "CommonFunc.h"
#include <fstream>
using namespace cv;
using namespace std;


int importMat_(void *p_args)
{
	//time_t t0, t1;
	//t0 = clock();
	importMat_args *args = (importMat_args *)p_args;
	cv::Mat srcm = *(args->pMat);
	cv::Mat m;
	cv::cvtColor(srcm, m, COLOR_BGR2GRAY);
	cv::resize(m, m, cv::Size(args->new_width, args->new_height), 0, 0, cv::INTER_AREA);
// 	auto clahe = cv::createCLAHE(3, cv::Size(16, 16));
// 	clahe->apply(m,m);

	cv::copyMakeBorder(m, m, (args->height_ - args->new_height + 1) / 2, (args->height_ - args->new_height) / 2,
		(args->width_ - args->new_width + 1) / 2, (args->width_ - args->new_width) / 2, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
	m.convertTo(m, CV_32FC1);//��ɫ
	//cv::Mat cm[3];
	//cv::split(m, cm);
	//cv::imshow("test", m);
	//cv::waitKey(0);
	for (int c = 0; c < 3; c++)
	{
		memcpy(args->pDst + c * args->width_*args->height_, m.data, sizeof(float)*args->width_*args->height_);
	}
	return 1;
}

TagDetector::TagDetector(const wchar_t* model_path, float confidence_threshold/*=0.3*/, int cuda_id/*=-1*/, size_t input_image_num)
{

	assert(input_image_num <= MAX_IMAGE_NUM);
	//�趨����ͼƬ��������ʵ�������ͼƬС�ڵ��ڸ�ֵ
	this->m_image_num = input_image_num;
	input_shape_[0] = input_image_num;
	output_shape_[0] = input_image_num;
	this->m_confidence_threshold = confidence_threshold;
	this->input_image_ = new std::array<float, MAX_IMAGE_NUM * WIDTH_ * HEIGHT_ * CHANNEL_>;

	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
	input_tensor_ = Ort::Value::CreateTensor<float>(memory_info, input_image_->data(), input_image_->size(), input_shape_.data(), input_shape_.size());
	output_tensor_ = Ort::Value::CreateTensor<float>(memory_info, results_.data(), results_.size(), output_shape_.data(), output_shape_.size());
	initial_model(model_path, cuda_id);
#ifdef MULTI_THREAD_INPUT
	tpool = new ThreadPool(MAX_IMAGE_NUM);
#endif // MULTI_THREAD_INPUT
}


int TagDetector::initial()
{
	std::vector<cv::Mat> ms;
	for (int i = 0; i < m_image_num; i++)
	{
		cv::Mat m = cv::Mat::ones(cv::Size(this->WIDTH_, this->HEIGHT_), CV_8UC3);
		ms.push_back(m);
	}
	std::vector<std::vector<cv::RotatedRect>> points(m_image_num);
	std::vector<std::vector<int>> cls_inds(m_image_num);
	this->detectParcels(ms, points, cls_inds);
	return 1;
}

int TagDetector::initial_model(const wchar_t* model_file, size_t cuda_id /*= 0*/)
{
	assert(model_file != nullptr);
	session_options.SetIntraOpNumThreads(5);
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
// 	if (cuda_id >= 0)
// 		OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, cuda_id);

	//

	// Available levels are
	// ORT_DISABLE_ALL -> To disable all optimizations
	// ORT_ENABLE_BASIC -> To enable basic optimizations (Such as redundant node removals)
	// ORT_ENABLE_EXTENDED -> To enable extended optimizations (Includes level 1 + more complex optimizations like node fusions)
	// ORT_ENABLE_ALL -> To Enable All possible opitmizations

	env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "");

	session_ = new Ort::Session(*env, model_file, session_options);

	return 1;
}

int TagDetector::run_detect()
{
	session_->Run(Ort::RunOptions{ nullptr }, input_node_names.data(), &input_tensor_, 1,
		output_node_names.data(), &output_tensor_, 1);
	return 1;
}

void TagDetector::importMat(std::vector<cv::Mat> & srcms)
{
	importMat_cpu(srcms);
}

void TagDetector::importMat_cpu(std::vector<cv::Mat> & srcms)
{
	std::vector< std::future<int> > results(MAX_IMAGE_NUM);
	//importMat_args image_args[MAX_IMAGE_NUM] = { 0 };
	size_t image_num = srcms.size() <= this->m_image_num ? srcms.size() : this->m_image_num;
	this->m_real_input_num = image_num;
	// #pragma omp paralle for // ͼƬ�����϶�ʱʹ��
	for (int i = 0; i < image_num; i++)
	{
		assert(srcms[i].channels() == CHANNEL_);
		float scal = std::min(float(WIDTH_) / srcms[i].cols, float(HEIGHT_) / srcms[i].rows);
		this->resize_scal[i] = scal;
		int new_width = scal * srcms[i].cols;
		int new_height = scal * srcms[i].rows;
		this->shift_y[i] = (HEIGHT_ - new_height + 1) / 2;
		this->shift_x[i] = (WIDTH_ - new_width) / 2;
		image_args[i].height_ = this->HEIGHT_;
		image_args[i].width_ = this->WIDTH_;
		image_args[i].pDst = this->input_image_->data() + i * CHANNEL_ * WIDTH_ * HEIGHT_;
		image_args[i].pMat = &(srcms[i]);
		image_args[i].new_width = new_width;
		image_args[i].new_height = new_height;
		image_args[i].channel_ = CHANNEL_;
#ifdef MULTI_THREAD_INPUT
		results.emplace_back(
			tpool->enqueue([i](void* pargs) {
			return importMat_(pargs);
		}, (&image_args[i])));
#else
		importMat_(&image_args[i]);
#endif // 
	}
#ifdef MULTI_THREAD_INPUT
	for (auto && result : results)
		result.get();
#endif // MULTI_THREAD_INPUT
	}

void TagDetector::importMat_gpu(std::vector<cv::Mat> &srcms)
{

}

void TagDetector::convert_to_rrects(std::array<float, MAX_IMAGE_NUM * MAX_BOX_NUM * FEATS_PER> *src_data,
	std::vector<std::vector<cv::RotatedRect>> & rrects, std::vector<std::vector<int>> &cls_inds)
{
	for (int n = 0; n < m_real_input_num; n++)
	{
		std::vector<std::array<cv::Point2f, 4>> parcels;
		//parcels.reserve(MAX_BOX_NUM);
		std::vector<cv::RotatedRect> one_res;
		//one_res.reserve(MAX_BOX_NUM);
		std::vector<int> cls_;
		cv::Point2f _cent;
		cv::Point2f _size;
		cv::RotatedRect rrc;
		for (int b = 0; b < MAX_BOX_NUM; b++)
		{
			float *pData = src_data->data() + n * MAX_BOX_NUM * FEATS_PER + b * FEATS_PER;
#ifdef DEBUG_ONNX_EFFICIENTDET_R0
			printf("detect box %d: %f %f %f %f %f %f %f %f\n", b, pData[0], pData[1], pData[2], pData[3],
				pData[4], pData[5], pData[6], pData[7]);
#endif
			if (pData[0] < 0.5) break; //�жϽ�β
			//if (pData[1] > 0.998) continue; //����
			if (pData[1] < this->m_confidence_threshold) continue;


			std::array<float, 8> points_m;
			_size.x = pData[6] < 1 ? 1 : pData[6];
			_size.y = pData[5] < 1 ? 1 : pData[5];
			_cent.x = pData[3];
			_cent.y = pData[4];
			float theta = -pData[7] / CV_PI * 180;
			//theta = (theta > 90) ? (90 - theta) : theta;
			rrc.angle = theta;
			rrc.center = _cent;
			rrc.size = _size;
			one_res.push_back(rrc);
			cls_.push_back(floor(pData[2]));
		}
		nms_multi_class(one_res, cls_, this->m_iou_threshold);
		cv::Point2f scal_pt(shift_x[n], shift_y[n]);

		//�ظ���ԭʼͼ��ߴ�
		for (int i = 0; i < one_res.size(); i++)
		{
			one_res[i].center -= scal_pt;
			one_res[i].center.x /= resize_scal[n];
			one_res[i].center.y /= resize_scal[n];
			one_res[i].size.height /= resize_scal[n];
			one_res[i].size.width /= resize_scal[n];
		}

		rrects[n] = one_res;
		cls_inds[n] = (cls_);
	}

}

void TagDetector::nms_rotated_rect(std::vector<cv::RotatedRect> &rects, float iou_threshold)
{

	std::vector<cv::RotatedRect>::iterator it0;
	std::vector<cv::RotatedRect>::iterator it1;
	cv::Point c_dist_pt;
	if (rects.size() <= 1)
		return;

	for (it0 = rects.begin(); it0 != rects.end(); it0++)
	{
		it1 = it0 + 1;
		for (; it1 != rects.end();)
		{
			c_dist_pt = it0->center - it1->center;
			c_dist_pt.x = abs(c_dist_pt.x);
			c_dist_pt.y = abs(c_dist_pt.y);
			int c_dist = std::max(c_dist_pt.x, c_dist_pt.y) + std::min(c_dist_pt.x, c_dist_pt.y) / 2;
			int edge_dist = std::max(it0->size.width, it0->size.height) / 2 + std::max(it1->size.width, it1->size.height) / 2;
			if (c_dist > edge_dist)
			{
				it1++;
				continue;
			}
			std::vector<cv::Point2f> contour;
			cv::rotatedRectangleIntersection(*it0, *it1, contour);
			if (contour.size() < 3)
			{
				it1++;
				continue;
			}
			double contour_area = cv::contourArea(contour);
			if (contour_area / (it0->size.area() + it1->size.area() - contour_area) > iou_threshold)
			{
				it1 = rects.erase(it1);
			}
			else
			{
				it1++;
			}
		}
	}
}

bool sort_func_rects_by_cls(std::pair<cv::RotatedRect, int>& a, std::pair<cv::RotatedRect, int>&b)
{
	return a.second > b.second;
}


void TagDetector::nms_multi_class(std::vector<cv::RotatedRect> &rects, std::vector<int> &class_ids, float iou_threshold)
{
	if (rects.size() <= 1)
	{
		return;
	}
	std::pair<cv::RotatedRect, int> rct;
	std::vector<std::pair<cv::RotatedRect, int>> rects_cls;
	rects_cls.reserve(rects.size());
	for (size_t i = 0; i < rects.size(); i++)
	{
		rct.first = rects[i];
		rct.second = class_ids[i];
		rects_cls.push_back(rct);
	}
	std::vector<cv::RotatedRect> _tp;
	rects.swap(_tp);
	std::vector<int> _cl;
	class_ids.swap(_cl);

	std::sort(rects_cls.begin(), rects_cls.end(), sort_func_rects_by_cls);
	size_t same_cls_count = 0;
	size_t cls = rects_cls[0].second;
	for (size_t i = 1; i < rects_cls.size() + 1; i++)
	{
		if (i == rects_cls.size())
		{
			if (same_cls_count == 0)
			{
				rects.push_back(rects_cls[i - 1].first);
				class_ids.push_back(cls);
				same_cls_count = 0;
			}
			else
			{
				std::vector<cv::RotatedRect> rt;
				for (int j = i - same_cls_count - 1; j < i; j++)
				{
					rt.push_back(rects_cls[j].first);
				}
				nms_rotated_rect(rt, iou_threshold);
				rects.insert(rects.end(), rt.begin(), rt.end());
				for (int j = 0; j < rt.size(); j++)
				{
					class_ids.push_back(cls);
				}
				same_cls_count = 0;
			}
			break;
		}

		if (rects_cls[i].second != cls)
		{
			if (same_cls_count == 0)
			{
				rects.push_back(rects_cls[i - 1].first);
				class_ids.push_back(cls);
				same_cls_count = 0;
			}
			else
			{
				std::vector<cv::RotatedRect> rt;
				for (int j = i - same_cls_count - 1; j < i; j++)
				{
					rt.push_back(rects_cls[j].first);
				}
				nms_rotated_rect(rt, iou_threshold);
				rects.insert(rects.end(), rt.begin(), rt.end());
				for (int j = 0; j < rt.size(); j++)
				{
					class_ids.push_back(cls);
				}
				same_cls_count = 0;
			}

			cls = rects_cls[i].second;

		}
		else
		{
			same_cls_count++;
		}
	}
}

void TagDetector::detectParcels(std::vector<cv::Mat> &srcms, std::vector<std::vector<cv::RotatedRect>> & rrects,
	std::vector<std::vector<int>> &cls_inds)
{

	this->m_real_input_num = srcms.size();

#ifdef DEBUG_ONNX_EFFICIENTDET_R0
	clock_t t0 = clock();
	importMat(srcms);
	clock_t t1 = clock();
	double timeconsume = (double)(t1 - t0);
	std::cout << "pre-process time consume:" << timeconsume << "ms" << std::endl;
	run_detect();
	t0 = clock();
	timeconsume = (double)(t0 - t1);
	std::cout << "model time consume:" << timeconsume << "ms" << std::endl;
	convert_to_rrects(&results_, rrects, cls_inds);
	t1 = clock();
	timeconsume = (double)(t1 - t0);
	std::cout << "post-process time consume:" << timeconsume << "ms" << std::endl;
#else

	importMat(srcms);
	run_detect();
	convert_to_rrects(&results_, rrects, cls_inds);

#endif // DEBUG_ONNX_EFFICIENTDET_R0

}

OCRStandardTag::OCRStandardTag()
{

}

std::string OCRStandardTag::get_postcode_string(cv::Mat tag_mat, OcrAlgorithm_config *pConfig)
{
	log_str = "";
	if (tag_mat.empty())
	{
		log_str += "src image is empty,";
		return "";
	}
	if (tag_mat.cols<50||tag_mat.rows<50)
	{
		log_str += "src image width or height is too small,";
		return "";
	}


	string postcodestr;
	cv::Rect rt(tag_mat.cols/2,0,tag_mat.cols/2,tag_mat.rows/2);
	cv::Mat rtmat;
	tag_mat(rt).copyTo(rtmat);
	//cv::Mat match_linemat = cv::imread("E:\\cpp_projects\\Thailand_projects\\Projects\\_run_dir\\resource\\match_line_image.jpg");
	cv::Mat textrange_mat;
	int res = locate_text_range(rtmat, textrange_mat, pConfig); //定位右上角文字区域
	if (res==0 || textrange_mat.empty())
	{
		log_str += "do not locate text range,";
#ifdef DEBUG_STD_TAG
		cv::waitKey(5);
#endif // DEBUG_STD_TAG

		return postcodestr;
	}
	cv::Mat post_code_line;
	res = get_postcode_line(textrange_mat, post_code_line); // 定位邮编行
	if (!res)
	{
		res = get_postcode_line(rtmat, post_code_line); //如果定位邮编四百，则使用未经过旋转的图片再次定位。
	}
	if (!post_code_line.empty())
	{
		_run_ocr(post_code_line, postcodestr, pConfig);
		format_postcode(postcodestr,pConfig);
	}
#ifdef DEBUG_STD_TAG
	cv::waitKey(5);
#endif // DEBUG_STD_TAG
	return postcodestr;
}


std::string OCRStandardTag::get_last_log()
{
	return log_str;
}

int OCRStandardTag::get_postcode_line(cv::Mat srcm, cv::Mat &dstm)
{
	if (srcm.empty())
	{
		log_str += "get post line image is empty";
		return 0;
	}
	cv::Mat roiMat;
	srcm.copyTo(roiMat);
	if (roiMat.channels() == 3)
		cvtColor(roiMat, roiMat, COLOR_BGR2GRAY);

	float scal_ = 150.0 / roiMat.rows;
	cv::resize(roiMat, roiMat, cv::Size(), scal_, scal_);

	int g_width = roiMat.cols;


	cv::normalize(roiMat, roiMat, 255, 0, cv::NORM_MINMAX);


	cv::Rect adjr = cv::Rect(0, roiMat.rows / 2, roiMat.cols, roiMat.rows / 2);
	float pix = ImageProcessFunc::getAveragePixelInRect(roiMat, adjr);
	cv::Mat g_img = roiMat;
	ImageProcessFunc::adJustBrightness(g_img, 1.5, 2, pix*0.6);

	
	//cv::GaussianBlur(roiMat, g_img, cv::Size(3, 3), 0);
	cv::Canny(g_img, g_img, 20, 150);

#ifdef DEBUG_STD_TAG
	cv::imshow("canny处理结果", g_img);
#endif // DEBUG_STD_TAG

	threshold(g_img, g_img, 30, 255, THRESH_BINARY);
	//bitwise_not(g_img, g_img);



	g_img = g_img(Rect(0, 0, g_img.cols-30, g_img.rows));
	g_img = g_img.clone();


#ifdef DEBUG_STD_TAG
	imshow("形态学处理前图像", g_img);
#endif // DEBUG_STD_TAG


	//形态学运算
	Mat element = getStructuringElement(MORPH_RECT, Size(30, 1));
	morphologyEx(g_img, g_img, MORPH_CLOSE, element);

	element = getStructuringElement(MORPH_RECT, Size(7, 7));
	morphologyEx(g_img, g_img, MORPH_ERODE, element);

	element = getStructuringElement(MORPH_RECT, Size(55, 5));
	morphologyEx(g_img, g_img, MORPH_OPEN, element);


#ifdef DEBUG_STD_TAG
	imshow("形态学处理后图像", g_img);
#endif // DEBUG_STD_TAG
	//
	//waitKey(0);


	//缩小统计区域
	cv::Mat summat;
	g_img(Rect(g_img.cols / 3, 0, g_img.cols * 2 / 3, g_img.rows)).copyTo(summat);

	// 计算可能包含文字的行
	vector<unsigned int>PixelsAdd;
	ImageProcessFunc::sumPixels(summat, 0, PixelsAdd);

	//将
	unsigned int max_pixes = 0;
	int isolate_count = 0;
	bool is_continue = false;
	vector<Point> bars_vec;
	Point bar_pt;
	for (int i = 0; i < PixelsAdd.size(); i++)
	{
		if (PixelsAdd[i] > 0 && is_continue == false)
		{
			is_continue = true;
			bar_pt.x = i;
		}
		if (is_continue == true && PixelsAdd[i] == 0)
		{
			bar_pt.y = i - 1;
			is_continue = false;
			bars_vec.push_back(bar_pt);
		}
		if (i == PixelsAdd.size() - 1 && is_continue == true)
		{
			bar_pt.y = i - 1;
			is_continue = false;
			bars_vec.push_back(bar_pt);
		}

	}

	// 联通短暂的不连续区域
	if (bars_vec.size() == 0)
	{
		log_str += "num of text line exception,";
#ifdef DEBUG_STD_TAG
		printf("未找到字符行\n");
#endif // DEBUG_STD_TAG
		return 0;
	}
	//Point pre_pt = bars_vec[0];
	vector<Point>::iterator it;
	vector<Point>::iterator pre_it = bars_vec.begin();
	for (it = bars_vec.begin() + 1; it != bars_vec.end();)
	{
		if ((it->x - pre_it->y) <= 2)
		{
			pre_it->y = it->y;
			it = bars_vec.erase(it);
		}
		else
		{
			pre_it = it;
			it++;
		}

	}
	if (bars_vec.size() == 0)
	{
		log_str += "num of text line exception,";
#ifdef DEBUG_STD_TAG
		printf("联通字符行后，未找到字符行\n");
#endif // DEBUG_STD_TAG
		return 0;
	}


	// 删除不连续行区域，如果区域高小于3，者删除之
	for (it = bars_vec.begin(); it != bars_vec.end(); )
	{
		if (it->y - it->x < 3)
		{
			it = bars_vec.erase(it);
		}
		else
		{
			it++;
		}
	}

	if (bars_vec.size() == 0)
	{
		log_str += "num of text line exception,";
#ifdef DEBUG_STD_TAG
		printf("未找到字符行\n");
#endif // DEBUG_STD_TAG
		return 0;
	}

	// 删除干扰区域，如果下一个区域位置超过本区域高的两倍，认为不是正确区域
	pre_it = bars_vec.begin();
//	for (it = bars_vec.begin() + 1; it != bars_vec.end();)
//	{
//		if ((it->x - pre_it->y) > 2 * (pre_it->y - pre_it->x))
//		{
//			int n = it - bars_vec.begin();
//			if (n >= 3)
//			{
//				break;
//			}
//			it = bars_vec.erase(bars_vec.begin(), it);
//			pre_it = it;
//			it++;
//		}
//		else
//		{
//			pre_it = it;
//			it++;
//		}
//
//	}
	if (bars_vec.size() < 3)
	{
		log_str += "num of text line exception,";
#ifdef DEBUG_STD_TAG
		printf("字符行不正确\n");
#endif // DEBUG_STD_TAG
		return 0;
	}

	//判断是否出现两行粘连
	vector<int> bars_pos_y_vec;
	for (int i = 0; i < 3; i++)
	{
		bars_pos_y_vec.push_back(bars_vec[i].y - bars_vec[i].x);
		//cout << "bars " << i << " pos:" << bars_pos_y_vec[i] << endl;
	}
	std::sort(bars_pos_y_vec.begin(), bars_pos_y_vec.end());

//	if (bars_pos_y_vec[2] >= 2 * bars_pos_y_vec[0])
//	{
//#ifdef DEBUG_STD_TAG
//		printf("出现行粘连\n");
//#endif // DEBUG_STD_TAG
//		return 0;
//	}
//


	// 确定邮编行区域
	int bar_pos = (bars_vec[2].x + bars_vec[2].y) / 2;
	int bar_width = bars_vec[2].y - bars_vec[2].x;

	int postcode_h = bar_width + 16;
	int postcode_y = (bar_pos - postcode_h / 2 < 0) ? 0 : (bar_pos - postcode_h / 2);
	Rect Rec_postcode = Rect(0, postcode_y, g_width, postcode_h);
	//cv::rectangle(roiMat, Rec_postcode, Scalar(255, 255, 255), 2);
	if (postcode_y + postcode_h >= srcm.rows)
	{
		log_str += "text line location out of range";
#ifdef DEBUG_STD_TAG
		printf("邮编定位超出界限\n");
#endif // DEBUG_STD_TAG
		return 0;
	}
	Rec_postcode.x = Rec_postcode.x / scal_;
	Rec_postcode.y = Rec_postcode.y / scal_;
	Rec_postcode.width = Rec_postcode.width / scal_;
	Rec_postcode.height = Rec_postcode.height / scal_;
	ImageProcessFunc::CropRect(cv::Rect(0,0,srcm.cols,srcm.rows), Rec_postcode);

	srcm(Rec_postcode).copyTo(dstm);

	return 1;
}

int OCRStandardTag::_run_ocr(cv::Mat post_code_line, std::string &results, OcrAlgorithm_config *pConfig)
{
	if (post_code_line.empty())
	{
		log_str += "post code line is empty,";
		return 0;
	}
	Mat srcm;
	post_code_line.copyTo(srcm);
	if (srcm.channels() == 3)
	{
		cv::cvtColor(srcm, srcm, COLOR_BGR2GRAY);
	}
	///////////// 预处理
	int h = srcm.rows;
	int sh = 20;
	float scal_ = sh / float(h);
	cv::Mat resizedMat;
	if (scal_ > 1.1)
	{
		cv::resize(srcm, resizedMat, cv::Size(), scal_, scal_, cv::INTER_AREA);
	}
	else
	{
		srcm.copyTo(resizedMat);
	}

	cv::Rect mR;
	mR.x = 0;
	mR.y = resizedMat.rows/2;
	mR.height = resizedMat.rows/2;
	mR.width = resizedMat.cols / 2;
	cv::normalize(resizedMat, resizedMat, 255, 0, cv::NORM_MINMAX);



	double vPix = ImageProcessFunc::getAveragePixelInRect(resizedMat, mR);
	double anchor = 120;
	double alpha = 3.0;
	double beta = 200 - vPix;
	ImageProcessFunc::adJustBrightness(resizedMat, alpha, beta, anchor);

	//cv::threshold(resizedMat, resizedMat, vPix*0.6, 255, cv::THRESH_BINARY);
#ifdef DEBUG_STD_TAG
	imshow("_runOcr调整亮度后", resizedMat);
#endif // OCR_DEBUG
	int w = resizedMat.cols;
	h = resizedMat.rows;
	unsigned char *pImgData = resizedMat.data;

	//cout << "h" << w << endl;

	tesseract::TessBaseAPI* pTess = (tesseract::TessBaseAPI*)pConfig->pTessEn;
	if (pTess ==NULL)
	{
		log_str += "tesseract engine is Null,";
		return 0;
	}
	pTess->SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_LINE);
	//pTess->SetVariable("save_best_choices", "T");
	pTess->SetImage(pImgData, w, h, resizedMat.channels(), resizedMat.step1());
	pTess->SetVariable("user_defined_api","300");
	pTess->Recognize(0);

	// get result and delete[] returned char* string
#ifdef DEBUG_STD_TAG
	std::cout << std::unique_ptr<char[]>(pTess->GetUTF8Text()).get() << std::endl;
#endif // OCR_DEBUG
	//
	
	char *res_data = pTess->GetUTF8Text();
	size_t max_len = 512;
	size_t str_len = std::min(max_len, strlen(res_data));
	results = std::string(res_data, str_len);
	delete[] res_data;

	return 1;
}

int OCRStandardTag::locate_text_range(cv::Mat srcm,cv::Mat &dstm, OcrAlgorithm_config *pConfig)
{
	const int MAX_KEYPOINTS = 50;
	using namespace cv;
	using namespace std;
	//using namespace cv::;

	 //参考的图像宽高。
	cv::Mat mcp;
	srcm.copyTo(mcp);
	//cv::Mat im2Gray = match_m;

	if (mcp.channels() == 3)
		cv::cvtColor(mcp, mcp, COLOR_BGR2GRAY);
	//if (match_m.channels() == 3)
	//	cv::cvtColor(match_m, im2Gray, COLOR_BGR2GRAY);
	
	float scal = 200.0/ mcp.rows;
	cv::resize(mcp, mcp, cv::Size(), scal, scal);

	//scal = 50.0 / im2Gray.rows;
	//cv::resize(im2Gray, im2Gray, cv::Size(), scal, scal);

	cv::Mat im1Gray;
	cv::blur(mcp, im1Gray, cv::Size(3,3));
	//cv::GaussianBlur(srcm, im1Gray, cv::Size(3, 3), 0);

	cv::normalize(im1Gray, im1Gray, 255, 0, cv::NORM_MINMAX);
	
// 	cv::Rect adjr = cv::Rect(0, im1Gray.rows / 2, im1Gray.cols, im1Gray.rows / 2);
// 	float pix = ImageProcessFunc::getAveragePixelInRect(im1Gray, adjr);
//  ImageProcessFunc::adJustBrightness(im1Gray, 1.3, 120 - pix, pix*0.5);



#ifdef DEBUG_STD_TAG
	cv::imshow("tagsrc", im1Gray);
	//Mat result;
	//drawMatches(im1Gray, n_keypoints1, im2Gray, n_keypoints2, matches, result, Scalar(0, 255, 0), Scalar::all(-1));//匹配特征点绿色，单一特征点颜色随机
	//imshow("Match_Result", result);
#endif // DEBUG_STD_TAG



	cv::Size refSize(269 + 10, 50*3.5);


	std::vector<KeyPoint> keypoints1, keypoints2;
	cv::Mat descriptors1, descriptors2;
	cv::Ptr<Feature2D> sift1 = cv::SIFT::create(MAX_KEYPOINTS*4);
	//cv::Ptr<Feature2D> sift2 = cv::xfeatures2d::SIFT::create(MAX_KEYPOINTS);
	sift1->detectAndCompute(im1Gray, Mat(), keypoints1, descriptors1);
	if (keypoints1.empty() || descriptors1.empty())
	{
		log_str += "sift key points empty, ";
		return 0;
	}
	//sift2->detectAndCompute(im2Gray, Mat(), keypoints2, descriptors2);

	keypoints2 = pConfig->match_data.keypoints_tag_line;
	descriptors2 = pConfig->match_data.descriptors_tag_line;

	//调试用
	//Mat img_keypoints_1, img_keypoints_2;
	//cv::drawKeypoints(im1Gray, keypoints1, img_keypoints_1, Scalar::all(-1));
	//cv::drawKeypoints(im2Gray, keypoints2, img_keypoints_2, Scalar::all(-1));
	//cv::imshow("img_keypoints_1", img_keypoints_1);
	//cv::imshow("img_keypoints_2", img_keypoints_2);


	std::vector<DMatch> matches;

	cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create("BruteForce");
	matcher->match(descriptors2, descriptors1, matches, Mat());

	if (matches.empty())
	{
		log_str += "sift matched points is empty,";
#ifdef DEBUG_STD_TAG
		cout << "匹配点数小于4" << endl;
#endif // DEBUG_STD_TAG
		return 0;
	}

	if (matches.size() < 4)
	{
		log_str += "sift matched points num is lower than 4,";
#ifdef DEBUG_STD_TAG
		cout << "匹配点数小于4" << endl;
#endif // DEBUG_STD_TAG
		return 0;
	}




	//剔除不匹配的keypoint
	float keypoint_percent = 0.6;
	int valid_num_kpt = int(MAX_KEYPOINTS*keypoint_percent);

	if (matches.size()>valid_num_kpt)
	{
		nth_element(matches.begin(), matches.begin() + int(MAX_KEYPOINTS*keypoint_percent), matches.end());
		matches.erase(matches.begin() + valid_num_kpt, matches.end());
	}

	
	
	   //剔除掉其余的匹配结果
	std::vector<KeyPoint> n_keypoints1, n_keypoints2;

	int ind = 0;
	for (int i=0;i<matches.size();i++)
	{
		if (keypoints1[matches[i].trainIdx].pt.y > im1Gray.rows*0.6)
			continue;
		n_keypoints1.push_back(keypoints1[matches[i].trainIdx]);
		n_keypoints2.push_back(keypoints2[matches[i].queryIdx]);
		matches[ind].trainIdx = ind;
		matches[ind].queryIdx = ind;
		ind++;
	}
	if( ind != matches.size())
		matches.erase(matches.begin() + ind, matches.end());


	if (matches.size() < 4)
	{
		log_str += "sift matched points num is lower than 4,";
#ifdef DEBUG_STD_TAG
		cout << "匹配点数小于4" << endl;
#endif // DEBUG_STD_TAG
		return 0;
	}


#ifdef DEBUG_STD_TAG
	//Mat result;
	//drawMatches(im1Gray, n_keypoints1, im2Gray, n_keypoints2, matches, result, Scalar(0, 255, 0), Scalar::all(-1));//匹配特征点绿色，单一特征点颜色随机
	//imshow("Match_Result", result);
#endif // DEBUG_STD_TAG

	

	std::vector<Point2f> points_1, points_ref;
	for (size_t i = 0; i < matches.size(); i++)
	{
		points_1.push_back(n_keypoints1[matches[i].trainIdx].pt);
		points_ref.push_back(n_keypoints2[matches[i].queryIdx].pt);
	}
	Mat h1 = cv::findHomography(points_1, points_ref, cv::RANSAC);
	if (h1.empty())
	{
		log_str += "not find homography mat, ";
		return 0;
	}

	//std::cout << "h1:" << cv::format(h1, cv::Formatter::FMT_NUMPY) << std::endl;
	cv::warpPerspective(mcp, dstm, h1, refSize);

	if (dstm.empty())
	{
		log_str += "warpPerspective image output empty";
		return 0;
	}
// 	cv::imshow("src", srcm);
// 	cv::imshow("dst", dstm);
// 	cv::waitKey(0);

#ifdef DEBUG_STD_TAG
	cv::imshow("dstimg", dstm);
#endif // DEBUG_STD_TAG
	return 1;

}

int OCRStandardTag::format_postcode(std::string &str, OcrAlgorithm_config *pConfig)
{

	string results;
	double score = postcodeStringScore(str, results,pConfig);

	if (score>0.5)
	{
		str = results;
	}
	else
	{
		log_str += "digits score is lower than 0.5";
		str = "";
	}
	return 1;
}

double OCRStandardTag::continuousDigitsScore(std::string srcStr, int continus_num)
{
	if (continus_num <= 0) return 0.0;
	if (srcStr.empty()) return 0.0;

	for (int i = 0; i < srcStr.length();)//剔除空格
	{
		unsigned char c = srcStr.at(i);
		if (c == ' ')
		{
			srcStr.erase(i, 1);
		}
		else
		{
			i++;
		}
	}

	int src_length = srcStr.length();

	int m_continues_num = maxNumContinuousDigits(srcStr);

	if (m_continues_num < continus_num) return 0.0;

	///为连续数字的质量打分
	double conti_socre = 1.0 - (m_continues_num - continus_num) / double(continus_num);
	conti_socre = max(conti_socre, 0.0);

	///为字符串长度打分
	double length_socre = 1.0 - (src_length - continus_num) / (1.2*double(continus_num));//对分数的影响打折
	length_socre = max(length_socre, 0.0);

	//返回复合分数
	return conti_socre * length_socre;

}

int OCRStandardTag::maxNumContinuousDigits(std::string srcStr)
{
	if (srcStr.empty()) return 0;
	int max_digits = 0;
	for (int i = 0; i < srcStr.size();)
	{
		unsigned char c = srcStr.at(i);
		if (isdigit(c))
		{
			int digits_num = 1;
			int j = i + 1;
			for (; j < srcStr.size(); j++)
			{
				unsigned char c = srcStr.at(j);
				if (isdigit(c))
				{
					digits_num++;
				}
				else
				{
					break;
				}
			}
			i = j + 1;
			max_digits = (max_digits < digits_num) ? digits_num : max_digits;
		}
		else
		{
			i++;
		}
	}
	return max_digits;
}
size_t OCRStandardTag::getFirstContinuousDigits(std::string srcStr, int conti_num, std::string &dstStr)
{
	unsigned char c;
	for (int i = 0; i < srcStr.length();)
	{
		c = srcStr[i];
		if (isdigit(c))
		{
			int j = i + 1;
			for (; j < srcStr.size(); j++)
			{
				c = srcStr[j];
				if (isdigit(c))
				{
					if (j - i + 1 == conti_num)
					{
						dstStr = srcStr.substr(i, conti_num);
						return 1;
					}
				}
				else
				{
					break;
				}
			}
			i = j + 1;
		}
		else
		{
			i++;
		}

	}
	return 0;
}
double OCRStandardTag::postcodeStringScore(std::string srcStr, std::string &resultStr, OcrAlgorithm_config *pConfig)
{
	//////////////////////////////////////////////////////////////////////////
//根据设定
	bool strict_mode = 0;
	bool support_code_num_10 = 0;
	int std_postcoce_num = 5;

	if (srcStr.size()<std_postcoce_num)
	{
		log_str += "post code length is lower than 4";
		return 0;
	}



	if (pConfig)
	{
		strict_mode = pConfig->strict_mode;
		support_code_num_10 = pConfig->support_postcode_10;
		std_postcoce_num = pConfig->std_postcode_num;
	}


	int length_left = std_postcoce_num;
	int length_right_4 = 4;
	int length_right_5 = std_postcoce_num;
	if (srcStr.size() < length_left) return 0;

	double whole_score = 0;
	///去除空格
	if (!strict_mode)
	{
		for (int i = 0; i < srcStr.length();)//剔除空格
		{
			unsigned char c = srcStr.at(i);
			if (c == ' ' || c == '\n')
			{
				srcStr.erase(i, 1);
			}
			else
			{
				i++;
			}
		}
	}



	//////////////////////////////////////////////////////////////////////////
//有“-”的情况 5+5,5+4的情况,
	size_t _pos = srcStr.find('-');
	if (_pos != srcStr.npos)
	{
		//判断-号位置，

		std::string substr1 = srcStr.substr(0, _pos);
		std::string substr2 = srcStr.substr(_pos + 1);

		double score_left = continuousDigitsScore(substr1, length_left);

		std::string resStr1, resStr2;
		getFirstContinuousDigits(substr1, length_left, resStr1);

		double score_right_5 = continuousDigitsScore(substr2, length_right_5); 
		double score_right_4 = 1;
		if (support_code_num_10)
		{
			score_right_4 = continuousDigitsScore(substr2, length_right_4);
		}
		
		double score_right = max(score_right_5, score_right_4);
		int max_length_ = (score_right_5 < score_right_4) ? length_right_4 : length_right_5;
		getFirstContinuousDigits(substr2, max_length_, resStr2);
		resultStr = resStr1 +"-"+ resStr2;

		whole_score = score_left * score_right;

		return whole_score;
		//}
	}

	//是否考虑-被识别为空格的情况,这种情况只允许连续10个数字
	double score_full_10 = continuousDigitsScore(srcStr, length_left + length_right_5);
	if (score_full_10 > 0.8)
	{
		int res = getFirstContinuousDigits(srcStr, length_left + length_right_5, resultStr);
		if (res)
		{
			whole_score = score_full_10;
		}
	}
	return whole_score;
}


int MatchDataStruct::getMatchDataFromImg_tag_line(const std::string &refImg1)
{
	const int MAX_KEYPOINTS = 50;
	cv::Mat referenceMat1 = imread(refImg1);
	if (referenceMat1.empty())
	{
		return 0;
	}

	float scal = 50.0 / referenceMat1.rows;
	cv::resize(referenceMat1, referenceMat1, cv::Size(), scal, scal);

	cv::Mat im2Gray = referenceMat1;

	if (referenceMat1.channels() == 3)
	{
		cv::cvtColor(referenceMat1, im2Gray, COLOR_BGR2GRAY);
	}

	// Detect ORB features and compute descriptors.
	cv::Ptr<Feature2D> sift1 = cv::SIFT::create(MAX_KEYPOINTS);

	sift1->detectAndCompute(im2Gray, Mat(), keypoints_tag_line, descriptors_tag_line);

	return 1;
}

OCRArbitaryTag::OCRArbitaryTag()
{

}

std::string OCRArbitaryTag::get_postcode_string(cv::Mat tag_mat, OcrAlgorithm_config *pConfig)
{
	log_str = "";
	m_results = "";
	if (tag_mat.empty())
	{
		log_str += "src image is empty,";
		return "";
	}
	
	_run_ocr(tag_mat, m_results, pConfig);

	return format_results(m_results);
}


std::string OCRArbitaryTag::get_last_log()
{
	return log_str;
}

std::string OCRArbitaryTag::get_last_full_ocr_data()
{
	return m_results;
}

int OCRArbitaryTag::_run_ocr(cv::Mat post_code_line, std::string &results, OcrAlgorithm_config *pConfig)
{
	if (post_code_line.empty())
	{
		log_str += "tag mat is empty, ";
		return 0;
	}


	Mat srcm;
	post_code_line.copyTo(srcm);
	if (srcm.channels() == 3)
	{
		cv::cvtColor(srcm, srcm, COLOR_BGR2GRAY);
	}

	cv::Rect mR;
	mR.x = 0;
	mR.y = srcm.rows / 2;
	mR.height = srcm.rows / 2;
	mR.width = srcm.cols / 2;

	cv::normalize(srcm, srcm, 255, 0, cv::NORM_MINMAX);

	double vPix = ImageProcessFunc::getAveragePixelInRect(srcm, mR);
	double anchor = 120;
	double alpha = 3.0;
	double beta = 200 - vPix;

	ImageProcessFunc::adJustBrightness(srcm, alpha, beta, anchor);

	//cv::threshold(resizedMat, resizedMat, vPix*0.6, 255, cv::THRESH_BINARY);
#ifdef ARBITURARY_TAG_DEBUG
	imshow("_runOcr调整亮度后", srcm);
#endif // OCR_DEBUG
	int w = srcm.cols;
	int h = srcm.rows;
	unsigned char *pImgData = srcm.data;

	//cout << "h" << w << endl;

	tesseract::TessBaseAPI* pTess = (tesseract::TessBaseAPI*)pConfig->pTessThld;
	if (pTess == NULL)
	{
		log_str += "tesseract pointer is Null,";
		return 0;
	}
	pTess->SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_BLOCK);
	//pTess->SetVariable("save_best_choices", "T");
	pTess->SetImage(pImgData, w, h, srcm.channels(), srcm.step1());
	pTess->SetVariable("user_defined_api", "72");
	pTess->Recognize(0);

	// get result and delete[] returned char* string
#ifdef ARBITURARY_TAG_DEBUG
	std::cout << std::unique_ptr<char[]>(pTess->GetUTF8Text()).get() << std::endl;
#endif // OCR_DEBUG
	//

	char *res_data = NULL;
	res_data = pTess->GetUTF8Text();
	//pTess->GetUNLVText();
	//std::cout <<"strlen:"<< strlen(res_data) << std::endl;
	//std::cout << res_data << std::endl;
	int str_len = strlen(res_data);
	size_t max_lenth = std::min(str_len, 1024);
	results = std::string(res_data, max_lenth);

	if(res_data != NULL)
		delete[] res_data;
	
	return 1;
}

bool _isdigit(int c)
{
	return isdigit(c) || c == '-';
}


std::string OCRArbitaryTag::format_results(std::string res_str)
{
	int digt_len = 0;
	string postcode;
	bool start = true;
	for (int i= res_str.size()/2;i<res_str.size();i++)
	{
		unsigned char c = res_str.at(i);
		if (c == '.' || c == ',')
		{
			c = ' ';
		}
		if ((c == ' ' || c== '\n') && start == false)
		{
			start = true;
			continue;
		}
		if (start && _isdigit(c))
		{
			digt_len++;
			continue;
		}
		if ((c == ' ' || c == '\n') && start == true)
		{
			if (digt_len == 5 || digt_len == 10)
			{
				postcode = res_str.substr(i - digt_len, digt_len);
			}
			digt_len = 0;
			continue;
		}
		if (!_isdigit(c))
		{
			digt_len = 0;
			start = false;
		}
	}

	if (digt_len== 5 || digt_len==10)
	{
		postcode = res_str.substr(res_str.size() - digt_len, digt_len);
	}
	return postcode;
}

OCRHandWriteBox::OCRHandWriteBox()
{
	

	this->input_image_ = new std::array<float, MAX_IMAGE_NUM * WIDTH_ * HEIGHT_ * CHANNEL_>;

	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
	input_tensor_ = Ort::Value::CreateTensor<float>(memory_info, input_image_->data(), input_image_->size(), input_shape_.data(), input_shape_.size());
}

int OCRHandWriteBox::initial_model(const wchar_t* model_file, float thresh_conf, size_t cuda_id)
{
	//session_options.SetIntraOpNumThreads(1);
	// Available levels are
	// ORT_DISABLE_ALL -> To disable all optimizations
	// ORT_ENABLE_BASIC -> To enable basic optimizations (Such as redundant node removals)
	// ORT_ENABLE_EXTENDED -> To enable extended optimizations (Includes level 1 + more complex optimizations like node fusions)
	// ORT_ENABLE_ALL -> To Enable All possible opitmizations
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

#ifdef USE_CUDA_DEVICE
	OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, cuda_id);
#endif // USE_CUDA_DEVICE

	//Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "");

	try
	{
		m_pSession = new Ort::Session(env, model_file, session_options);
	}
	catch (const Ort::Exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	m_threshold_confidence = thresh_conf;

	return 1;
}

std::string OCRHandWriteBox::get_postcode_string(cv::Mat tag_mat)
{
	log_str = "";
	std::vector<cv::Mat> boxes;
	int res = get_handwrite_square_boxes(tag_mat, boxes);
	if (!res)
	{
		log_str += "do not get five square box,";
		return "";
	}
	for (int i=0;i<boxes.size();i++)
	{
		remove_box_border(boxes[i]);
	}
	return ocr_with_classifier(boxes);

}

std::string OCRHandWriteBox::get_last_log()
{
	return log_str;
}


int OCRHandWriteBox::find_key_words(cv::Mat tag_mat, std::vector<std::string>& key_words, void*pTess_/*=NULL*/)
{
	log_str = "";
	if (tag_mat.empty())
	{
		log_str += "tag mat is empty, ";
		return 0;
	}
	if (tag_mat.cols<40 || tag_mat.rows<40)
	{
		log_str += "tag mat size is too small, ";
		return 0;
	}

	cv::Mat roimat;
	tag_mat(cv::Rect(0, 0, tag_mat.cols / 4, tag_mat.rows / 2.3)).copyTo(roimat);

	if (roimat.channels()==3)
	{
		cv::cvtColor(roimat, roimat, cv::COLOR_BGR2GRAY);
	}
	float dst_width = 240;
	float scale = dst_width / roimat.cols;
	cv::resize(roimat, roimat, cv::Size(), scale, scale);
	cv::normalize(roimat, roimat, 255, 0, cv::NORM_MINMAX);

#ifdef DEBUG_HAND_WRITE_BOX
	cv::imshow("findkey range", roimat);
#endif // DEBUG_HAND_WRITE_BOX

	cv::Mat bnmat;
	cv::threshold(roimat, bnmat, 120, 255, cv::THRESH_BINARY);

	//判断类型，黑底白字，白底黑字
	cv::Scalar meanv = cv::mean(bnmat);
	//std::cout << "mean:" << meanv[0] << std::endl;
	if (meanv[0] < 90)
	{
		bnmat = ~bnmat;
		roimat = ~roimat;
	}

	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::morphologyEx(bnmat, bnmat, cv::MORPH_CLOSE, element);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(bnmat, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	
#ifdef DEBUG_HAND_WRITE_BOX
	cv::Mat drmat0 = bnmat.clone();
	for (int i = 0; i < contours.size(); i++)
	{
		cv::drawContours(drmat0, contours, i, cv::Scalar(125));
		cv::putText(drmat0, std::to_string(i), contours[i][0], 1, 1, cv::Scalar(255));
	}
	cv::imshow("hwrt_type_contours", drmat0);
#endif


	float bnmat_area = bnmat.cols*bnmat.rows;
	int ind_contour = -1;
	cv::Rect dstrc;
	int pos_y = -1;
	for (int i=0;i<contours.size();i++)
	{
		if (contours[i].size()<4) continue;
		cv::Rect rc;
		ImageProcessFunc::getContourRect(contours[i], rc);
		double area = cv::contourArea(contours[i]);
		if (area/(rc.area()+0.01)>0.8 && area>bnmat_area*0.1 && rc.y > pos_y)
		{
			pos_y = rc.y;
			//std::cout<<"contour:"<<std::to_string(i) << ", area ratio:" << area / (dstrc.area() + 0.01) << ", area:" << area << std::endl;
			dstrc = rc;
			ind_contour = i;
		}
	}
	if (ind_contour ==-1)
	{
		log_str += "can not locate type range,";
		return 0;
	}

	cv::Mat mask(roimat.rows,roimat.cols, roimat.type(),cv::Scalar(0,0,0));
	cv::drawContours(mask, contours, ind_contour,cv::Scalar(255,255,255), -1);
	mask = mask(dstrc);

	//cv::imshow("mask", mask);

	cv::Mat squaremat(dstrc.height, dstrc.width, roimat.type(), cv::Scalar(255));
	cv::copyTo(roimat(dstrc), squaremat, mask);

	cv::normalize(squaremat, squaremat, 255, 0, cv::NORM_MINMAX);
	meanv = cv::mean(squaremat);
	float anchor = meanv[0] - 50;
	anchor = std::max(anchor, 50.0f);
	ImageProcessFunc::adJustBrightness(squaremat, 2, 1, anchor);

#ifdef DEBUG_HAND_WRITE_BOX
	cv::imshow("box type bnmat", squaremat);
#endif // DEBUG_HAND_WRITE_BOX

	cv::Mat ocrmat;
	squaremat.copyTo(ocrmat);


	tesseract::TessBaseAPI* pTess = (tesseract::TessBaseAPI*)pTess_;
	if (pTess == NULL)
	{
		log_str += "tesseract pointer is Null,";
		return 0;
	}
	pTess->SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_LINE);
	//pTess->SetVariable("save_best_choices", "T");
	pTess->SetImage(ocrmat.data, ocrmat.cols , ocrmat.rows, ocrmat.channels(), ocrmat.step1());
	pTess->SetVariable("user_defined_api", "72");
	pTess->Recognize(0);

	// get result and delete[] returned char* string
#ifdef DEBUG_HAND_WRITE_BOX
	std::cout << std::unique_ptr<char[]>(pTess->GetUTF8Text()).get() << std::endl;
#endif // OCR_DEBUG
	//
	char *res_data = NULL;
	res_data = pTess->GetUTF8Text();

	//std::cout <<"strlen:"<< strlen(res_data) << std::endl;
	//std::cout << res_data << std::endl;
	int str_len = strlen(res_data);
	size_t max_lenth = std::min(str_len, 32);
	std::string results = std::string(res_data, max_lenth);

	if (res_data != NULL)
		delete[] res_data;

	std::transform(results.begin(), results.end(), results.begin(), ::tolower);

	for (int i=0;i<key_words.size();i++)
	{
		size_t res = results.find(key_words[i]);
		if (res!=std::string::npos)
		{
			return 1;
		}
	}
	log_str += "can not find keywords";
	return 0;
}
int OCRHandWriteBox::getPostCodeLine_nobox(const cv::Mat &srcMat, std::vector<cv::Mat> &toMats, std::vector<cv::Mat> &fromMats)
{
	if (srcMat.empty()) return 0;

	Mat grymat;

	float scal_max2 = 1000.0 / max(srcMat.cols, srcMat.rows);

	cv::resize(srcMat, grymat, cv::Size(), scal_max2, scal_max2);

	if (grymat.channels() == 3) cvtColor(grymat, grymat, COLOR_BGR2GRAY);


	if (grymat.rows < 200 || grymat.cols < 200) return 0;


	//Rect cr(bord_wd, 0, grymat.cols - 2 * bord_wd, grymat.rows);
	Rect cr_sample(grymat.cols*0.2, grymat.rows*0.1, grymat.cols*0.6, grymat.rows*0.4);

	//imshow("srcm2", centm);
	double avg_pix = ImageProcessFunc::getAveragePixelInRect(grymat, cr_sample);

	Mat bnmat;
	cv::threshold(grymat, bnmat, avg_pix / 1.5, 255, cv::THRESH_BINARY);
	bnmat = ~bnmat;

#ifdef POSTCODE_BOX_DEBUG
	imshow("bnmat", bnmat);
#endif // POSTCODE_BOX_DEBUG


	cv::Mat element;
	element = getStructuringElement(MORPH_RECT, Size(5, 20));
	morphologyEx(bnmat, bnmat, MORPH_CLOSE, element);
	element = getStructuringElement(MORPH_RECT, Size(50, 5));
	morphologyEx(bnmat, bnmat, MORPH_CLOSE, element);

	element = getStructuringElement(MORPH_RECT, Size(1, 5));
	morphologyEx(bnmat, bnmat, MORPH_OPEN, element);


	element = getStructuringElement(MORPH_RECT, Size(3, 3));
	morphologyEx(bnmat, bnmat, MORPH_ERODE, element);




#ifdef POSTCODE_BOX_DEBUG
	imshow("morpmat", bnmat);
#endif // POSTCODE_BOX_DEBUG

	//裁切掉黑边
	vector<unsigned int> sum_pixels;
	ImageProcessFunc::sumPixels(bnmat, 0, sum_pixels);
	int iw = bnmat.cols;
	int cut_line_up = 0;
	for (int i = 0; i < sum_pixels.size(); i++)
	{
		int avpix = sum_pixels[i] / iw;
		if (avpix >= 127)
		{
			cut_line_up++;
		}
		else
		{
			break;
		}
	}
	int cut_line_down = 0;
	for (int i = sum_pixels.size() - 1; i >= 0; i--)
	{
		int avpix = sum_pixels[i] / iw;
		if (avpix >= 120)
		{
			cut_line_down++;
		}
		else
		{
			break;
		}
	}
	if ((cut_line_down + cut_line_up) > (bnmat.rows / 2))
	{
		cout << "裁切上下异常" << endl;
		return 0;
	}
	Rect rc_tb_cut(0, 0, bnmat.cols, bnmat.rows);
	if (cut_line_down != 0 || cut_line_up != 0)
	{
		rc_tb_cut.y = cut_line_up;
		rc_tb_cut.height = rc_tb_cut.height - (cut_line_up + cut_line_down);
	}
	bnmat = bnmat(rc_tb_cut);

	vector<unsigned int>().swap(sum_pixels);
	ImageProcessFunc::sumPixels(bnmat, 1, sum_pixels);
	int ih = bnmat.rows;
	int cut_line_left = 0;
	for (int i = 0; i < sum_pixels.size(); i++)
	{
		int avpix = sum_pixels[i] / ih;
		if (avpix >= 127)
		{
			cut_line_left++;
		}
		else
		{
			break;
		}
	}
	int cut_line_right = 0;
	for (int i = sum_pixels.size() - 1; i >= 0; i--)
	{
		int avpix = sum_pixels[i] / ih;
		if (avpix >= 127)
		{
			cut_line_right++;
		}
		else
		{
			break;
		}
	}
	if ((cut_line_left + cut_line_right) > (bnmat.cols / 2))
	{
		cout << "裁切左右异常" << endl;
		return 0;
	}
	Rect rc_lr_cut(0, 0, bnmat.cols, bnmat.rows);
	if (cut_line_down != 0 || cut_line_up != 0)
	{
		rc_lr_cut.x = cut_line_left;
		rc_lr_cut.width = rc_lr_cut.width - (cut_line_left + cut_line_right);
	}
	bnmat = bnmat(rc_lr_cut);


#ifdef POSTCODE_BOX_DEBUG
	imshow("cutmargin", bnmat);
#endif // POSTCODE_BOX_DEBUG
	////imshow("cut_tb", bnmat);

	//获得轮廓的rect
	std::vector<std::vector<cv::Point>>contours;
	std::vector<cv::Vec4i> hierarchy;
	std::vector<cv::Point> contour;
	double aera = 0;
	//src_gray = src_gray > 100;
	cv::findContours(bnmat, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);
	vector<float> contours_score;
	vector<Rect> contours_rect;
	if (contours.size() == 0)
	{
		return 0;
	}
	for (int i = 0; i < contours.size(); i++)
	{
		Rect _rc;
		ImageProcessFunc::getContourRect(contours[i], _rc);
		double maera = cv::contourArea(contours[i]);
		if (_rc.width*_rc.height * 0.3 > maera)//轮廓面积只有rect面积的1/2，弃掉
		{
			continue;
		}
		//rectangle(bnmat, _rc, Scalar(125, 125, 125));
		contours_rect.push_back(_rc);
	}
	score_for_rect(contours_rect, bnmat.cols, bnmat.rows, contours_score);

	//查找评分大于阈值的rect
	float score_threshold = 0.3;//评分阈值
	vector<cv::Rect> candidate_rects;
	for (int i = 0; i < contours_score.size(); i++)
	{
		if (contours_score[i] < score_threshold)
		{
			continue;
		}
		candidate_rects.push_back(contours_rect[i]);
	}

	if (candidate_rects.empty())
	{
		cout << "没有找到合适的候选框" << endl;
		return 0;
	}

	//拆分rect
	vector<cv::Rect> candidate_lt_rects; //左上角
	vector<cv::Rect> candidate_rb_rects; //右下角
	for (int i = 0; i < candidate_rects.size(); i++)
	{
		if ((candidate_rects[i].x + candidate_rects[i].width / 2) < bnmat.cols / 2
			|| (candidate_rects[i].y + candidate_rects[i].height / 2) < bnmat.rows / 2)
		{
			candidate_lt_rects.push_back(candidate_rects[i]);
		}
		else
		{
			candidate_rb_rects.push_back(candidate_rects[i]);
		}
	}

	rc_lr_cut.x /= scal_max2;
	rc_lr_cut.y /= scal_max2;
	rc_lr_cut.height /= scal_max2;
	rc_lr_cut.width /= scal_max2;


	rc_tb_cut.x /= scal_max2;
	rc_tb_cut.y /= scal_max2;
	rc_tb_cut.width /= scal_max2;
	rc_tb_cut.height /= scal_max2;
	ImageProcessFunc::CropRect(cv::Rect(0, 0, srcMat.cols, srcMat.rows), rc_tb_cut);
	ImageProcessFunc::CropRect(cv::Rect(0, 0, srcMat(rc_tb_cut).cols, srcMat(rc_tb_cut).rows), rc_lr_cut);

	//获取候选的框
	vector<cv::Mat> fromMatVec;
	vector<cv::Mat> toMatVec;
	for (int i = 0; i < candidate_lt_rects.size(); i++)
	{
		cv::Rect refinedRect = candidate_lt_rects[i];
		refinedRect.x /= scal_max2;
		refinedRect.y /= scal_max2;
		refinedRect.width /= scal_max2;
		refinedRect.height /= scal_max2;
		bool needRotate = false;
		getHandWriteRange(srcMat(rc_tb_cut)(rc_lr_cut), refinedRect, refinedRect, needRotate);
		cv::Mat candi_mat = srcMat(rc_tb_cut)(rc_lr_cut)(refinedRect);
		if (needRotate)
		{
			cv::rotate(candi_mat, candi_mat, ROTATE_180);
			toMatVec.push_back(candi_mat);
		}
		else
		{
			candi_mat = candi_mat.clone();
			fromMatVec.push_back(candi_mat);
		}

	}

	for (int i = 0; i < candidate_rb_rects.size(); i++)
	{
		cv::Rect refinedRect = candidate_rb_rects[i];
		refinedRect.x /= scal_max2;
		refinedRect.y /= scal_max2;
		refinedRect.width /= scal_max2;
		refinedRect.height /= scal_max2;
		bool needRotate = false;
		getHandWriteRange(srcMat(rc_tb_cut)(rc_lr_cut), refinedRect, refinedRect, needRotate);
		cv::Mat candi_mat = srcMat(rc_tb_cut)(rc_lr_cut)(refinedRect);
		if (needRotate)
		{
			cv::rotate(candi_mat, candi_mat, ROTATE_180);
			fromMatVec.push_back(candi_mat);
		}
		else
		{
			candi_mat = candi_mat.clone();
			toMatVec.push_back(candi_mat);
		}

	}
	toMatVec.swap(toMats);
	fromMatVec.swap(fromMats);


	return 1;
}



int OCRHandWriteBox::getHandWriteRange(const cv::Mat &srcMat, cv::Rect &srcRect, cv::Rect &dstRect, bool &need_rotate)
{
	if (srcMat.empty()) return 0;

// 	if (srcMat.channels() == 3)
// 	{
// 		cv::cvtColor(srcMat, srcMat, COLOR_BGR2GRAY);
// 	}


	Rect best_rect1_refine = srcRect;
	//Rect best_rect2_refine = contours_rect[best_ind2];

	float zoom_scal_x = 1.5;
	float zoom_scal_y = 2.0;

	best_rect1_refine.width = best_rect1_refine.width*zoom_scal_x;
	best_rect1_refine.height = best_rect1_refine.height*zoom_scal_y;
	best_rect1_refine.x = best_rect1_refine.x - srcRect.width*(zoom_scal_x - 1) / 2;
	best_rect1_refine.y = best_rect1_refine.y - srcRect.height*(zoom_scal_y - 1) / 2;


	int res = ImageProcessFunc::CropRect(Rect(0, 0, srcMat.cols, srcMat.rows), best_rect1_refine);
	if (res == 0) return 0;

	dstRect = best_rect1_refine;

	//检查是否旋转
	int top2rect1 = best_rect1_refine.y;
	int left2rect1 = best_rect1_refine.x;
	int bottom2rect1 = srcMat.rows - (top2rect1 + best_rect1_refine.height);
	int right2rect1 = srcMat.cols - (left2rect1 + best_rect1_refine.width);

	if (top2rect1 < bottom2rect1)
	{
		if (left2rect1 < srcMat.cols / 8)
		{
			need_rotate = true;
		}
		else
		{
			need_rotate = false;
		}
	}
	else
	{
		if (right2rect1 > srcMat.cols / 8)
		{
			need_rotate = true;
		}
		else
		{
			need_rotate = false;
		}
	}

	return 1;

}
int OCRHandWriteBox::score_for_rect(std::vector<cv::Rect> rcs, int im_width, int im_height, std::vector<float> &rc_scores)
{
	const int ref_width = 200;
	const float ref_ratio = 7;
	const int ref_height_up = 35;
	const int ref_height_down = 10;

	for (int i = 0; i < rcs.size(); i++)
	{
		float score_ = 1;
		int c_x = rcs[i].x + rcs[i].width / 2;
		int c_y = rcs[i].y + rcs[i].height / 2;
		if ((c_x > im_width / 2 && c_y < im_height / 2) || (c_x<im_width / 2 && c_y>im_height / 2))
		{
			rc_scores.push_back(0);
			continue;
		}
		if (rcs[i].height > ref_height_up || rcs[i].height < ref_height_down)
		{
			rc_scores.push_back(0);
			continue;
		}
		float _ratio = float(rcs[i].width) / rcs[i].height;
		float score_tmp = 1 - fabs(_ratio - ref_ratio) / ref_ratio; //长宽比打分
		score_tmp = (score_tmp < 0) ? 0 : score_tmp;
		score_ *= score_tmp;

		rc_scores.push_back(score_);


	}

	return 1;
}
int OCRHandWriteBox::split_digits_nobox(cv::Mat &srcMat, std::vector<cv::Mat> &dstDigits)
{
	if (srcMat.empty())
	{
		return 0;
	}
	if (srcMat.channels() == 3)
	{
		cvtColor(srcMat, srcMat, COLOR_BGR2GRAY);
	}
	cv::normalize(srcMat, srcMat, 255, 0, cv::NORM_MINMAX);
	float avg_pix = ImageProcessFunc::getAverageBrightness(srcMat);
	Mat adj_mat = srcMat.clone();
	ImageProcessFunc::adJustBrightness(adj_mat, 10, 0, avg_pix / 2.0);
	adj_mat = ~adj_mat;

#ifdef DEBUG_HAND_WRITE_BOX
	imshow("digits", adj_mat);
#endif // POSTCODE_BOX_DEBUG

	vector<unsigned int> sum_pixs;
	ImageProcessFunc::sumPixels(adj_mat, 1, sum_pixs);

	int ih = adj_mat.rows;
	vector<Point> seg_points_x;
	bool start_flag = false;
	Point pt;
	for (size_t i = 0; i < sum_pixs.size(); i++)
	{
		int _p = sum_pixs[i] / ih;
		if (_p > 3 && start_flag == false)
		{
			pt.x = i;
			start_flag = true;
		}
		if (_p <= 3 && start_flag == true)
		{
			pt.y = i;
			seg_points_x.push_back(pt);
			start_flag = false;
		}
	}

	vector<Rect> seg_rects;
	for (size_t i = 0; i < seg_points_x.size(); i++)
	{
		Rect _rc(seg_points_x[i].x, 0, seg_points_x[i].y - seg_points_x[i].x, adj_mat.rows);
		int iw = _rc.width;
		vector<unsigned int> sum_pixs;
		ImageProcessFunc::sumPixels(adj_mat(_rc), 0, sum_pixs);
		Point pt;
		for (size_t j = 0; j < sum_pixs.size(); j++)
		{
			int _p = sum_pixs[j] / iw;
			if (_p > 10)
			{
				pt.x = j;
				break;
			}
		}
		for (size_t j = sum_pixs.size() - 1; j >= 0; j--)
		{
			int _p = sum_pixs[j] / iw;
			if (_p > 10)
			{
				pt.y = j;
				break;
			}
		}
		if (pt.x == pt.y)
		{
			cout << "分割数字失败" << endl;
			return 0;
		}
		_rc.y = pt.x;
		_rc.height = pt.y - pt.x + 1;
		seg_rects.push_back(_rc);
	}

	//过滤rect
	if (seg_rects.size() < 5)
	{
		cout << "分割数字失败" << endl;
		return 0;
	}
	if (seg_rects.size() > 5)
	{
		vector<Rect>::iterator it = seg_rects.begin();
		int area_threshold = 50;
		for (; it != seg_rects.end();)
		{
			if (it->area() < area_threshold)
			{
				it = seg_rects.erase(it);
			}
			else
			{
				it++;
			}
		}
	}
#ifdef DEBUG_HAND_WRITE_BOX
	Mat mshow = adj_mat.clone();
	for (size_t i = 0; i < seg_rects.size(); i++)
	{
		rectangle(mshow, seg_rects[i], Scalar(150, 150, 150));
	}
	imshow("adj_mat", mshow);
#endif // POSTCODE_BOX_DEBUG


	if (seg_rects.size() != 5)
	{
		cout << "分割数字失败" << endl;
		return 0;
	}

	//调整rect尺寸
	for (size_t i = 0; i < seg_rects.size(); i++)
	{
		Rect _rc = seg_rects[i];
		int bd = 3;
		_rc.x -= bd;
		_rc.y -= bd;
		_rc.width += 2 * bd;
		_rc.height += 2 * bd;
		if (_rc.width < _rc.height)
		{
			int cent_x = _rc.x + _rc.width / 2;
			_rc.width = _rc.height;
			_rc.x = cent_x - _rc.width / 2;
		}
		if (_rc.width > _rc.height)
		{
			int cent_y = _rc.y + _rc.height / 2;
			_rc.height = _rc.width;
			_rc.y = cent_y - _rc.height / 2;
		}
		ImageProcessFunc::CropRect(Rect(0, 0, adj_mat.cols, adj_mat.rows), _rc);
		seg_rects[i] = _rc;
	}
	Mat cropMat;
	srcMat.copyTo(cropMat);
	ImageProcessFunc::adJustBrightness(cropMat, 10, 0, avg_pix / 1.4);
	//cropMat = ~cropMat;


	for (size_t i = 0; i < seg_rects.size(); i++)
	{
		Mat _m = cropMat(seg_rects[i]);
		cv::resize(_m, _m, cv::Size(32, 32));
		dstDigits.push_back(_m.clone());
	}
	return 1;
}


int OCRHandWriteBox::getPostCode_nobox(std::vector<cv::Mat> &srcMat_vec, std::vector<std::string> &result_str, std::vector<float> &confidence)
{
	if (srcMat_vec.size() == 0) return 0;

#ifdef DEBUG_HAND_WRITE_BOX
	cv::Mat showMat(cv::Size(srcMat_vec.size() * 32, 32), CV_8UC1);
	for (int i = 0; i < srcMat_vec.size(); i++)
	{
		cv::Rect r(i * 32, 0, 32, 32);
		srcMat_vec[i].copyTo(showMat(r));
	}
	imshow("post_code_boxes", showMat);
#endif // POSTCODE_BOX_DEBUG

	int postcodeline_num = srcMat_vec.size() / 5;


	for (int i = 0; i < postcodeline_num; i++)
	{
		std::vector<cv::Mat> m_vec(srcMat_vec.begin() + i*5, srcMat_vec.begin() + (i+1)*5);
		std::string res_str;
		float confid = 0;
		res_str = ocr_with_classifier(m_vec, &confid);

		result_str.push_back(res_str);
		confidence.push_back(confid);

	}

	return postcodeline_num;

}
bool sort_func_vertical(cv::Rect &a, cv::Rect &b)
{
	return (a.tl().y + a.br().y) > (b.tl().y + b.br().y);
}
bool sort_func_horizon(cv::Rect &a, cv::Rect &b)
{
	return (a.tl().x + a.br().x) < (b.tl().x + b.br().x);
}

std::string OCRHandWriteBox::get_postcode_string_test_v2(const cv::Mat &tag_mat)
{
	log_str = "";
	if (tag_mat.empty())
	{
		log_str += "tag mat is empty, ";
		return "";
	}

	float scal = 640.0f / tag_mat.cols;
	cv::Mat rsmat;
	cv::resize(tag_mat, rsmat, cv::Size(), scal, scal);
	if (rsmat.channels()==3)
	{
		cv::cvtColor(rsmat, rsmat, cv::COLOR_BGR2GRAY);
	}
	cv::Rect digit_rc(rsmat.cols*0.4, rsmat.rows*0.75, rsmat.cols*0.6, (rsmat.rows - rsmat.rows*0.75));
	rsmat = rsmat(digit_rc).clone();

	

 	cv::Scalar meanv = cv::mean(rsmat);
 	ImageProcessFunc::adJustBrightness(rsmat, 3, 1, meanv[0]*0.55);
	cv::Mat graymat;
	rsmat.copyTo(graymat);

	meanv = cv::mean(rsmat);
 	cv::threshold(rsmat, rsmat, meanv[0]*0.45, 255, cv::THRESH_BINARY);
 	rsmat = ~rsmat;
// 
	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::morphologyEx(rsmat, rsmat, cv::MORPH_DILATE, element);
	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 9));
	cv::morphologyEx(rsmat, rsmat, cv::MORPH_CLOSE, element);

#ifdef DEBUG_HAND_WRITE_BOX
	cv::imshow("nobox_postcode_line", rsmat);
#endif // DEBUG_HAND_WRITE_BOX


	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(rsmat, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	if (contours.size() < 5)
	{
		log_str += "total contours num lower than 5, ";
		return "";
	}
	std::vector<cv::Rect> cand_rects;
	for (int i=0;i<contours.size();i++)
	{
		cv::Rect rc;
		ImageProcessFunc::getContourRect(contours[i],rc);
		if (rc.height < rsmat.rows*0.12)
		{
			continue;
		}
		if (float(rc.width)/rc.height>2)
		{
			continue;
		}
		cand_rects.push_back(rc);
	}

	if (cand_rects.size() < 5)
	{
		log_str += "valid contours num lower than 5, ";
		return "";
	}
	if (cand_rects.size() > 5)//选取底部的5个
	{
		std::sort(cand_rects.begin(), cand_rects.end(), sort_func_vertical);
		std::vector<cv::Rect> _cand_rects(cand_rects.begin(), cand_rects.begin() + 5);
		cand_rects.swap(_cand_rects);
	}
	std::sort(cand_rects.begin(), cand_rects.end(), sort_func_horizon); //水平排列

	//检查水平间距
	float min_v = 1000;
	float max_v = 0;
	for (int i = 0; i < cand_rects.size() - 1; i++)
	{
		float wr = ((cand_rects[i + 1].br() + cand_rects[i + 1].tl()) / 2 - \
			(cand_rects[i].br() + cand_rects[i].tl()) / 2).x;

		if (wr < min_v) min_v = wr;
		if (wr > max_v) max_v = wr;
	}
	if ((max_v - min_v) / (max_v + min_v + 0.0001) > 0.3)
	{
		log_str += "digits horizontal distance exception";
		return "";
	}
	//cv::threshold(graymat, graymat, meanv[0] * 0.65, 255, cv::THRESH_BINARY);
#ifdef DEBUG_HAND_WRITE_BOX
	cv::imshow("nobox_postcode_img", graymat);
#endif
	//


	cv::normalize(graymat, graymat, 255, 0, cv::NORM_MINMAX);
	meanv = cv::mean(graymat);
	ImageProcessFunc::adJustBrightness(graymat, 3, 1, meanv[0] * 0.45);



	std::vector<cv::Mat> digits_mats;
	for (int i=0;i<cand_rects.size();i++)
	{
		int size_ = std::max(cand_rects[i].width, cand_rects[i].height);
		size_ += 10;
		cv::Point cent = (cand_rects[i].br() + cand_rects[i].tl()) / 2;
		cv::Rect rc;
		rc.x = cent.x - size_ / 2;
		rc.y = cent.y - size_ / 2;
		rc.width = size_;
		rc.height = size_;
		ImageProcessFunc::CropRect(cv::Rect(0, 0, rsmat.cols, rsmat.rows), rc);
#ifdef DEBUG_HAND_WRITE_BOX
		cv::imshow(std::to_string(i), graymat(rc));
#endif // DEBUG_HAND_WRITE_BOX

		digits_mats.push_back(graymat(rc));
	}

	std::string postcode = ocr_with_classifier(digits_mats);
	return postcode;

}

int OCRHandWriteBox::identify_handbox_type(const cv::Mat &srcm)
{

	if (srcm.empty())
	{
		return 0;
	}
	cv::Mat tmat;
	srcm(cv::Rect(0, 0, srcm.cols, srcm.rows / 2)).copyTo(tmat);
	int dstwidth = 480;
	float scal = float(dstwidth) / tmat.cols;
	cv::resize(tmat, tmat, cv::Size(), scal, scal);
	cv::medianBlur(tmat, tmat, 3);

	cv::Scalar meanv;
	cv::Scalar stdv;

	cv::meanStdDev(tmat, meanv, stdv);
// 	std::cout << "mean:" << meanv[0] << std::endl;
// 	std::cout << "stdv:" << stdv[0] << std::endl;

	if (stdv[0] < 22.5f)
	{
		return 1;
	}
	else
	{
		return 2;
	}

	return 1;
}

std::string OCRHandWriteBox::get_postcode_string_test(cv::Mat srcMat)
{
	if (srcMat.empty())
	{
		return "";
	}
	//Mat fromMat, toMat;
	if (srcMat.rows > srcMat.cols) cv::rotate(srcMat, srcMat, ROTATE_90_CLOCKWISE);//旋转图片
	vector<Mat> toMats, fromMats;
	int res = getPostCodeLine_nobox(srcMat, toMats, fromMats);
	if (res == 0)
	{
		return "";
	}
#ifdef POSTCODE_BOX_DEBUG
	for (int i = 0; i < fromMats.size(); i++)
	{
		imshow("FromMat" + to_string(i), fromMats[i]);
	}
	for (int i = 0; i < toMats.size(); i++)
	{
		imshow("ToMat" + to_string(i), toMats[i]);
	}

#endif // POSTCODE_BOX_DEBUG

	vector<Mat> to_digits_vec;
	for (int i = 0; i < toMats.size(); i++)
	{
		res = split_digits_nobox(toMats[i], to_digits_vec);
		if (res == 0)
		{
			continue;
		}
	}
	if (to_digits_vec.empty())
	{
		cout << "未找到目的地邮编！" << endl;
		return "";
	}
	vector<string> to_code_vec;
	vector<float> to_confidence_vec;

	getPostCode_nobox(to_digits_vec, to_code_vec, to_confidence_vec);

	
	int code_index = -1;
	float tem_confidence = 0;
	for (int i = 0; i < to_confidence_vec.size(); i++)
	{
		if (tem_confidence < to_confidence_vec[i] && to_confidence_vec[i] >= m_threshold_confidence)
		{
			code_index = i;
			tem_confidence = to_confidence_vec[i];
		}
	}
	string to_postcode, from_postcode;
	if (code_index != -1)
	{
		to_postcode = to_code_vec[code_index];
	}
	else
	{
		cout << "目的地邮编置信度较低" << endl;
		return "";
	}

	return to_postcode;
}

bool sort_contours_func(std::vector<cv::Point> &a, std::vector<cv::Point> &b)
{
	cv::Point ct1 = (a[0] + a[a.size() / 3] + a[a.size() * 2 / 3]) / 3;
	cv::Point ct2 = (b[0] + b[b.size() / 3] + b[b.size() * 2 / 3]) / 3;
	return ct1.x < ct2.x;
}



int OCRHandWriteBox::get_handwrite_square_boxes(cv::Mat &srcm, std::vector<cv::Mat> &square_boxes)
{
	if (srcm.empty())
	{
		log_str += "src image is empty,";
		return 0;
	}
	if (srcm.rows > srcm.cols)
	{
		log_str += "image height is large than width,";
		return 0;
	}
	if (srcm.cols < 64 || srcm.rows < 64)
	{
		log_str += "image size is to small, ";
		return 0;
	}

	int hdwrt_hight = srcm.rows * 0.25;
	cv::Rect rng_rct(0, srcm.rows - hdwrt_hight - 1, srcm.cols, hdwrt_hight);
	cv::Mat rng_mat;
	srcm(rng_rct).copyTo(rng_mat);

	if (srcm.channels() == 3)
	{
		cv::cvtColor(rng_mat, rng_mat, COLOR_BGR2GRAY);
	}


	
	int dst_width = 720;
	float scal_ = float(dst_width)/ rng_mat.cols;
	cv::Mat rzmat;
	cv::resize(rng_mat, rzmat, cv::Size(), scal_, scal_,cv::INTER_LINEAR);
	//cv::imshow("s", rzmat);

	cv::normalize(rzmat, rzmat, 255, 0, cv::NORM_MINMAX);
	//cv::equalizeHist(rzmat, rzmat);

#ifdef DEBUG_HAND_WRITE_BOX
	cv::imshow("handbox_bn", rzmat);
#endif // DEBUG_HAND_WRITE_BOX
	
	
	cv::Scalar mean_pxl = cv::mean(rzmat);

	cv::Mat bnmat;
	cv::threshold(rzmat, bnmat, mean_pxl[0] - 25, 255, cv::THRESH_BINARY_INV);
	//cv::imshow("bn", bnmat);


#ifdef DEBUG_HAND_WRITE_BOX
	cv::imshow("hwrt box bnmat", bnmat);
#endif

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(bnmat, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);


	//过滤轮廓-宽高比，尺寸过滤
	//std::vector<cv::Point> tl_pt;
	std::vector<std::vector<cv::Point>>::iterator it;
	for (it=contours.begin();it!=contours.end();)
	{
		cv::Rect rc;
		int res = ImageProcessFunc::getContourRect(*it, rc);
		if (float(rc.width)/rc.height < 0.75 || float(rc.width) / rc.height > 1.25 \
			|| rc.width < bnmat.cols*0.05 || rc.width > bnmat.cols*0.1)
		{
			it = contours.erase(it);
		}
		else
		{
			//tl_pt.push_back(rc.tl());
			it++;
		}
	}


#ifdef DEBUG_HAND_WRITE_BOX
	cv::Mat drmat0 = rzmat.clone();
	for (int i = 0; i < contours.size(); i++)
	{
		cv::drawContours(drmat0, contours, i, cv::Scalar(125));
		cv::putText(drmat0, std::to_string(i), contours[i][0], 1, 1, cv::Scalar(255));
	}
	cv::imshow("hwrt_boxes_contours", drmat0);
#endif

	if (contours.size() < 5)
	{
		log_str += "square box num is lower than 5,";
		return 0;
	}
	if (contours.size() > 5)
	{
		for (it = contours.begin() + 5; it != contours.end();) //保留底部的五个轮廓
		{
			it = contours.erase(it);
		}
	}

	//面积对比过滤
	cv::Rect arc;
	ImageProcessFunc::getContourRect(contours[0], arc);
	double area = arc.area() + 1e-6;
	for (int i=1;i<contours.size();i++)
	{
		ImageProcessFunc::getContourRect(contours[i], arc);
		double area_rate = arc.area() / area;
		if (area_rate<0.7 || area_rate > 1.3)
		{
			log_str += "square area is exception,";
			return 0;
		}
		cv::Rect rc;
	}

	std::sort(contours.begin(), contours.end(), sort_contours_func);

	//距离过滤
	std::vector<cv::Rect> outrects;
	for (int i=0;i<contours.size();i++)
	{
		cv::Rect rc;
		ImageProcessFunc::getContourRect(contours[i], rc);
		outrects.push_back(rc);
	}
	
	float min_v=1000;
	float max_v=0;
	for (int i=0;i<outrects.size()-1;i++)
	{
		float wr = outrects[i + 1].x - outrects[i].x;
		if (wr < min_v) min_v = wr;
		if (wr > max_v) max_v = wr;
	}
	//auto minmax = std::minmax(dist_vec.begin(), dist_vec.end());

	if ((max_v - min_v) / (max_v + min_v + 0.0001) > 0.3)
	{
		log_str += "square distance exception,";
		return 0;
	}

#ifdef DEBUG_HAND_WRITE_BOX
	cv::Mat drmat = rzmat.clone();
	for (int i = 0; i < contours.size(); i++)
	{
		cv::drawContours(drmat, contours, i, cv::Scalar(125));
		cv::putText(drmat, std::to_string(i), contours[i][0], 1, 1, cv::Scalar(255));
	}
	cv::imshow("hwrt_boxes", drmat);
#endif
	
	

	for (int i=0;i<5;i++)
	{

		cv::Mat box = cv::Mat(outrects[i].size(), rzmat.type(),cv::Scalar(45,45,45));
		cv::Mat maskmat = cv::Mat::zeros(outrects[i].size(), rzmat.type());
		for (int j=0;j<contours[i].size();j++)
		{
			contours[i][j] -= outrects[i].tl();
		}
		cv::drawContours(maskmat, contours, i, cv::Scalar(255, 255, 255), -1);

		rzmat(outrects[i]).copyTo(box, maskmat);
		square_boxes.push_back(box);

 		//cv::imshow(std::to_string(i), box);
// 		cv::waitKey(0);
	}
	return 1;
}

int OCRHandWriteBox::remove_box_border(cv::Mat &src_mat)
{

	cv::medianBlur(src_mat, src_mat, 3);
	cv::normalize(src_mat, src_mat, 0, 255, cv::NORM_MINMAX, CV_8U);
	//src_mat.convertTo(src_mat, CV_8U);
// 	double minv, maxv;
// 	cv::minMaxIdx(src_mat, &minv, &maxv);

	//cv::imshow("rm_src", src_mat);
	cv::Mat bnmat;
	cv::threshold(src_mat, bnmat, 125, 255, cv::THRESH_BINARY);
	Mat element = cv::getStructuringElement(cv::MORPH_RECT, Size(15, 15));
	cv::morphologyEx(bnmat, bnmat, cv::MORPH_CLOSE, element , cv::Point(-1, -1),1,0,0);
	element = cv::getStructuringElement(cv::MORPH_RECT, Size(3, 3));
	cv::morphologyEx(bnmat, bnmat, cv::MORPH_ERODE, element);

	cv::Mat dstm = cv::Mat(bnmat.size(), src_mat.type(), cv::Scalar(200, 200, 200));

	src_mat.copyTo(dstm, bnmat);

	ImageProcessFunc::adJustBrightness(dstm, 2, 0, 75);
	
	src_mat = dstm;

	return 1;
}

std::string OCRHandWriteBox::ocr_with_classifier(std::vector<cv::Mat> &srcms, float *pConf)
{
	//导入图像数据
	if (srcms.size()!= MAX_IMAGE_NUM)
	{
		log_str += std::string("input image num ") + std::to_string(srcms.size()) + \
			" not equal to model defined " + std::to_string(MAX_IMAGE_NUM);
		return "";
	}
	for (int i =0;i<MAX_IMAGE_NUM;i++)
	{
		if (CHANNEL_!=srcms[i].channels())
		{
			cv::cvtColor(srcms[i], srcms[i], cv::COLOR_BGR2GRAY);//灰度单通道
		}
		cv::resize(srcms[i], srcms[i], cv::Size(WIDTH_, HEIGHT_));
		srcms[i] = 255 - srcms[i];//白底转黑底
		double minv, maxv;

		//cv::imshow(std::to_string(i), srcms[i]);

		cv::minMaxIdx(srcms[i], &minv, &maxv);
		if (maxv < 100)
		{
			log_str += "one digit in square is empty,";
			return ""; //空
		}
		
		cv::Mat m;
		srcms[i].convertTo(m, CV_32FC1);//灰度单通道

		size_t st_pos_img = i * CHANNEL_*HEIGHT_*WIDTH_;
		memcpy((float*)input_image_ + st_pos_img, m.data, sizeof(float)*WIDTH_*HEIGHT_);
	}

	std::vector<DigitClassRes> pred_res;
	std::string res_str;
	float min_conf = 1;
	int low_conf_ind = 0;
	//运行模型
	try
	{
		auto output_tensors = m_pSession->Run(Ort::RunOptions{ nullptr }, input_node_names.data(), &input_tensor_, 1, output_node_names.data(), 1);
		if (output_tensors.size()==0) 
		{
			log_str += "onnxruntime return none,";
			return "";
		}
		auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape(); // batch x out_feats
		float *pData = output_tensors[0].GetTensorMutableData<float>();
		int batch_size = output_shape[0];
		int out_feats = output_shape[1];
		
		for (int i=0;i< batch_size;i++)
		{
			float* pos = pData + i * out_feats;
			int class_ind = 0;
			float confidence = 0;
			for (int j=0;j<out_feats;j++)
			{
				if (*(pos+j) > confidence)
				{
					confidence = *(pos+j);
					class_ind = j;
				}
			}
			
			if (confidence < min_conf)
			{
				min_conf = confidence;
				low_conf_ind = i;
			}
			res_str += std::to_string(class_ind);
		}
	}
	catch (const Ort::Exception& e)
	{
		log_str += std::string("digits classifier exception:") + e.what();
		return "";
	}
	log_str += "confidence:" + std::to_string(min_conf) + ",";

	if (min_conf < m_threshold_confidence)
	{
		log_str += "the " + to_string(low_conf_ind+1) +"rd square confidence is low, possible result:" + res_str;
		return "";
	}
	if (pConf!=NULL)
	{
		*pConf = min_conf;
	}
	return res_str;
}

std::string OCRHandWriteBox::ocr_with_tesseract(std::vector<cv::Mat> &srcms)
{
	if (srcms.empty()) return "";
	int width = 0;
	int height = 0;
	for (int i=0;i<srcms.size();i++)
	{
		width += srcms[i].cols;
		height = (height < srcms[i].rows) ? srcms[i].rows : height;
	}
	if (width < 5 || height < 5) return "";
	cv::Mat dm = cv::Mat(height, width, srcms[0].type(), cv::Scalar(255, 255, 255));
	int pos = 0;
	for (int i=0;i<srcms.size();i++)
	{
		cv::Rect rc(pos, (height - srcms[i].rows) / 2, srcms[i].cols, srcms[i].rows);
		srcms[i].copyTo(dm(rc));
		pos += srcms[i].cols;
	}
	//cv::imshow("dmats", dm);

	return "";
}



OCRLotteryTag::OCRLotteryTag()
{

}

std::string OCRLotteryTag::get_postcode_string(cv::Mat tag_mat, OcrAlgorithm_config *pConfig)
{
	cv::Mat postcode_line;
	int res = get_postcode_line(tag_mat, postcode_line);
	std::string ocr_res;
	if (res==1 && !postcode_line.empty())
	{
		res = _run_ocr(postcode_line, ocr_res, pConfig);
		if (!ocr_res.empty())
		{
			ocr_res = format_postcode(ocr_res);
		}
		else
		{
			log_str += "ocr result is empty!";
		}
	}
	else
	{
		log_str += "not find postcode line; ";
	}
#ifdef DEBUG_LOTTERY_TAG
	cv::waitKey(5);
#endif // DEBUG_LOTTERY_TAG

	return ocr_res;
}

std::string OCRLotteryTag::get_last_log()
{
	return log_str;
}

int OCRLotteryTag::get_postcode_line(cv::Mat srcm, cv::Mat &dstm)
{
	int dst_width = 720;
	float scal_ = float(dst_width) / srcm.cols;
	cv::Mat nmat;
	cv::Mat gmat;
	cv::resize(srcm, gmat, cv::Size(), scal_, scal_, cv::INTER_LINEAR);
	int width = gmat.cols;
	int height = gmat.rows;
	if (srcm.channels()==3)
	{
		cv::cvtColor(srcm, gmat, cv::COLOR_BGR2GRAY);
	}

	cv::normalize(gmat, nmat, 255, 0, cv::NORM_MINMAX);
	//cv::imshow("nmat", nmat);
	cv::threshold(nmat, nmat, 120, 255, cv::THRESH_BINARY_INV);
#ifdef DEBUG_LOTTERY_TAG
	cv::imshow("threshmat", nmat);
#endif // DEBUG_LOTTERY_TAG

	

	Mat element = cv::getStructuringElement(cv::MORPH_RECT, Size(35, 3));
	cv::morphologyEx(nmat, nmat, cv::MORPH_CLOSE, element, cv::Point(-1, -1), 1, 0, 0);
	element = cv::getStructuringElement(cv::MORPH_RECT, Size(3, 3));
	cv::morphologyEx(nmat, nmat, cv::MORPH_ERODE, element);
	
#ifdef DEBUG_LOTTERY_TAG
	cv::imshow("bnmat_contour", nmat);
#endif // DEBUG_LOTTERY_TAG

	


	// 查找轮廓
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(nmat, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	std::vector<std::vector<cv::Point>> contours_cand;

	//过滤轮廓-宽高比，尺寸过滤
	//std::vector<cv::Point> tl_pt;
	std::vector<std::vector<cv::Point>>::iterator it;
	std::vector<std::vector<std::vector<cv::Point>>::iterator> its;
	for (it = contours.begin(); it != contours.end(); it++)
	{
		cv::Rect rc;
		int res = ImageProcessFunc::getContourRect(*it, rc); 
		if(rc.width<100 ||rc.height<20) continue;
		if (rc.width > 0.35*width && rc.height > 0.0666*width && rc.width < 0.7*width &&
			rc.x < 0.25*width && rc.y < 0.75*height && rc.y > 0.2*height)
		{
			contours_cand.push_back(*it);
		}
	}

	if (contours_cand.empty())
	{
		log_str += "can not find center barcode; ";
		return 0;
	}

	for (it = contours_cand.begin(); it != contours_cand.end(); it++)
	{
		cv::Rect rc;
		cv::RotatedRect rrc = cv::minAreaRect(*it);
		double wrh = std::max(rrc.size.width, rrc.size.height) / std::min(rrc.size.width, rrc.size.height);
		if (contourArea(*it) / rrc.size.area() < 0.94 || wrh < 5 || wrh > 8)
		{
			it = contours_cand.erase(it);
		}
// 		std::cout<< (contourArea(*it) / rrc.size.area())<<std::endl;
// 		std::cout << wrh << std::endl;
	}

	// 确定条形码位置
	int centpos_y = height;
	int ind = -1;
	cv::Rect barcode_rc;

	for (int i=0;i< contours_cand.size();i++)
	{
		cv::Rect rc;
		int res = ImageProcessFunc::getContourRect(contours_cand[i], rc);
		int pos_y = std::abs(rc.x + rc.height / 2 - height/2); //靠近中间
		if (pos_y < centpos_y)
		{
			ind = i;
			centpos_y = pos_y;
			barcode_rc = rc;
		}
	}

	if (ind == -1)
	{
		log_str += "Can not find center barcode; ";
		return 0;
	}
	
    // 定位邮编行
	//cv::rectangle(nmat, barcode_rc, cv::Scalar(150, 150, 150), 2);
	//cv::imshow("barcode", nmat);
	

	cv::RotatedRect barcode_rrect = cv::minAreaRect(contours_cand[ind]);
	

	if (barcode_rrect.angle < -45 || barcode_rrect.angle > 45)
	{
		int a = barcode_rrect.size.width;
		barcode_rrect.size.width = barcode_rrect.size.height;
		barcode_rrect.size.height = a;
	}

	cv::Rect postcode_range_rc(barcode_rrect.center.x-barcode_rrect.size.width/2, 
		barcode_rrect.center.y - barcode_rrect.size.height / 2, 
		barcode_rrect.size.width, barcode_rrect.size.height);

	postcode_range_rc.y -= postcode_range_rc.height; 
	postcode_range_rc.width += postcode_range_rc.x;
	postcode_range_rc.x = 0;
	
	
	cv::Rect postcode_rc;
	ind = -1;
	for (it = contours.begin(); it != contours.end(); it++)
	{
		cv::Rect rc;
		int res = ImageProcessFunc::getContourRect(*it, rc);
		if (rc.width < 12 || rc.height < 12) continue;

		cv::RotatedRect rrc = cv::minAreaRect(*it);

		if (rrc.angle < -45 || rrc.angle > 45)
		{
			int a = rrc.size.width;
			rrc.size.width = rrc.size.height;
			rrc.size.height = a;
		}

		rc = cv::Rect(rrc.center.x - rrc.size.width / 2,
			rrc.center.y - rrc.size.height / 2,
			rrc.size.width, rrc.size.height);

		if (rc.y> postcode_range_rc.y && rc.y < (postcode_range_rc.y+postcode_range_rc.height) &&
			(rc.y + rc.height) > postcode_range_rc.y && (rc.y + rc.height) < (postcode_range_rc.y + postcode_range_rc.height) &&
			rc.x + rc.width < postcode_range_rc.x + postcode_range_rc.width)
		{
			ind = 0;
			postcode_rc = rc;
		}
	}
	if (ind == -1)
	{
		log_str += "can not find postcode range; ";
		return 0;
	}

	postcode_rc.x -= 5;
	postcode_rc.y -= 5;
	postcode_rc.width += 10;
	postcode_rc.height += 10;

	ImageProcessFunc::CropRect(cv::Rect(0, 0, nmat.cols, nmat.rows), postcode_rc);

	//定位邮编行
	cv::rectangle(nmat, postcode_rc, cv::Scalar(150, 150, 150), 2);
	//cv::imshow("postcode_range", nmat);

	cv::Mat postcodemat;

	gmat(postcode_rc).copyTo(dstm);

#ifdef DEBUG_LOTTERY_TAG
	cv::imshow("postcode", dstm);
#endif // DEBUG_LOTTERY_TAG


	return 1;
}

int OCRLotteryTag::_run_ocr(cv::Mat post_code_line, std::string &results, OcrAlgorithm_config *pConfig)
{
	if (post_code_line.empty())
	{
		log_str += "tag mat is empty, ";
		return 0;
	}


	Mat srcm;
	post_code_line.copyTo(srcm);
	if (srcm.channels() == 3)
	{
		cv::cvtColor(srcm, srcm, COLOR_BGR2GRAY);
	}

	cv::Rect mR(0,0,srcm.cols,srcm.rows);


	cv::normalize(srcm, srcm, 255, 0, cv::NORM_MINMAX);

	double vPix = ImageProcessFunc::getAveragePixelInRect(srcm, mR);
	double anchor = 120;
	double alpha = 3.0;
	double beta = 200 - vPix;

	ImageProcessFunc::adJustBrightness(srcm, alpha, beta, anchor);

	//cv::threshold(resizedMat, resizedMat, vPix*0.6, 255, cv::THRESH_BINARY);
#ifdef DEBUG_LOTTERY_TAG
	imshow("_runOcr调整亮度后", srcm);
#endif // OCR_DEBUG
	int w = srcm.cols;
	int h = srcm.rows;
	unsigned char *pImgData = srcm.data;

	//cout << "h" << w << endl;

	tesseract::TessBaseAPI* pTess = (tesseract::TessBaseAPI*)pConfig->pTessThld;
	if (pTess == NULL)
	{
		log_str += "tesseract pointer is Null,";
		return 0;
	}
	pTess->SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_LINE);
	//pTess->SetVariable("save_best_choices", "T");
	pTess->SetImage(pImgData, w, h, srcm.channels(), srcm.step1());
	pTess->SetVariable("user_defined_api", "72");
	pTess->Recognize(0);

	// get result and delete[] returned char* string
#ifdef DEBUG_LOTTERY_TAG
	std::cout << std::unique_ptr<char[]>(pTess->GetUTF8Text()).get() << std::endl;
#endif // OCR_DEBUG
	//

	char *res_data = NULL;
	res_data = pTess->GetUTF8Text();
	//pTess->GetUNLVText();
	//std::cout <<"strlen:"<< strlen(res_data) << std::endl;
	//std::cout << res_data << std::endl;
	int str_len = strlen(res_data);
	size_t max_lenth = std::min(str_len, 64);
	results = std::string(res_data, max_lenth);

	if (res_data != NULL)
		delete[] res_data;
	return 1;
}

std::string OCRLotteryTag::format_postcode(std::string &res_str)
{

	int digt_len = 0;
	string postcode;
	bool start = true;
	for (int i = res_str.size() / 2; i < res_str.size(); i++)
	{
		unsigned char c = res_str.at(i);
		if (c == '.' || c == ',')
		{
			c = ' ';
		}
		if ((c == ' ' || c == '\n') && start == false)
		{
			start = true;
			continue;
		}
		if (start && _isdigit(c))
		{
			digt_len++;
			continue;
		}
		if ((c == ' ' || c == '\n') && start == true)
		{
			if (digt_len == 5)
			{
				postcode = res_str.substr(i - digt_len, digt_len);
			}
			digt_len = 0;
			continue;
		}
		if (!_isdigit(c))
		{
			digt_len = 0;
			start = false;
		}
	}

	if (digt_len == 5 )
	{
		postcode = res_str.substr(res_str.size() - digt_len, digt_len);
	}
	return postcode;

}

