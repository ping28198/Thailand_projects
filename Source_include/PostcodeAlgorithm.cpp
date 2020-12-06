#include "PostcodeAlgorithm.h"
#include <onnxruntime_cxx_api.h>
//#include <cuda_provider_factory.h>
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
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/xfeatures2d/nonfree.hpp"

using namespace cv;
using namespace std;


int importMat_(void *p_args)
{
	//time_t t0, t1;
	//t0 = clock();
	importMat_args *args = (importMat_args *)p_args;
	cv::Mat srcm = *(args->pMat);
	cv::Mat m;
	cv::resize(srcm, m, cv::Size(args->new_width, args->new_height), 0, 0,cv::INTER_AREA);
	cv::cvtColor(m, m, COLOR_BGR2GRAY);

	cv::copyMakeBorder(m, m, (args->height_ - args->new_height + 1) / 2, (args->height_ - args->new_height) / 2,
		(args->width_ - args->new_width + 1) / 2, (args->width_ - args->new_width) / 2, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
	m.convertTo(m, CV_32FC1);//彩色
	m = m / 255.0f;
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
	//设定输入图片的数量，实际输入的图片小于等于该值
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
	//session_options.SetIntraOpNumThreads(1);
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
	//if (cuda_id>=0)
	//	OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, cuda_id);

	//

	// Available levels are
	// ORT_DISABLE_ALL -> To disable all optimizations
	// ORT_ENABLE_BASIC -> To enable basic optimizations (Such as redundant node removals)
	// ORT_ENABLE_EXTENDED -> To enable extended optimizations (Includes level 1 + more complex optimizations like node fusions)
	// ORT_ENABLE_ALL -> To Enable All possible opitmizations

	env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "");

	try
	{
		session_ = new Ort::Session(*env, model_file, session_options);
	}
	catch (const Ort::Exception& e)
	{
		std::cout << e.what() <<std::endl;
		
	}
	

	

	return 1;
}
std::string print_shape(const std::vector<int64_t>& v) {
	std::stringstream ss("");
	for (size_t i = 0; i < v.size() - 1; i++)
		ss << v[i] << "x";
	ss << v[v.size() - 1];
	return ss.str();
}

