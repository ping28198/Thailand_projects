#pragma once
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <ThreadPool.h>
#include "ImageProcessFunc.h"
#include <string>
#include <vector>

#define MAX_BOX_NUM 32


//#define DEBUG_ONNX_EFFICIENTDET_R0
//#define DEBUG_STD_TAG
//#define ARBITURARY_TAG_DEBUG
//#define USE_CUDA_DEVICE
//#define DEBUG_HAND_WRITE_BOX
class MatchDataStruct
{
public:
	cv::Mat descriptors_tag_line;
	std::vector<cv::KeyPoint> keypoints_tag_line;
	int getMatchDataFromImg_tag_line(const std::string &refImg1);

};


class OcrAlgorithm_config
{
public:

	void *pTessEn=nullptr;
	void *pTessThld=nullptr;
	void *pTessHandWrt=nullptr;
	int strict_mode = 0; //严格模式下不支持邮编位数4
	int std_postcode_num = 5;
	bool support_postcode_4 = 0;
	bool support_postcode_10 = 0;

	MatchDataStruct match_data;

	OcrAlgorithm_config()
	{

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
	TagDetector(const wchar_t* model_path, float confidence_threshold = 0.3, int cuda_id = -1, size_t input_image_num = 1); // cuda_id = -1 use cpu

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

	void detectParcels(std::vector<cv::Mat> &srcms, std::vector<std::vector<cv::RotatedRect>> & rrects, std::vector<std::vector<int>> &cls_inds);


};



class OCRStandardTag
{
public:
	OCRStandardTag();

	std::string get_postcode_string(cv::Mat tag_mat, OcrAlgorithm_config *pConfig);
	std::string get_last_log();

private:
	std::string log_str;
	int get_postcode_line(cv::Mat srcm, cv::Mat &dstm);

	int _run_ocr(cv::Mat post_code_line, std::string &results, OcrAlgorithm_config *pConfig);

	int locate_text_range(cv::Mat srcm, cv::Mat &dstm, OcrAlgorithm_config *pConfig);


	//用于格式化邮编
	int format_postcode(std::string &str, OcrAlgorithm_config *pConfig=nullptr);
	double continuousDigitsScore(std::string srcStr, int continus_num);
	int maxNumContinuousDigits(std::string srcStr);
	double postcodeStringScore(std::string srcStr, std::string &resultStr, OcrAlgorithm_config *pConfig = nullptr);
	size_t getFirstContinuousDigits(std::string srcStr, int conti_num, std::string &dstStr);
	


};



class OCRArbitaryTag
{
public:
	OCRArbitaryTag();

	std::string get_postcode_string(cv::Mat tag_mat, OcrAlgorithm_config *pConfig);
	std::string get_last_log();
	std::string get_last_full_ocr_data();

private:
	std::string log_str;
	std::string m_results;
	int _run_ocr(cv::Mat post_code_line, std::string &results, OcrAlgorithm_config *pConfig);

	std::string format_results(std::string res_str);

};


struct DigitClassRes
{
	int class_ind;
	float confidence;
};





class OCRHandWriteBox
{
public:
	OCRHandWriteBox();
	int initial_model(const wchar_t* model_file,float thresh_conf= 0.9, size_t cuda_id =0);

	//正常函数
	std::string get_postcode_string(cv::Mat tag_mat);

	std::string get_last_log();

	int find_key_words(cv::Mat tag_mat, std::vector<std::string>& key_words, void*pTess=NULL);
public:
	//用于检测“没有框”的手写数字
	std::string get_postcode_string_test(cv::Mat tag_mat);

private:
	int getPostCodeLine_nobox(const cv::Mat &srcMat, std::vector<cv::Mat> &toMats, std::vector<cv::Mat> &fromMats);
	int getHandWriteRange(const cv::Mat &srcMat, cv::Rect &srcRect, cv::Rect &dstRect, bool &need_rotate);
	int split_digits_nobox(cv::Mat &srcMat, std::vector<cv::Mat> &dstDigits);
	int score_for_rect(std::vector<cv::Rect> rcs, int im_width, int im_height, std::vector<float> &rc_scores);
	int getPostCode_nobox(std::vector<cv::Mat> &srcMat_vec, std::vector<std::string> &result_str, \
		std::vector<float> &confidence);

	//测试2
public:
	//用于识别没有“明显框”的手写数字
	std::string get_postcode_string_test_v2(const cv::Mat &tag_mat);


	int identify_handbox_type(const cv::Mat &srcm); //0，不是手写框，1，是不带边框，2是带边框


private:
	//获得五个手写小方格，包括外边框
	int get_handwrite_square_boxes(cv::Mat &src_mat, std::vector<cv::Mat> &square_boxes);//获得手写框，带边框
	
	//从方框中获得手写数字（去除黑边），白底黑字
	int remove_box_border(cv::Mat &src_mat);
	
	// onnxruntime 分类模型，白底，黑字
	std::string ocr_with_classifier(std::vector<cv::Mat> &srcms, float *conf=NULL);


	std::string ocr_with_tesseract(std::vector<cv::Mat> &srcms); //弃用

	float m_threshold_confidence;
	std::string log_str;

private:
	static const int MAX_IMAGE_NUM = 5;
	static const int WIDTH_ = 32;
	static const int HEIGHT_ = 32;
	static const int CHANNEL_ = 1;

	Ort::Env env;
	Ort::SessionOptions session_options;

	std::vector<const char*> input_node_names = { "input" };
	std::vector<const char*> output_node_names = { "output" };


	Ort::Value input_tensor_{ nullptr };
	std::array<int64_t, 4> input_shape_{ MAX_IMAGE_NUM, CHANNEL_, HEIGHT_, WIDTH_ };


	Ort::Session *m_pSession;
	std::array<float, MAX_IMAGE_NUM * WIDTH_ * HEIGHT_ * CHANNEL_> *input_image_ = NULL;



};




