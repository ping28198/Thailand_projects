
#pragma once
//#include "atlconv.h"
//#include "stdafx.h"
//#include "stdafx.h"
#include "HandwriteDigitsRecognition.h"
#include "yolo_v3.h"
#include <algorithm>
//#include "OcrAlgorithm.h"
//#include <iostream>
//#include <fstream>
//#include <opencv2/opencv.hpp>
//#include <opencv2/dnn.hpp>
//#include "tag_detect.h"
//#include "atlbase.h"
//#include "atlstr.h"
//#include "tchar.h"
#include <time.h>
//#include "math.h"
#include "CommonFunc.h"
//
int cutImageOnMouse();
int test_HDR_digits();
int detectAllImages();
int adJustBrightness(cv::Mat& src, double alpha, double beta, double anchor);
int makeBoarderConstant(cv::Mat &srcMat, unsigned char boarder_value, int boarder_width);
//int prepare_image_data(cv::Mat src_img, cv::Size input_shape, tensorflow::Tensor & dst_tensor);
int main() {
	
	//yolo3_tiny();
	//test();
	//WCHAR exeFullPath[MAX_PATH]; // Full path
	//string strPath = "";

	//GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	//USES_CONVERSION;
	//char *pexeFullp = CT2A(exeFullPath);
	//std::string imgNamestring = pexeFullp;

	test_HDR_digits();
	//cutImageOnMouse();



	//detectAllImages();

}

int test_HDR_digits()
{
	std::string dir = "F:/手写框/box/*.jpg";
	//string dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/Image[2019-8-2]/*.jpg";
	std::vector<std::string> imgfiles;
	CommonFunc::getAllFilesNameInDir(dir, imgfiles, false, true);
	Digits_HWR_CNN digit_cnn;
	int res = digit_cnn.initial("E:/python_projects/Digits_recog_cnn/HDRdigits_avg.pb");
	if (res==0)
	{
		std::cout << "initial error" << std::endl;
		return 0;
	}



	for (int i = 0; i < imgfiles.size(); i++)
	{
		cv::Mat srcm = cv::imread(imgfiles[i]);
		std::vector<cv::Mat> imgs;
		if (srcm.channels()==3)
		{
			cv::cvtColor(srcm, srcm, cv::COLOR_BGR2GRAY);
		}
		
		adJustBrightness(srcm, 10, 0, 40);
		imshow("ssrc", srcm);
		//cv::threshold(srcm, srcm, 40, 255,cv::THRESH_BINARY);
		srcm = ~srcm;
		makeBoarderConstant(srcm, 0, 1);

		imshow("srcm", srcm);
		imgs.push_back(srcm);
		std::vector<int> class_;
		std::vector<float> confd_;
		digit_cnn.detect_mat_avg(imgs, class_, confd_);
		for (int j=0;j<class_.size();j++)
		{
			std::cout << "数字：" << class_[j] << "@" << confd_[j] << std::endl;
		}











		cv::waitKey(0);
	}














	return 1;
}

void onMouse(int Event, int x, int y, int flags, void* param)
{
	cv::Mat srcimg = *(cv::Mat*)param; // 先转换类型，再取数据
	//Vec3b depth;    // 初始化不可放在case内
	cv::Rect rc;
	rc.width = 28;
	rc.height = 28;
	clock_t time_;
	std::string str_name;
	// 一般包含如下3个事件
	switch (Event)
	{
	case cv::EVENT_MOUSEMOVE:
		//cout << "Please select one point:" << endl;
		break;
	case cv::EVENT_LBUTTONDOWN: // 鼠标左键按下
		rc.x = x - 14;
		rc.y = y - 14;

		cv::imshow("digit", srcimg(rc));
		time_ = clock();
		str_name = std::to_string(time_);
		str_name = "F:/CommonDatasets/MNIST_ext/8_" + str_name + ".jpg";
		cv::imwrite(str_name, srcimg(rc));


		break;
	case cv::EVENT_LBUTTONUP: // 鼠标左键释放

		break;
	default:
		break;


	}
}

int cutImageOnMouse()
{
	std::string imgstr = "E:/cpp_projects/Thailand_projects/资源文件/手写数字2.jpg";
	cv::Mat srcMat = cv::imread(imgstr);
	if (srcMat.channels()==3)
	{
		cv::cvtColor(srcMat, srcMat, cv::COLOR_BGR2GRAY);
	}
	float scal_max1 = 1000.0 / std::max(srcMat.cols, srcMat.rows);


	cv::resize(srcMat, srcMat, cv::Size(),scal_max1,scal_max1,cv::INTER_AREA);
	adJustBrightness(srcMat, 4, 0, 110);

	srcMat = ~srcMat;
	
	cv::imshow("srcMat", srcMat);
	cv::setMouseCallback("srcMat", onMouse, &srcMat);
	cv::waitKey();




	return 1;
}