int TagDetector::run_detect(std::vector<std::vector<cv::RotatedRect>> & rrects, std::vector<std::vector<int>> &cls_inds)
{
	//session_->Run(Ort::RunOptions{ nullptr }, input_node_names.data(), &input_tensor_, 1,
	//	output_node_names.data(), &output_tensor_, 1);
	try
	{
		auto output_tensors = session_->Run(Ort::RunOptions{ nullptr }, input_node_names.data(), &input_tensor_, 1, output_node_names.data(), 1);
		for (int i=0;i<output_tensors.size();i++)
		{
			auto output_shape = output_tensors[i].GetTensorTypeAndShapeInfo().GetShape();
			float *pData = output_tensors[i].GetTensorMutableData<float>();
			int obj_num = output_shape[0];
			convert_to_rrects(pData, obj_num, i, rrects[i], cls_inds[i]);
		}

	}
	catch (const Ort::Exception& e)
	{
		std::cout << e.what() << std::endl;

	}
	

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
	// #pragma omp paralle for // 图片数量较多时使用
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

void TagDetector::convert_to_rrects(float *src_data, size_t num_objs, size_t image_ind,
	std::vector<cv::RotatedRect> & rrects, std::vector<int> &cls_inds)
{
	const int feats_per = 7;
	std::vector<std::array<cv::Point2f, 4>> parcels;
	//parcels.reserve(MAX_BOX_NUM);
	//one_res.reserve(MAX_BOX_NUM);

	cv::Point2f _cent;
	cv::Point2f _size;
	cv::RotatedRect rrc;
	for (int n = 0; n < num_objs; n++)
	{

		float *pData = src_data + n * FEATS_PER;
#ifdef DEBUG_ONNX_EFFICIENTDET_R0
		printf("detect box %d: %f %f %f %f %f %f %f\n", n, pData[0], pData[1], pData[2], pData[3],
			pData[4], pData[5], pData[6]);
#endif
		if (pData[5] < this->m_confidence_threshold) continue;
		_size.x = pData[3] < 1 ? 1 : pData[3];
		_size.y = pData[2] < 1 ? 1 : pData[2];
		_cent.x = pData[0];
		_cent.y = pData[1];
		float theta = -pData[4] / CV_PI * 180;
		//theta = (theta > 90) ? (90 - theta) : theta;
		rrc.angle = theta;
		rrc.center = _cent;
		rrc.size = _size;
		rrects.push_back(rrc);
		cls_inds.push_back(floor(pData[6]));
	}

	nms_multi_class(rrects, cls_inds, this->m_iou_threshold);
	cv::Point2f scal_pt(shift_x[image_ind], shift_y[image_ind]);
	//恢复到原始图像尺寸
	for (int i = 0; i < rrects.size(); i++)
	{
		rrects[i].center -= scal_pt;
		rrects[i].center.x /= resize_scal[image_ind];
		rrects[i].center.y /= resize_scal[image_ind];
		rrects[i].size.height /= resize_scal[image_ind];
		rrects[i].size.width /= resize_scal[image_ind];
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
			if (c_dist<std::min(it0->size.width, it0->size.width) || c_dist < std::min(it1->size.width, it1->size.width))
			{
				it1 = rects.erase(it1);
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
	if (rects.size()<=1)
	{
		return;
	}
	
	std::vector<std::pair<cv::RotatedRect, int>> rects_cls;
	rects_cls.reserve(rects.size());
	for (size_t i=0;i<rects.size();i++)
	{
		std::pair<cv::RotatedRect, int> rct;
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
	for (size_t i=1;i<rects_cls.size()+1;i++)
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
			if (same_cls_count==0)
			{
				rects.push_back(rects_cls[i - 1].first);
				class_ids.push_back(cls);
				same_cls_count = 0;
			}
			else
			{
				std::vector<cv::RotatedRect> rt;
				for (int j=i-same_cls_count-1;j<i;j++)
				{
					rt.push_back(rects_cls[j].first);
				}
				nms_rotated_rect(rt, iou_threshold);
				rects.insert(rects.end(), rt.begin(), rt.end());
				for (int j=0;j<rt.size();j++)
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
	run_detect(rrects,cls_inds);
	t0 = clock();
	timeconsume = (double)(t0 - t1);
	std::cout << "model time consume:" << timeconsume << "ms" << std::endl;
	
	t1 = clock();
	timeconsume = (double)(t1 - t0);
	std::cout << "post-process time consume:" << timeconsume << "ms" << std::endl;
#else

	importMat(srcms);
	run_detect(rrects, cls_inds);
	//convert_to_rrects(&results_, rrects, cls_inds);

#endif // DEBUG_ONNX_EFFICIENTDET_R0



}

OCRStandardTag::OCRStandardTag()
{

}

std::string OCRStandardTag::get_postcode_string(cv::Mat tag_mat, OcrAlgorithm_config *pConfig)
{
	string postcodestr;
	cv::Rect rt(tag_mat.cols/2,0,tag_mat.cols/2,tag_mat.rows/2);
	cv::Mat rtmat = tag_mat(rt);
	//cv::Mat match_linemat = cv::imread("E:\\cpp_projects\\Thailand_projects\\Projects\\_run_dir\\resource\\match_line_image.jpg");
	cv::Mat textrange_mat;
	locate_text_range(rtmat, textrange_mat, pConfig); //定位右上角文字区域
	if (textrange_mat.empty())
		return postcodestr;
	cv::Mat post_code_line;
	get_postcode_line(textrange_mat, post_code_line); // 定位邮编行
	if (!post_code_line.empty())
	{
		_run_ocr(post_code_line, postcodestr, pConfig);
		format_postcode(postcodestr);
	}
	return postcodestr;
}


int OCRStandardTag::get_postcode_line(cv::Mat srcm, cv::Mat &dstm)
{
	cv::Mat roiMat = srcm;
	if (srcm.channels() == 3)
		cvtColor(srcm, roiMat, COLOR_BGR2GRAY);

	float scal_ = 150.0 / roiMat.rows;
	cv::resize(roiMat, roiMat, cv::Size(), scal_, scal_);

	int g_width = roiMat.cols;

	cv::Rect adjr = cv::Rect(0, roiMat.rows / 2, roiMat.cols, roiMat.rows / 2);
	float pix = ImageProcessFunc::getAveragePixelInRect(roiMat, adjr);
	cv::Mat g_img = roiMat;
	ImageProcessFunc::adJustBrightness(g_img, 1.5, 2, pix*0.6);

	
	//cv::GaussianBlur(roiMat, g_img, cv::Size(3, 3), 0);
	cv::Canny(g_img, g_img, 20, 150);

#ifdef POSTCODE_ROI_DEBUG
	cv::imshow("canny处理结果", g_img);
#endif // POSTCODE_ROI_DEBUG

	threshold(g_img, g_img, 30, 255, THRESH_BINARY);
	//bitwise_not(g_img, g_img);



	g_img = g_img(Rect(0, 0, g_img.cols-30, g_img.rows));
	g_img = g_img.clone();


#ifdef POSTCODE_ROI_DEBUG
	imshow("形态学处理前图像", g_img);
#endif // POSTCODE_ROI_DEBUG


	//形态学运算
	Mat element = getStructuringElement(MORPH_RECT, Size(30, 1));
	morphologyEx(g_img, g_img, MORPH_CLOSE, element);

	element = getStructuringElement(MORPH_RECT, Size(7, 7));
	morphologyEx(g_img, g_img, MORPH_ERODE, element);

	element = getStructuringElement(MORPH_RECT, Size(55, 5));
	morphologyEx(g_img, g_img, MORPH_OPEN, element);


#ifdef POSTCODE_ROI_DEBUG
	imshow("形态学处理后图像", g_img);
#endif // POSTCODE_ROI_DEBUG
	//
	//waitKey(0);


	//缩小统计区域
	g_img = g_img(Rect(g_img.cols / 3, 0, g_img.cols * 2 / 3, g_img.rows));

	// 计算可能包含文字的行
	vector<unsigned int>PixelsAdd;
	ImageProcessFunc::sumPixels(g_img, 0, PixelsAdd);

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
#ifdef POSTCODE_ROI_DEBUG
		printf("未找到字符行\n");
#endif // POSTCODE_ROI_DEBUG
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
#ifdef POSTCODE_ROI_DEBUG
		printf("联通字符行后，未找到字符行\n");
#endif // POSTCODE_ROI_DEBUG
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
#ifdef POSTCODE_ROI_DEBUG
		printf("未找到字符行\n");
#endif // POSTCODE_ROI_DEBUG
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
#ifdef POSTCODE_ROI_DEBUG
		printf("字符行不正确\n");
#endif // POSTCODE_ROI_DEBUG
		return 0;
	}

	//判断是否出现两行粘连
	vector<int> bars_pos_y_vec;
	for (int i = 0; i < 3; i++)
	{
		bars_pos_y_vec.push_back(bars_vec[i].y - bars_vec[i].x);
		cout << "bars " << i << " pos:" << bars_pos_y_vec[i] << endl;
	}
	std::sort(bars_pos_y_vec.begin(), bars_pos_y_vec.end());

//	if (bars_pos_y_vec[2] >= 2 * bars_pos_y_vec[0])
//	{
//#ifdef POSTCODE_ROI_DEBUG
//		printf("出现行粘连\n");
//#endif // POSTCODE_ROI_DEBUG
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
#ifdef POSTCODE_ROI_DEBUG
		printf("邮编定位超出界限\n");
#endif // POSTCODE_ROI_DEBUG
		return 0;
	}
	Rec_postcode.x = Rec_postcode.x / scal_;
	Rec_postcode.y = Rec_postcode.y / scal_;
	Rec_postcode.width = Rec_postcode.width / scal_;
	Rec_postcode.height = Rec_postcode.height / scal_;
	ImageProcessFunc::CropRect(cv::Rect(0,0,srcm.cols,srcm.rows), Rec_postcode);

	dstm = srcm(Rec_postcode).clone();

	return 1;
}

int OCRStandardTag::_run_ocr(cv::Mat post_code_line, std::string &results, OcrAlgorithm_config *pConfig)
{
	if (post_code_line.empty()) return 0;
	Mat srcm = post_code_line;
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
		resizedMat = srcm.clone();
	}

	cv::Rect mR;
	mR.x = 0;
	mR.y = resizedMat.rows/2;
	mR.height = resizedMat.rows/2;
	mR.width = resizedMat.cols / 2;
	

	double vPix = ImageProcessFunc::getAveragePixelInRect(resizedMat, mR);
	double anchor = 120;
	double alpha = 3.0;
	double beta = 200 - vPix;
	ImageProcessFunc::adJustBrightness(resizedMat, alpha, beta, anchor);

	//cv::threshold(resizedMat, resizedMat, vPix*0.6, 255, cv::THRESH_BINARY);
#ifdef POSTCODE_ROI_DEBUG
	imshow("_runOcr调整亮度后", resizedMat);
#endif // OCR_DEBUG
	int w = resizedMat.cols;
	h = resizedMat.rows;
	unsigned char *pImgData = resizedMat.data;

	//cout << "h" << w << endl;

	tesseract::TessBaseAPI* pTess = (tesseract::TessBaseAPI*)pConfig->pTessEn;
	if (pTess ==NULL)
	{
		return 0;
	}
	pTess->SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_LINE);
	//pTess->SetVariable("save_best_choices", "T");
	pTess->SetImage(pImgData, w, h, resizedMat.channels(), resizedMat.step1());
	pTess->SetVariable("user_defined_api","300");
	pTess->Recognize(0);

	// get result and delete[] returned char* string
#ifdef OCR_DEBUG
	std::cout << std::unique_ptr<char[]>(pTess->GetUTF8Text()).get() << std::endl;
#endif // OCR_DEBUG
	//
	char tmp[512] = { 0 };
	char *res_data = pTess->GetUTF8Text();
	strcpy_s(tmp, 512, res_data);
	delete[] res_data;
	results = tmp;



	return 1;
}

int OCRStandardTag::locate_text_range(cv::Mat srcm,cv::Mat &dstm, OcrAlgorithm_config *pConfig)
{
	const int MAX_KEYPOINTS = 50;
	using namespace cv;
	using namespace std;
	using namespace cv::xfeatures2d;

	 //参考的图像宽高。

	//cv::Mat im2Gray = match_m;

	if (srcm.channels() == 3)
		cv::cvtColor(srcm, srcm, COLOR_BGR2GRAY);
	//if (match_m.channels() == 3)
	//	cv::cvtColor(match_m, im2Gray, COLOR_BGR2GRAY);
	
	float scal = 200.0/ srcm.rows;
	cv::resize(srcm, srcm, cv::Size(), scal, scal);

	//scal = 50.0 / im2Gray.rows;
	//cv::resize(im2Gray, im2Gray, cv::Size(), scal, scal);

	cv::Mat im1Gray;
	cv::blur(srcm, im1Gray, cv::Size(3,3));
	//cv::GaussianBlur(srcm, im1Gray, cv::Size(3, 3), 0);

	cv::Rect adjr = cv::Rect(0, im1Gray.rows / 2, im1Gray.cols, im1Gray.rows / 2);
	float pix = ImageProcessFunc::getAveragePixelInRect(im1Gray, adjr);
	ImageProcessFunc::adJustBrightness(im1Gray, 1.3, 120 - pix, pix*0.5);



#ifdef POSTCODE_ROI_DEBUG
	cv::imshow("tagsrc", im1Gray);
	//Mat result;
	//drawMatches(im1Gray, n_keypoints1, im2Gray, n_keypoints2, matches, result, Scalar(0, 255, 0), Scalar::all(-1));//匹配特征点绿色，单一特征点颜色随机
	//imshow("Match_Result", result);
#endif // POSTCODE_ROI_DEBUG





	cv::Size refSize(269 + 10, 50*3.5);


	std::vector<KeyPoint> keypoints1, keypoints2;
	cv::Mat descriptors1, descriptors2;
	cv::Ptr<Feature2D> sift1 = cv::xfeatures2d::SIFT::create(MAX_KEYPOINTS*4);
	//cv::Ptr<Feature2D> sift2 = cv::xfeatures2d::SIFT::create(MAX_KEYPOINTS);
	sift1->detectAndCompute(im1Gray, Mat(), keypoints1, descriptors1);
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

	//剔除不匹配的keypoint
	float keypoint_percent = 0.6;
	nth_element(matches.begin(), matches.begin() + int(MAX_KEYPOINTS*keypoint_percent), matches.end());   //提取出前30个最佳匹配结果     
	matches.erase(matches.begin() + int(MAX_KEYPOINTS*keypoint_percent)+1, matches.end());    //剔除掉其余的匹配结果
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


	if (matches.size() < 3)
	{
#ifdef POSTCODE_ROI_DEBUG
		cout << "匹配点数小于3" << endl;
#endif // POSTCODE_ROI_DEBUG
		return 0;
	}


#ifdef POSTCODE_ROI_DEBUG
	//Mat result;
	//drawMatches(im1Gray, n_keypoints1, im2Gray, n_keypoints2, matches, result, Scalar(0, 255, 0), Scalar::all(-1));//匹配特征点绿色，单一特征点颜色随机
	//imshow("Match_Result", result);
#endif // POSTCODE_ROI_DEBUG

	

	std::vector<Point2f> points_1, points_ref;
	for (size_t i = 0; i < matches.size(); i++)
	{
		points_1.push_back(n_keypoints1[matches[i].trainIdx].pt);
		points_ref.push_back(n_keypoints2[matches[i].queryIdx].pt);
	}

	Mat h1 = cv::findHomography(points_1, points_ref, cv::RANSAC);
	cv::warpPerspective(srcm, dstm, h1, refSize);


#ifdef POSTCODE_ROI_DEBUG
	cv::imshow("dstimg", dstm);
#endif // POSTCODE_ROI_DEBUG
	return 1;



}

int OCRStandardTag::format_postcode(std::string &str)
{

	string results;
	double score = postcodeStringScore(str, results);

	if (score>0.5)
	{
		str = results;
	}
	else
	{
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
		if (c == ' ' || c == '-')
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
double OCRStandardTag::postcodeStringScore(std::string srcStr, std::string &resultStr)
{
	//////////////////////////////////////////////////////////////////////////
//根据协议设定
	const int length_left = 5;
	const int length_right_4 = 4;
	const int length_right_5 = 5;
	if (srcStr.size() < length_left) return 0;

	double whole_score = 0;
	///去除空格
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


	//////////////////////////////////////////////////////////////////////////
//有“-”的情况 5+5,5+4的情况,
	size_t _pos = srcStr.find('-');
	if (_pos != srcStr.npos)
	{
		//判断-号位置，
		float div_pos_score = (fabs(_pos - srcStr.size() / 2.0)) / srcStr.size() * 2.0;//约往中间得分越低(0-1)
		//if (div_pos_score < 0.75)//-号在中间
		//{
		std::string substr1 = srcStr.substr(0, _pos);
		std::string substr2 = srcStr.substr(_pos + 1);

		double score_left = continuousDigitsScore(substr1, length_left);

		std::string resStr1, resStr2;
		getFirstContinuousDigits(substr1, length_left, resStr1);

		double score_right_5 = continuousDigitsScore(substr2, length_right_5);
		double score_right_4 = continuousDigitsScore(substr2, length_right_4);
		double score_right = max(score_right_5, score_right_4);
		int max_length_ = (score_right_5 < score_right_4) ? length_right_4 : length_right_5;
		getFirstContinuousDigits(substr2, max_length_, resStr2);
		resultStr = resStr1 + resStr2;
		whole_score = score_left * score_right;

		return whole_score;
		//}
	}

	//////////////////////////////////////////////////////////////////////////
	//没有找到正确的-号的情况
	//for (int i = 0; i < srcStr.length();)//剔除空格
	//{
	//	unsigned char c = srcStr.at(i);
	//	if (c == '-')
	//	{
	//		srcStr.erase(i, 1);
	//	}
	//	else
	//	{
	//		i++;
	//	}
	//}

	//是否考虑-被识别为空格的情况。。。
	double score_l_r5 = continuousDigitsScore(srcStr, length_left + length_right_5);
	double score_l_r4 = continuousDigitsScore(srcStr, length_left + length_right_4);
	double score_r5 = 0;// continuousDigitsScore(srcStr, length_right_5); //只识别到5位的将拒识

	int max_i = 0;
	if (score_l_r4 > score_l_r5)
	{
		if (score_l_r4 >= score_r5)
		{
			getFirstContinuousDigits(srcStr, length_left + length_right_4, resultStr);
			whole_score = score_l_r4;

		}
		else
		{
			getFirstContinuousDigits(srcStr, length_right_5, resultStr);
			whole_score = score_r5;
		}
	}
	else
	{
		if (score_l_r5 >= score_r5)
		{
			getFirstContinuousDigits(srcStr, length_left + length_right_5, resultStr);
			whole_score = score_l_r5;

		}
		else
		{
			getFirstContinuousDigits(srcStr, length_right_5, resultStr);
			whole_score = score_r5;
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
	cv::Ptr<Feature2D> sift1 = cv::xfeatures2d::SIFT::create(MAX_KEYPOINTS);

	sift1->detectAndCompute(im2Gray, Mat(), keypoints_tag_line, descriptors_tag_line);

	return 1;
}

OCRArbitaryTag::OCRArbitaryTag()
{

}

std::string OCRArbitaryTag::get_postcode_string(cv::Mat tag_mat, OcrAlgorithm_config *pConfig)
{
	string results;
	_run_ocr(tag_mat, results, pConfig);
	results = format_results(results);
	return results;
}

int OCRArbitaryTag::_run_ocr(cv::Mat post_code_line, std::string &results, OcrAlgorithm_config *pConfig)
{
	if (post_code_line.empty()) return 0;
	Mat srcm = post_code_line;
	if (srcm.channels() == 3)
	{
		cv::cvtColor(srcm, srcm, COLOR_BGR2GRAY);
	}
	///////////// 预处理
	cv::Mat resizedMat = srcm.clone();
	//int h = srcm.rows;
	//int sh = 20;
	//float scal_ = sh / float(h);
	//cv::Mat resizedMat;
	//if (scal_ > 1.1)
	//{
	//	cv::resize(srcm, resizedMat, cv::Size(), scal_, scal_, cv::INTER_AREA);
	//}
	//else
	//{
	//	resizedMat = srcm.clone();
	//}

	cv::Rect mR;
	mR.x = 0;
	mR.y = resizedMat.rows / 2;
	mR.height = resizedMat.rows / 2;
	mR.width = resizedMat.cols / 2;


	double vPix = ImageProcessFunc::getAveragePixelInRect(resizedMat, mR);
	double anchor = 120;
	double alpha = 3.0;
	double beta = 200 - vPix;
	ImageProcessFunc::adJustBrightness(resizedMat, alpha, beta, anchor);

	//cv::threshold(resizedMat, resizedMat, vPix*0.6, 255, cv::THRESH_BINARY);
#ifdef ARBITURARY_TAG_DEBUG
	imshow("_runOcr调整亮度后", resizedMat);
#endif // OCR_DEBUG
	int w = resizedMat.cols;
	int h = resizedMat.rows;
	unsigned char *pImgData = resizedMat.data;

	//cout << "h" << w << endl;

	tesseract::TessBaseAPI* pTess = (tesseract::TessBaseAPI*)pConfig->pTessThld;
	if (pTess == NULL)
	{
		return 0;
	}
	pTess->SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_BLOCK);
	//pTess->SetVariable("save_best_choices", "T");
	pTess->SetImage(pImgData, w, h, resizedMat.channels(), resizedMat.step1());
	pTess->SetVariable("user_defined_api", "72");
	pTess->Recognize(0);

	// get result and delete[] returned char* string
#ifdef ARBITURARY_TAG_DEBUG
	std::cout << std::unique_ptr<char[]>(pTess->GetUTF8Text()).get() << std::endl;
#endif // OCR_DEBUG
	//
	char tmp[2048] = { 0 };
	char *res_data = pTess->GetUTF8Text();
	strcpy_s(tmp, 2047, res_data);
	if(res_data!=nullptr)
		delete[] res_data;
	results = tmp;

	return 1;
}

std::string OCRArbitaryTag::format_results(std::string res_str)
{
	int digt_len = 0;
	string postcode;
	bool start = true;
	for (int i=0;i<res_str.size();i++)
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
		if (start && isdigit(c))
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
		if (!isdigit(c))
		{
			digt_len = 0;
			start = false;
		}
	}
	if (digt_len==5)
	{
		postcode = res_str.substr(res_str.size() - digt_len, digt_len);
	}
	return postcode;
}
