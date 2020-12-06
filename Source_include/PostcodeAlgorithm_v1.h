#pragma once
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <onnxruntime_cxx_api.h>
#include <cuda_provider_factory.h>
#include <string>
#include <ThreadPool.h>
#include "ImageProcessFunc.h"
#include <string>
#include <vector>

#define MAX_BOX_NUM 32




//#define DEBUG_ONNX_EFFICIENTDET_R0
//#define POSTCODE_ROI_DEBUG
#define ARBITURARY_TAG_DEBUG


class MatchDataStruct
{
public:
	cv::Mat descriptors_tag_line;
	std::vector<cv::KeyPoint> keypoints_tag_line;
	//int loadMatchData(const std::string &xmlfile);
	//int saveMatchData(const std::string &xmlfile);
	int getMatchDataFromImg_tag_line(const std::string &refImg1);

};



class OcrAlgorithm_config
{
public:
	std::string tess_en_data_path;
	std::string tess_thld_data_path;
	std::string tess_template_path;
	std::string handwrite_model_path;
	std::string tag_ref_line_img_path;


	void *pTessEn;
	void *pTessThld;
	void *pHWDigitsRecog;
	void *pTagDetector;
	void *pLogger;

	float TagDetectConfidence;
	float HandwriteDigitsConfidence;

	int Run_OCR_on_standard_tag; //是否检测标准标签邮编
	int Run_OCR_on_handwrite_box; //是否检测手写邮编
	int Run_OCR_on_unknown_tag; //是否检测任意标签
	int is_test_model; //是否为测试模式，仅为测试验收使用

	MatchDataStruct match_data;

	OcrAlgorithm_config()
	{
		pTessEn = NULL;
		pTessThld = NULL;
		pHWDigitsRecog = NULL;
		pTagDetector = NULL;
		pLogger = NULL;
		HandwriteDigitsConfidence = 0.85;
		TagDetectConfidence = 0.9;
		Run_OCR_on_handwrite_box = 1;
		Run_OCR_on_standard_tag = 1;
		Run_OCR_on_unknown_tag = 1;
		is_test_model = 0;
	}



};


struct importMat_args
{
	cv::Mat *pMat;
	int width_;
	int height_;
	float *pDst;
	int new_width;
	int new_height;
	int channel_;
};



class TagDetector
{
public:
	TagDetector(const wchar_t* model_path, float confidence_threshold=0.3, int cuda_id=-1, size_t input_image_num = 1); // cuda_id = -1 use cpu

	int initial();

private:
	static const int MAX_IMAGE_NUM = 2;
	static const int WIDTH_ = 512;
	static const int HEIGHT_ = 512;
	static const int CHANNEL_ = 3;
	

	//static const int MAX_BOX_NUM = 128;
	static const int FEATS_PER = 8;

	std::array<float, MAX_IMAGE_NUM * WIDTH_ * HEIGHT_ * CHANNEL_> *input_image_ = NULL; //输入图像数据位置
	std::array<float, MAX_IMAGE_NUM * MAX_BOX_NUM * FEATS_PER> results_{ 0 }; //输出结果


	std::vector<const char*> input_node_names = { "input" };
	std::vector<const char*> output_node_names = { "output" };

	Ort::Env *env;

	Ort::SessionOptions session_options;

	Ort::Session *session_ = nullptr;

	Ort::Value input_tensor_{ nullptr };
	std::array<int64_t, 4> input_shape_{ 1, CHANNEL_, HEIGHT_, WIDTH_ };

	Ort::Value output_tensor_{ nullptr };
	std::array<int64_t, 3> output_shape_{ 1, MAX_BOX_NUM, FEATS_PER };


	float m_confidence_threshold = 0.5;
	float m_iou_threshold = 0.2;


	//用于将模型输出结果转换到 输入图像尺寸
	int shift_x[MAX_IMAGE_NUM] = { 0 };
	int shift_y[MAX_IMAGE_NUM] = { 0 };
	float resize_scal[MAX_IMAGE_NUM] = { 1.0 };

	//用于图像坐标到物理坐标的转换
	bool is_applied_transform[MAX_IMAGE_NUM] = { false };
	float trans_scale_x[MAX_IMAGE_NUM] = { 1.0 };
	float trans_scale_y[MAX_IMAGE_NUM] = { 1.0 };
	float trans_shift_x[MAX_IMAGE_NUM] = { 0 };
	float trans_shift_y[MAX_IMAGE_NUM] = { 0 };

	//cuda 输入图像处理
	cv::cuda::GpuMat m_g_Mat;
	cv::cuda::GpuMat m_g_fmat;

	int m_image_num = 1;
	int m_real_input_num = 1;

	//输入图像处理多线程

	//多线程函数传入参数
	ThreadPool *tpool = NULL;// = new ThreadPool(MAX_IMAGE_NUM);

	importMat_args image_args[MAX_IMAGE_NUM] = { 0 };

private:
	int initial_model(const wchar_t* model_file, size_t cuda_id = 0);

	int run_detect();

	void importMat(std::vector<cv::Mat> & srcms);
	void importMat_cpu(std::vector<cv::Mat> & srcms);
	void importMat_gpu(std::vector<cv::Mat> &srcms);

	void convert_to_rrects(std::array<float, MAX_IMAGE_NUM * MAX_BOX_NUM * FEATS_PER> *src_data,
		std::vector<std::vector<cv::RotatedRect>> & rrects, std::vector<std::vector<int>> &cls_inds);


	void nms_rotated_rect(std::vector<cv::RotatedRect> &rects, float iou_threshold);
	void nms_multi_class(std::vector<cv::RotatedRect> &rects, std::vector<int> &class_ids, float iou_threshold);
public:


	void detectParcels(std::vector<cv::Mat> &srcms, std::vector<std::vector<cv::RotatedRect>> & rrects,std::vector<std::vector<int>> &cls_inds);




};



class OCRStandardTag
{
public:
	OCRStandardTag();

	std::string get_postcode_string(cv::Mat tag_mat, OcrAlgorithm_config *pConfig);

private:
	int get_postcode_line(cv::Mat srcm, cv::Mat &dstm);

	int _run_ocr(cv::Mat post_code_line, std::string &results, OcrAlgorithm_config *pConfig);

	int locate_text_range(cv::Mat srcm, cv::Mat &dstm, OcrAlgorithm_config *pConfig);


	//用于格式化邮编
	int format_postcode(std::string &str);
	double continuousDigitsScore(std::string srcStr, int continus_num);
	int maxNumContinuousDigits(std::string srcStr);
	double postcodeStringScore(std::string srcStr, std::string &resultStr);
	size_t getFirstContinuousDigits(std::string srcStr, int conti_num, std::string &dstStr);
	


};



class OCRArbitaryTag
{
public:
	OCRArbitaryTag();

	std::string get_postcode_string(cv::Mat tag_mat, OcrAlgorithm_config *pConfig);
private:


	int _run_ocr(cv::Mat post_code_line, std::string &results, OcrAlgorithm_config *pConfig);

	std::string format_results(std::string res_str);

};