int adJustBrightness(cv::Mat& src, double alpha, double beta, double anchor)
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

int makeBoarderConstant(cv::Mat &srcMat, unsigned char boarder_value, int boarder_width)
{
	if (srcMat.empty()) return 0;
	int w = srcMat.cols;
	int h = srcMat.rows;
	for (int i = 0;i<boarder_width;i++)
	{
		line(srcMat, cv::Point(i, i), cv::Point(i, h - i-1), 
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

int detectAllImages()
{
	//std::string img_dir = "F:/cpte_datasets/Tailand_tag_detection_datasets/tag_obj_datasets_2/*.jpg";
	std::string img_dir = "f:/shared_data/*.jpg";
	std::vector<std::string> img_paths;
	CommonFunc::getAllFilesNameInDir(img_dir, img_paths, false, true);
	
	std::string model_path = "D:/yolo3_tiny.pb";
	YOLO_V3 yolo;
	yolo.initial(model_path, 0.25, 2);

	//tag_detector detector;
	//int a = detector.initial(model_path, 0.25, 2);
	//img_paths.push_back("d:/t.jpg");
	//img_paths.push_back("f:/shared_data/1563349578808956.jpg");
	//string tes = "f:/shared_data/1563349578808956.jpg";
	//cv::Mat cvt = cv::imread(tes);
	//cv::Mat bicubic;
	//cv::resize(cvt, bicubic, cv::Size(544, 544), 0, 0, cv::INTER_CUBIC);
	//cv::imwrite("d:/cvp_bicubic.jpg", bicubic);
	//cv::resize(cvt, bicubic, cv::Size(544, 544), 0, 0, cv::INTER_AREA);
	//cv::imwrite("d:/cvp_AREA.jpg", bicubic);
	//cv::resize(cvt, bicubic, cv::Size(544, 544), 0, 0, cv::INTER_LINEAR);
	//cv::imwrite("d:/cvp_biLINER.jpg", bicubic);
	//cv::resize(cvt, bicubic, cv::Size(544, 544), 0, 0, cv::INTER_NEAREST);
	//cv::imwrite("d:/cvp_NEAREST.jpg", bicubic);


	clock_t start_t, end_t;
	double ave_timeconsume = 0;
	for (int i=0;i<img_paths.size();i++)
	{
		
		std::vector<cv::Rect> boxes;
		std::vector<int> cs;
		std::vector<float> ss;
		std::cout << img_paths[i] << std::endl;
		cv::Mat srcm = cv::imread(img_paths[i]);
		start_t = clock();
		yolo.detect_mat(srcm, boxes, cs, ss);
		//yolo.detect_image(img_paths[i], boxes, cs, ss);
		end_t = clock();
		double timeconsume = (double)(end_t - start_t) / CLOCKS_PER_SEC;
		
		printf("检测到box数量：%d,耗时:%fs ",boxes.size(), timeconsume, ave_timeconsume/(i+1));
		if (i!=0)
		{
			ave_timeconsume += timeconsume;
			printf("平均耗时:%fs\n",ave_timeconsume / i);
		}

		for (int j=0;j<boxes.size();j++)
		{
			cv::Scalar clor;
			if (cs[j] == 0)
			{
				clor = cv::Scalar(255, 0, 0);
			}
			else
			{
				clor = cv::Scalar(0, 255, 0);
			}
			cv::rectangle(srcm, boxes[j], clor, 3);
			//std::sprintf(score_str, "%d:%f", detected_class[i], detected_scores[i]);
			//cv::putText(srcm, ss[i], cv::Point(s_rec.x, s_rec.y), 1, 1, clor);
		
		}
		cv::resize(srcm, srcm, cv::Size(), 0.2, 0.2);
		cv::imshow("jieguo", srcm);
		cv::waitKey(0);
	}




	return 1;
}

//
//int test()
//{
//	Eigen::Tensor<float, 1, 1> a(12);
//	a.setValues({ 1,2,3,4,5,6,7,8,9,10,11,12 });
//	Eigen::array<Eigen::Index, 3> dim1{ 1,2,6 };
//	Eigen::Tensor<float, 3, 1> b = a.reshape(dim1);
//	cout << b << endl<<endl;
//	for (int i=0;i<2;i++)
//	{
//		for (int j=0;j<6;j++)
//		{
//			cout << b(0, i, j)<<" ";
//		}
//		cout << endl;
//	}
//
//
//
//	Eigen::array<Eigen::Index, 4> dim2{ 1,12,1,1 };
//	Eigen::Tensor<float, 4, 1> c = b.reshape(dim2);
//	cout << c << endl<<endl;
//
//
//	for (int i = 0; i < 2; i++)
//	{
//		for (int j = 0; j < 2; j++)
//		{
//			for (int n = 0; n < 3; n++)
//			{
//				cout << c(0, i, j, n) << " ";
//			}
//			cout << ";";
//		}
//		cout << endl;
//	}
//
//
//
//
//	Eigen::array<Eigen::Index, 4> resape{ 1,1,12,1 };
//	Eigen::Tensor<float, 4, 1> d = c.reshape(resape);
//	cout << d << endl<<endl;
//
//	Eigen::Tensor<float, 4, 1> e = d.concatenate(c, 3);
//
//	cout << "dims:" << e.dimension(0) << ":" << e.dimension(1) << ":" << e.dimension(2) << ":" << e.dimension(3);
//
//
//	return 1;
//}
//int prepare_image_data(cv::Mat src_img,cv::Size input_shape, tensorflow::Tensor & dst_tensor)
//{
//	if (src_img.empty()) return 0;
//	if (src_img.channels() == 1)
//	{
//		cv::cvtColor(src_img, src_img, cv::COLOR_GRAY2BGR);//输入图像为灰度，
//	}
//	int channels = 3;
//	cv::cvtColor(src_img, src_img, cv::COLOR_BGR2RGB);//输入图像为彩色
//	cv::Mat resized_img;//输入模型的图像数据
//	resize_image_pad(src_img, input_shape, resized_img);
//	//imshow("resized", resized_img);
//	//int channels_ = resized_img.channels();
//
//	//准备数据
//	//tensorflow::Tensor input_tensor(DT_FLOAT, TensorShape({ 1, input_height, input_width, channels }));
//	const uint8* source_data = resized_img.data;
//	auto input_tensor_mapped = dst_tensor.tensor<float, 4>();
//
//	for (int i = 0; i < input_shape.width; i++) {
//		const uint8* source_row = source_data + (i * input_shape.width * channels);
//		for (int j = 0; j < input_shape.height; j++) {
//			const uint8* source_pixel = source_row + (j * channels);
//			for (int c = 0; c < channels; c++) {
//				const uint8* source_value = source_pixel + c;
//				input_tensor_mapped(0, i, j, c) = ((float)(*source_value)) / 255.0;
//			}
//		}
//	}
//	return 1;
//
//}
//
//
//int yolo3_tiny(std::string image_path)
//{
//	const int input_height = 544;
//	const int input_width = 544;
//	const int channels = 3;
//	const int max_box = 1;//每种类别的最大box数量
//	const float score_threshold = 0.3;
//	const float iou_threshold = 0.45;
//	std::vector<cv::Size> anchors;
//	anchors.push_back(cv::Size(106, 107));
//	anchors.push_back(cv::Size(176, 221));
//	anchors.push_back(cv::Size(344, 319));
//
//	anchors.push_back(cv::Size(13, 18));
//	anchors.push_back(cv::Size(30, 35)); 
//	anchors.push_back(cv::Size(48, 76));
//
//
//	const std::string model_path = "D:/yolo3_tiny.pb";// tensorflow模型文件，注意不能含有中文
//	//const std::string image_path = "F:/cpte_datasets/Tailand_tag_detection_datasets/tag_obj_datasets/15633496068047075.jpg";    // 待inference的图片grace_hopper.jpg
//	cv::Mat img = cv::imread(image_path);
//	
//	//std::ofstream file3("d:/x.txt", std::ios::app | std::ios::out); //以输出方式打开文件
//	//if (!file3)//或者写成myfile.fail() 
//	//{
//	//	std::cout << "文件读取失败!\n" << std::endl;
//	//}
//	//else
//	//{
//	//	file3 << input_tensor_mapped << endl;
//	//	file3.flush();
//	//	file3.close();
//	//}
//	cv::Size input_shape(544, 544);
//	if (img.channels() == 1)
//	{
//		cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);//输入图像为灰度，
//	}
//	cv::cvtColor(img, img, cv::COLOR_BGR2RGB);//输入图像为彩色
//	cv::Mat resized_img;//输入模型的图像数据
//	resize_image_pad(img, input_shape, resized_img);
//	//imshow("resized", resized_img);
//	//int channels_ = resized_img.channels();
//	//准备数据
//	tensorflow::Tensor input_tensor(DT_FLOAT, TensorShape({ 1, input_height, input_width, channels }));
//	const uint8* source_data = resized_img.data;
//	auto input_tensor_mapped = input_tensor.tensor<float, 4>();
//
//	for (int i = 0; i < input_shape.width; i++) {
//		const uint8* source_row = source_data + (i * input_shape.width * channels);
//		for (int j = 0; j < input_shape.height; j++) {
//			const uint8* source_pixel = source_row + (j * channels);
//			for (int c = 0; c < channels; c++) {
//				const uint8* source_value = source_pixel + c;
//				input_tensor_mapped(0, i, j, c) = ((float)(*source_value)) / 255.0;
//			}
//		}
//	}
//
//	Session* session;
//	Status status = NewSession(SessionOptions(), &session);
//	if (!status.ok()) {
//		std::cerr << status.ToString() << endl;
//		return -1;
//	}
//	else {
//		cout << "Session created successfully" << endl;
//	}
//	// 读取二进制的模型文件到graph中
//	tensorflow::GraphDef graph_def;
//	status = ReadBinaryProto(Env::Default(), model_path, &graph_def);
//	if (!status.ok()) {
//		std::cerr << status.ToString() << endl;
//		return -1;
//	}
//	else {
//		cout << "Load graph protobuf successfully" << endl;
//	}
//
//	// 将graph加载到session
//	status = session->Create(graph_def);
//	if (!status.ok()) {
//		std::cerr << status.ToString() << endl;
//		return -1;
//	}
//	else {
//		cout << "Add graph to session successfully" << endl;
//	}
//	// 输入inputs，“x_input”是我在模型中定义的输入数据名称
//	std::vector<std::pair<std::string, tensorflow::Tensor>> inputs = {
//		{ "input_1", input_tensor },
//	};
//	// 输出outputs
//	std::vector<tensorflow::Tensor> outputs;
//
//	//批处理识别
//	double start = clock();
//	std::vector<std::string> output_nodes;
//	output_nodes.push_back("conv2d_10/BiasAdd");
//	output_nodes.push_back("conv2d_13/BiasAdd");
//	// 运行会话，最终结果保存在outputs中
//	status = session->Run(inputs, { output_nodes }, {}, &outputs );
//	if (!status.ok()) {
//		std::cerr << status.ToString() << endl;
//		return -1;
//	}
//	else {
//		cout << "Run session successfully" << endl;
//	}
//	std::vector<cv::Rect> box_Rect;
//	std::vector<float> box_socre;
//	std::vector<int> box_class;
//	yolo_eval(outputs,anchors,2,Size(input_width, input_height),
//		max_box, score_threshold,iou_threshold, box_Rect, box_socre, box_class);
//
//	//tensorflow::Tensor t = outputs[0];                   // Fetch the first tensor
//	//int ndim2 = t.shape().dims();             // Get the dimension of the tensor
//	//auto tmap = t.tensor<float, 4>();        // Tensor Shape: [batch_size, target_class_num]
//	//int output_dim = t.shape().dim_size(1);  // Get the target_class_num from 1st dimension
//
//	//printf("outputs_size:%d, output0_shape_dimes:%d, output_dim:%d\n", outputs.size(),ndim2,output_dim);
//	char score_str[32] = { 0 };
//	for (int i=0;i<box_Rect.size();i++)
//	{
//		cv::Rect s_rec = box_Rect[i];
//		cv::Rect img_rect(0, 0, input_width, input_height);
//		
//		if (crop_rect(img_rect,s_rec))
//		{
//			cv::rectangle(resized_img, s_rec, cv::Scalar(255, 0, 0));
//			std::sprintf(score_str, "%f", box_socre[i]);
//			cv::putText(resized_img, score_str, cv::Point(s_rec.x, s_rec.y), 1, 1, cv::Scalar(255, 0, 0));
//		}
//		else
//		{
//			printf("超出图像范围\n");
//		}
//		
//	}
//	imshow("box",resized_img);
//
//
//
//	printf("detected box num:%d\n", box_socre.size());
//
//
//	waitKey(0);
//
//
//	return 0;
//}
//int yolo_eval(std::vector<tensorflow::Tensor> inputs, std::vector<cv::Size> anchors, int num_class,
//	cv::Size image_shape, size_t max_box, float score_threshold, float iou_threshold,
//	std::vector<cv::Rect> &box_detected, std::vector<float> &box_scores, std::vector<int> &class_detected)
//{
//	int num_layers = inputs.size();
//	std::vector<Eigen::Tensor<float, 2, 1>> boxes_vec, scores_vec;
//	for (size_t i = 0; i < num_layers; i++)
//	{
//		std::vector<cv::Size> anc_l;
//		int ind_s = i * (anchors.size() / num_layers);
//		int ind_e = (i + 1) * (anchors.size() / num_layers);
//		anc_l.assign(anchors.begin() + ind_s, anchors.begin() + ind_e);
//
//		yolo_eval_layer(inputs[i], anc_l, num_class,
//			image_shape, boxes_vec,scores_vec);
//	}
//	//Eigen::Tensor<float, 2, 1> all_boxes , all_scores;
//	Eigen::Tensor<float, 2, 1> all_boxes = boxes_vec[0].concatenate(boxes_vec[1], 0);
//	Eigen::Tensor<float, 2, 1> all_scores = scores_vec[0].concatenate(scores_vec[1], 0);
//	//if (num_layers==1)
//	//{
//	//	all_boxes = boxes_vec[0];
//	//	all_scores = scores_vec[0];
//	//}
//	//else if (num_layers == 2)
//	//{
//	//	all_boxes = boxes_vec[0].concatenate(boxes_vec[1],0);
//	//	all_scores = scores_vec[0].concatenate(scores_vec[1], 0);
//	//}
//	//else if(num_layers ==3)
//	//{
//	//	Eigen::Tensor<float, 2, 1> box_1 = (boxes_vec[0].concatenate(boxes_vec[1], 0));
//	//	all_boxes = box_1.concatenate(boxes_vec[2], 0);
//	//	Eigen::Tensor<float, 2, 1> scores_1 = scores_vec[0].concatenate(scores_vec[1], 0);
//	//	all_scores = scores_1.concatenate(scores_vec[2], 0);
//	//}
//
//	//cout<< scores_vec[1] << endl;
//
//	//Eigen::Tensor<bool, 2, 1> mask_t = all_scores >= score_threshold;
//	//Eigen::array<int, 1> _dims{1};
//	//Eigen::Tensor<float, 1, 1> max_op = all_scores.maximum(_dims);
//	//auto mask_box_t = mask_t.reduce(1, max_op);
//
//	//将box 转换为像素级
//	assert(all_boxes.dimension(0) != all_scores.dimension(0));
//	//size_t num_abox = all_boxes.dimension(0);
//	//Eigen::Tensor<float, 2, 1> box_size_t(1, 4);
//	//box_size_t(1, 0) = (float)image_shape.width;
//	//box_size_t(1, 1) = (float)image_shape.height;
//	//box_size_t(1, 2) = (float)image_shape.width;
//	//box_size_t(1, 3) = (float)image_shape.height;
//	//Eigen::array<Eigen::Index, 2> broad{ num_abox,1 };
//	Eigen::Tensor<float, 2, 1> all_boxes_int = all_boxes;
//
//
//	int num_abox = all_boxes.dimension(0);
//	//提取置信度大于threshold的box
//	std::vector<std::vector<cv::Rect>> candi_box_vec;
//	std::vector < std::vector<float>> candi_socres_vec;
//	std::vector < std::vector<int>> candi_class_vec;
//	for (size_t i=0;i<num_class;i++)
//	{
//		//Eigen::array<Eigen::Index, 2> slice_start{0,i};
//		//Eigen::array<Eigen::Index, 2> slice_end{num_abox,1};
//		//Eigen::Tensor<float, 2, 1> sliced_t = all_scores.slice(slice_start, slice_end);
//		std::vector<cv::Rect> b_vec;
//		std::vector<float> s_vec;
//		std::vector<int> c_vec;
//		cv::Rect r;
//		for (size_t j = 0;j<num_abox;j++)
//		{
//			if (all_scores(j,i) > score_threshold)
//			{
//				//*2的原因是目前未知，否则box的宽高不对，可能和原keras-yolo作者的的处理有关
//				r.x = round(all_boxes_int(j, 0)- all_boxes_int(j, 2));
//				r.y = round(all_boxes_int(j, 1)- all_boxes_int(j, 3));
//				r.width = round(all_boxes_int(j,2) * 2.0);
//				r.height = round(all_boxes_int(j, 3) * 2.0);
//				b_vec.push_back(r);
//				s_vec.push_back(all_scores(j, i));
//				c_vec.push_back(i);
//			}
//			int bb = 0;
//		}
//		candi_box_vec.push_back(b_vec);
//		candi_socres_vec.push_back(s_vec);
//		candi_class_vec.push_back(c_vec);
//	}
//
//
//
//	for (int i=0;i<candi_box_vec.size();i++)
//	{
//		cv::dnn::experimental_dnn_34_v7::MatShape index_res;
//
//		cv::dnn::NMSBoxes(candi_box_vec[i], candi_socres_vec[i], score_threshold, iou_threshold, index_res, 1.0, max_box);
//
//		for (size_t j = 0; j < index_res.size(); j++)
//		{
//			int ind = index_res[j];
//			box_detected.push_back(candi_box_vec[i][ind]);
//			box_scores.push_back(candi_socres_vec[i][ind]);
//			class_detected.push_back(candi_class_vec[i][ind]);
//		}
//	}
//	//cv::dnn::NMSBoxes(,)
//	return 1;
//}
//
//int yolo_eval_layer(tensorflow::Tensor input_t, std::vector<cv::Size> anchors, int num_class,
//	cv::Size input_shape, std::vector<Eigen::Tensor<float, 2, 1>> &boxes, std::vector<Eigen::Tensor<float, 2, 1>> &scores)
//{
//	long grid_x = input_t.shape().dim_size(1);
//	long grid_y = input_t.shape().dim_size(2);
//	long num_anchors = anchors.size();
//	long dims = input_t.shape().dims();
//	long channels = input_t.shape().dim_size(0);
//	long feature_length = input_t.shape().dim_size(3);
//	long all_dims = 1;
//	for (int i = 0; i < dims; i++)
//	{
//		all_dims *= input_t.shape().dim_size(i);
//	}
//	long dim_1 = all_dims / (grid_x*grid_y*(num_class + 5)*num_anchors);
//	//Eigen::TensorMap<Eigen::Tensor<float, 5, 1, long long>, 16> t_map_5d();
//	auto input_tensor_mapped = input_t.tensor<float, 4>();
//	//Eigen::Tensor<float,4> t_5d(dim_1, grid_x, grid_y, num_anchors*(num_class + 5));
//	Eigen::Tensor<float, 4, Eigen::RowMajor> src_tensor(channels, grid_x, grid_y, feature_length);
//	for (long i = 0; i < channels; i++)
//	{
//		for (long j=0;j<grid_x;j++)
//		{
//			for (long m=0;m<grid_y;m++)
//			{
//				for (long n=0;n< feature_length;n++)
//				{
//					src_tensor(i, j, m, n) = input_tensor_mapped(i, j, m, n);
//				}
//			}
//		}
//
//	}
//	//std::cout << "src_tensor" << endl;
//	//std::cout << src_tensor(0,0,0,0,-1) << endl;
//	std::array<Eigen::Index, 5> reshape_dims{ dim_1, grid_x, grid_y, num_anchors,num_class + 5 };
//	Eigen::Tensor<float, 5, Eigen::RowMajor> reshape_t = src_tensor.reshape(reshape_dims);
//	
//	//修正confidence
//	std::array<Eigen::Index, 5> slice_start{ 0 , 0, 0, 0, 4 };
//	std::array<Eigen::Index, 5> slice_end{ dim_1, grid_x, grid_y, num_anchors, 1 };
//	Eigen::Tensor<float, 5, 1> slice_t = reshape_t.slice(slice_start, slice_end);
//	Eigen::Tensor<float, 5, 1> confidence_t = slice_t.sigmoid();
//
//
//
//
//
//	//修正宽高
//	Eigen::Tensor<float, 5, 1> anc( 1, 1, 1, num_anchors, 2 );
//	for (int i=0;i<num_anchors;i++)
//	{
//		anc(0, 0, 0, i, 0) = (float)anchors[i].width;
//		anc(0, 0, 0, i, 1) = (float)anchors[i].height;
//	}
//	//cout << anc << endl;
//	std::array<Eigen::Index, 5> bcast{ dim_1, grid_x, grid_y ,1,1};
//	Eigen::Tensor<float, 5, 1> wh_anchor_t = anc.broadcast(bcast);
//	//Eigen::Tensor<float, 5, 1> anc_1(1,1,1,1,2);
//	//anc1(0,0,0,0,0) 
//	slice_start[4] = 2;
//	slice_end[4] = 2;
//	Eigen::Tensor<float, 5, 1> slice_wh_t = reshape_t.slice(slice_start, slice_end);
//	Eigen::Tensor<float, 5, 1> wh_t = slice_wh_t.exp()*wh_anchor_t;
//
//
//
//	//修正位置
//	std::array<Eigen::Index, 5> bcast2{1,1,1,num_anchors,1};
//	Eigen::Tensor<float, 5, 1> an_x(1, grid_x, 1, 1, 1);
//	for (int i=0;i<grid_x;i++)
//	{
//		an_x(0, i, 0, 0, 0) = (float)i;
//	}
//	bcast[0] = 1;bcast[1] = 1; bcast[2] = grid_y; bcast[3] = 1; bcast[4] = 1;
//	//bcast2[0] = 1; bcast2[1] = 1; bcast2[2] = 1; bcast2[3] = num_anchors; bcast2[4] = 1;
//	Eigen::Tensor<float, 5, 1> xy_x_t = an_x.broadcast(bcast);
//	auto xy_x_t_1 = xy_x_t.broadcast(bcast2);
//	
//	Eigen::Tensor<float, 5, 1> an_y(1, 1, grid_y, 1, 1);
//	for (int i = 0; i < grid_y; i++)
//	{
//		an_y(0, 0, i, 0, 0) = (float)i;
//	}
//	bcast[1] = grid_x;
//	bcast[2] = 1;
//	Eigen::Tensor<float, 5, 1> xy_y_t = an_y.broadcast(bcast);
//	auto xy_y_t_1 = xy_y_t.broadcast(bcast2);
//	Eigen::Tensor<float, 5, 1> con_xy_grid = xy_y_t_1.concatenate(xy_x_t_1,4);
//
//	slice_start[4] = 0;
//	slice_end[4] = 2;
//	auto slice_xy_t = reshape_t.slice(slice_start, slice_end);
//	auto xy_sigmoid_t = slice_xy_t.sigmoid();
//	Eigen::Tensor<float, 5, 1> xy_add = xy_sigmoid_t + con_xy_grid;
//
//	Eigen::Tensor<float, 5, 1> grid_xy(1, 1, 1, 1, 2);
//	grid_xy(0, 0, 0, 0, 0) = grid_x;
//	grid_xy(0, 0, 0, 0, 1) = grid_y;
//	bcast[0] = dim_1; bcast[1] = grid_x; bcast[2] = grid_y; bcast[3] = num_anchors; bcast[4] = 1;
//	Eigen::Tensor<float, 5, 1> grid_xy_bc = grid_xy.broadcast(bcast);
//	auto xy_div = xy_add / grid_xy_bc;
//	//图像宽高
//
//	grid_xy(0, 0, 0, 0, 0) = (float)input_shape.width;
//	grid_xy(0, 0, 0, 0, 1) = (float)input_shape.height;
//	Eigen::Tensor<float, 5, 1> img_wh_bc = grid_xy.broadcast(bcast);
//
//	Eigen::Tensor<float, 5, 1> xy_t = xy_div * img_wh_bc;
//
//
//	//获得class类别
//	slice_start[4] = 5;
//	slice_end[4] = num_class;
//	Eigen::Tensor<float, 5, 1> class_t = reshape_t.slice(slice_start, slice_end).sigmoid();
//	
//
//	//展开矩阵
//	//box
//	Eigen::Tensor<float, 5, 1> box_xy_wh = xy_t.concatenate(wh_t, 4);
//	std::array<Eigen::Index, 2> shape_2{ dim_1*grid_x*grid_y*num_anchors,4};
//	Eigen::Tensor<float, 2, 1> box_xy_wh_1 = box_xy_wh.reshape(shape_2);
//
//	bcast[0] = 1; bcast[1] = 1; bcast[2] = 1; bcast[3] = 1; bcast[4] = num_class;
//	shape_2[1] = num_class;
//	Eigen::Tensor<float, 2, 1> scores_1 = (confidence_t.broadcast(bcast) * class_t).reshape(shape_2);
//
//	//////////////////////////////////////////////////////////////////////////
//	//调试用
//	//std::cout << xy_t.dimension(0) << ":";
//	//std::cout << xy_t.dimension(1) << ":";
//	//std::cout << xy_t.dimension(2) << ":";
//	//std::cout << xy_t.dimension(3) << ":";
//	//std::cout << xy_t.dimension(4) << ":" << endl;
//	//std::array<Eigen::Index, 2> test_arr{ dim_1*grid_x*grid_y*num_anchors,2 };
//	//Eigen::Tensor<float, 2, 1> test_t = xy_t.reshape(test_arr);
//	//std::string f;
//	//if (grid_y == 17)
//	//{
//	//	f = "d:/x1.txt";
//	//}
//	//else
//	//{
//	//	f = "d:/x2.txt";
//	//}
//	//std::ofstream testfile(f.c_str(), std::ios::out);
//	//if (testfile)
//	//{
//	//	testfile << test_t << std::endl;
//	//	testfile.flush();
//	//	testfile.close();
//	//}
//	//else
//	//{
//	//	std::cout << "文件打开失败" << std::endl;
//	//}
//
//	boxes.push_back(box_xy_wh_1);
//	scores.push_back(scores_1);
//	return 1;
//
//
//
//}
//
//
//float sigmoid(float i)
//{
//	return 1.0 / (1 + exp(-i));
//}
//
//int crop_rect(cv::Rect main_rect, cv::Rect &to_crop_rect)
//{
//	if (to_crop_rect.x + to_crop_rect.width <= main_rect.x) return 0;
//	if (to_crop_rect.y + to_crop_rect.height <= main_rect.y) return 0;
//	if (to_crop_rect.x >= main_rect.width + main_rect.x) return 0;
//	if (to_crop_rect.y >= main_rect.height + main_rect.y) return 0;
//	if (main_rect.x > to_crop_rect.x) to_crop_rect.x = main_rect.x;
//	if (main_rect.y > to_crop_rect.y) to_crop_rect.y = main_rect.y;
//	if (main_rect.x+main_rect.width < to_crop_rect.x + to_crop_rect.width)
//	{
//		to_crop_rect.width = main_rect.x + main_rect.width - to_crop_rect.x;
//	}
//	if (main_rect.y + main_rect.height < to_crop_rect.y + to_crop_rect.height)
//	{
//		to_crop_rect.height = main_rect.y + main_rect.height - to_crop_rect.y;
//	}
//	return 1;
//}
//
//int resize_image_pad(cv::Mat srcimg,cv::Size msize,cv::Mat &dstimg)
//{
//	int input_height = msize.height;
//	int input_width = msize.width;
//	cv::Mat resied_m = cv::Mat(msize, srcimg.type(),cv::Scalar(128,128,128));
//	double fscal_y = double(input_height) / srcimg.rows;
//	double fscal_x = double(input_width) / srcimg.cols;
//	double fscal = min(fscal_x, fscal_y);
//	cv::Size rsize;
//	rsize.height = fscal * srcimg.rows;
//	rsize.width = fscal * srcimg.cols;
//	cv::Mat resize_src;
//	cv::resize(srcimg, resize_src, rsize);
//
//	cv::Rect imRect;
//	imRect.x = (input_width - resize_src.cols) / 2;
//	imRect.y = (input_height - resize_src.rows) / 2;
//	imRect.width = resize_src.cols;
//	imRect.height = resize_src.rows;
//	//imRect.x = (imRect.x < 0) ? 0 : imRect.x;
//	//imRect.y = (imRect.y < 0) ? 0 : imRect.y;
//	//imRect.width = (imRect.width > input_width - imRect.x) ? input_width - imRect.x : imRect.width;
//	//imRect.height = (imRect.height > input_height - imRect.y) ? input_height - imRect.y : imRect.height;
//	resize_src.copyTo(resied_m(imRect));
//	dstimg = resied_m;
//	return 1;
//}
//
//int test_tensorflow()
//{
//	const std::string model_path = "D:/Program_files/tensorflow_cpp/test/frozen_inference_graph.pb";// tensorflow模型文件，注意不能含有中文
//	const std::string image_path = "D:/Program_files/tensorflow_cpp/test/image1.jpg";    // 待inference的图片grace_hopper.jpg
//
//	// 设置输入图像
//	cv::Mat img = cv::imread(image_path);
//	cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
//	int height = img.rows;
//	int width = img.cols;
//	int depth = img.channels();
//
//	// 取图像数据，赋给tensorflow支持的Tensor变量中
//	tensorflow::Tensor input_tensor(DT_UINT8, TensorShape({ 1, height, width, depth }));
//	const uint8* source_data = img.data;
//	auto input_tensor_mapped = input_tensor.tensor<uint8, 4>();
//
//	for (int i = 0; i < height; i++) {
//		const uint8* source_row = source_data + (i * width * depth);
//		for (int j = 0; j < width; j++) {
//			const uint8* source_pixel = source_row + (j * depth);
//			for (int c = 0; c < depth; c++) {
//				const uint8* source_value = source_pixel + c;
//				input_tensor_mapped(0, i, j, c) = *source_value;
//			}
//		}
//	}
//
//	// 初始化tensorflow session
//	Session* session;
//	Status status = NewSession(SessionOptions(), &session);
//	if (!status.ok()) {
//		std::cerr << status.ToString() << endl;
//		return -1;
//	}
//	else {
//		cout << "Session created successfully" << endl;
//	}
//
//	// 读取二进制的模型文件到graph中
//	tensorflow::GraphDef graph_def;
//	status = ReadBinaryProto(Env::Default(), model_path, &graph_def);
//	if (!status.ok()) {
//		std::cerr << status.ToString() << endl;
//		return -1;
//	}
//	else {
//		cout << "Load graph protobuf successfully" << endl;
//	}
//
//	// 将graph加载到session
//	status = session->Create(graph_def);
//	if (!status.ok()) {
//		std::cerr << status.ToString() << endl;
//		return -1;
//	}
//	else {
//		cout << "Add graph to session successfully" << endl;
//	}
//	// 输入inputs，“x_input”是我在模型中定义的输入数据名称
//	std::vector<std::pair<std::string, tensorflow::Tensor>> inputs = {
//		{ "image_tensor:0", input_tensor },
//	};
//
//	// 输出outputs
//	std::vector<tensorflow::Tensor> outputs;
//
//	//批处理识别
//	double start = clock();
//	std::vector<std::string> output_nodes;
//	output_nodes.push_back("num_detections");
//	output_nodes.push_back("detection_boxes");
//	output_nodes.push_back("detection_scores");
//	output_nodes.push_back("detection_classes");
//	// 运行会话，最终结果保存在outputs中
//	status = session->Run(inputs, { output_nodes }, {}, &outputs);
//	if (!status.ok()) {
//		std::cerr << status.ToString() << endl;
//		return -1;
//	}
//	else {
//		cout << "Run session successfully" << endl;
//	}
//
//	double	finish = clock();
//	double duration = (double)(finish - start) / CLOCKS_PER_SEC;
//	cout << "spend time:" << duration << endl;
//	cv::imshow("image", img);
//	cv::waitKey();
//	return 0;
//}