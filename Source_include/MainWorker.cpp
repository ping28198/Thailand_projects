
#include <algorithm>
#include "MainWorker.h"
#include "CPTENetworkServer.h"
#include "PostcodeAlgorithm_v1.h"
#include "cc_util.hpp"
#include "cstring"
#include "stl/time.hpp"
#include "stl/stringhelper.hpp"
#include "CommonFunc.h"
#include "CutParcelBox_thp.h"
#include <fstream>
#include <map>

#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h" // for loading levels from the environment variable
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

//auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/daily.txt", 2, 30);
auto logger = spdlog::rotating_logger_mt("main", CommonFunc::get_exe_dir() + "/log/rotating.txt", 1048576 * 5, 30);

//auto logger = spdlog::stdout_color_mt("console");










float format_angle_rad(const float angle)
{
	float tmp = angle;
	while (tmp < 0)
	{
		tmp += 2 * CV_PI;
	}
	while (tmp> 2*CV_PI)
	{
		tmp -= 2 * CV_PI;
	}
	return tmp;
}


OCR_MainWoker::OCR_MainWoker()
{

}

OCR_MainWoker::~OCR_MainWoker()
{
	logger->error("Main worker destroyed, program exited!");
	if (pClient) delete pClient;
	for (int i=0;i<m_pThreads.size();i++)
	{
		delete m_pThreads[i];
	}
}

int OCR_MainWoker::load_parameter()
{
	std::string exedir = CommonFunc::get_exe_dir();
	std::map<std::string, std::string> parameters_pair;

	std::ifstream ifs;
	ifs.open(exedir + "\\system.ini",std::ios::in);
	if (ifs.is_open())
	{
		while (ifs.peek() != EOF)
		{
			char tmpstr[512] = { 0 };
			ifs.getline(tmpstr, 511);
			std::string kstr(tmpstr);
			if (!kstr.empty())
			{
				
				size_t pos = kstr.find_first_of("#;");
				if (pos!=std::string::npos)
				{
					kstr = kstr.substr(0, pos);
				}
				if (kstr.empty()) continue;
				pos = kstr.find('=');
				if (pos != std::string::npos)
				{
					std::string key_ = kstr.substr(0, pos);
					std::string value_ = kstr.substr(pos + 1);
					stl::stringhelper::trim(key_);
					stl::stringhelper::trim(value_);
					parameters_pair.insert(std::map<std::string, std::string>::value_type(key_, value_));
				}
			}
		}
		ifs.close();
	}
	std::map<std::string, std::string>::iterator pos;

	pos = parameters_pair.find("ImageServerIP");
	if (pos != parameters_pair.end())
	{
		m_param.image_server_ip = pos->second;
	}

	pos = parameters_pair.find("ImageServerPort");
	if (pos != parameters_pair.end())
	{
		m_param.image_server_port = std::atol(pos->second.c_str());
	}
	
	pos = parameters_pair.find("TagDetectConfidence");
	if (pos != parameters_pair.end())
	{
		m_param.tag_detect_confidence = std::atof(pos->second.c_str());
	}

	pos = parameters_pair.find("OCRID");
	if (pos != parameters_pair.end())
	{
		m_param.ocr_id = std::atoi(pos->second.c_str());
	}

	pos = parameters_pair.find("StdTagSupport");
	if (pos != parameters_pair.end())
	{
		m_param.is_ocr_standard_tag = std::atoi(pos->second.c_str());
	}

	pos = parameters_pair.find("WinTagSupport");
	if (pos != parameters_pair.end())
	{
		m_param.is_ocr_window_tag = std::atoi(pos->second.c_str());
	}

	pos = parameters_pair.find("ArbTagSupport");
	if (pos != parameters_pair.end())
	{
		m_param.is_ocr_arb_tag = std::atoi(pos->second.c_str());
	}

	pos = parameters_pair.find("HwtTagSupport");
	if (pos != parameters_pair.end())
	{
		m_param.is_ocr_handwrite = std::atoi(pos->second.c_str());
	}

	pos = parameters_pair.find("TrayPosLeft");
	if (pos != parameters_pair.end())
	{
		m_param.tray_lf = std::atof(pos->second.c_str());
	}

	pos = parameters_pair.find("TrayPosRight");
	if (pos != parameters_pair.end())
	{
		m_param.tray_rt = std::atof(pos->second.c_str());
	}

	pos = parameters_pair.find("TrayPixel");
	if (pos != parameters_pair.end())
	{
		m_param.tray_pix = std::atoi(pos->second.c_str());
	}

	pos = parameters_pair.find("Postcode10Support");
	if (pos != parameters_pair.end())
	{
		m_param.support_code_num_10 = std::atoi(pos->second.c_str());
	}

	pos = parameters_pair.find("TestMode");
	if (pos != parameters_pair.end())
	{
		m_param.is_test_mode = std::atoi(pos->second.c_str());
	}
	pos = parameters_pair.find("AddBottom");
	if (pos != parameters_pair.end())
	{
		m_param.support_bottom_view = std::atoi(pos->second.c_str());
	}
	pos = parameters_pair.find("LogLevel");
	if (pos != parameters_pair.end())
	{
		m_param.log_level = std::atoi(pos->second.c_str());
	}
	pos = parameters_pair.find("LocalPort");
	if (pos != parameters_pair.end())
	{
		m_param.local_port = std::atoi(pos->second.c_str());
	}

	return 1;
}

void OCR_MainWoker::initial()
{

	load_parameter(); //从配置文件加载参数


 	spdlog::flush_every(std::chrono::seconds(1));
// 	//启动线程
	
	logger->set_level(spdlog::level::debug);
	logger->info("system starting...");
	logger->info("OCR id: {}", m_param.ocr_id);
	logger->info("Image server ip: {}", m_param.image_server_ip);
	logger->info("Image server port: {}", m_param.image_server_port);
	logger->info("Local port:{}", m_param.local_port);
	logger->info("Standard tag support: {}", m_param.is_ocr_standard_tag);
	logger->info("Windows tag support: {}", m_param.is_ocr_window_tag);
	logger->info("Arb tag support: {}", m_param.is_ocr_arb_tag);
	logger->info("Handwrite box support: {}", m_param.is_ocr_handwrite);
	logger->info("Postcode length 10 support: {}", m_param.support_code_num_10);
	logger->info("Tag detect threshold confidence: {}", m_param.tag_detect_confidence);
	logger->info("tray left pos: {:.2f}, tray right pos: {:.2f}", m_param.tray_lf, m_param.tray_rt);
	logger->info("default tray pixel value: {}", m_param.tray_pix);
	logger->info("rigid level: {} ", m_param.rigid_level);
	logger->info("test mode: {} ", m_param.is_test_mode);
	logger->info("Bottom view image support: {} ", m_param.support_bottom_view);
	
	switch (m_param.log_level)
	{
	case 1:
		logger->set_level(spdlog::level::debug);
		logger->info("Log Level: debug");
		break;
	case 2:
		logger->set_level(spdlog::level::info);
		logger->info("Log Level: info");
		break;
	case 3:
		logger->set_level(spdlog::level::warn);
		logger->info("Log Level: warn");
		break;
	default:
		logger->info("Log Level: debug");
		break;
	}
	
	//发送消息线程
	std::thread *pthread = new std::thread(OCR_MainWoker::thread_send_msg, this);
	m_pThreads.push_back(pthread);

	//打包任务线程
	pthread = new std::thread(OCR_MainWoker::thread_package_tasks, this);
	m_pThreads.push_back(pthread);
	
	const int num_loader = 1;
	const int num_detector = 1;

	//加载图像线程
	for (int i=0;i<num_loader;i++)
	{
		pthread = new std::thread(OCR_MainWoker::thread_load_image, this);
		m_pThreads.push_back(pthread);
	}

	//切割包裹线程
	pthread = new std::thread(OCR_MainWoker::thread_cut_parcel, this);
	m_pThreads.push_back(pthread);

	//检测标签线程
	for (int i=0;i<num_detector;i++)
	{
		pthread = new std::thread(OCR_MainWoker::thread_detect_tags, this, i);
		m_pThreads.push_back(pthread);
	}
	
	//接受消息线程
	pthread = new std::thread(OCR_MainWoker::thread_recv_msg, this);
	m_pThreads.push_back(pthread);

	//ocr标准标签线程
	pthread = new std::thread(OCR_MainWoker::thread_ocr_std_tag, this);
	m_pThreads.push_back(pthread);

	//ocr任意标签线程
	if (m_param.is_ocr_arb_tag)
	{
		pthread = new std::thread(OCR_MainWoker::thread_ocr_arb_tag, this);
		m_pThreads.push_back(pthread);
	}

	//ocr手写框线程
	if (m_param.is_ocr_handwrite)
	{
		pthread = new std::thread(OCR_MainWoker::thread_ocr_hwrt_box, this);
		m_pThreads.push_back(pthread);
	}
	
	//网络服务
	pClient = new NetworkClient(m_param.image_server_ip, m_param.image_server_port,\
		m_param.local_port,":", "\r\n", CHINA_POST_PROTO);

	//状态检测线程
	pthread = new std::thread(OCR_MainWoker::thread_check_state, this);
	m_pThreads.push_back(pthread);

}

void OCR_MainWoker::thread_recv_msg(OCR_MainWoker* parent)
{
	ulong _task_id = 1;
	int ocr_id = parent->m_param.ocr_id;
	TaskData *ptask = new TaskData();


	while (true)
	{
		std::string msgdata;
		int res = 0;
		if (parent->pClient)
		{
			res = parent->pClient->recv_message(msgdata);
		}
		else
		{
			stl::time::sleep(10);
		}
		if(!res) continue;
		//std::cout << "get a msg" << std::endl;
		//logger->debug("recv a msg!");
		msgdata = NetworkClient::decode_data_hex(msgdata); //将16进制字符串转为数据
		if (!NetworkServer::check_msg_validation_china_post(msgdata)) //校验数据
		{
			//std::cout << "get a message, check invalidation" << std::endl;
			logger->debug("recv a msg, but check msg invalidation");
			continue;
		}
		char* pdata = (char*)msgdata.c_str();
		if (uchar(*pdata) != MSG_OCR_ADDR)
		{
			//std::cout << "get a message, but destination is no OCR!" << std::endl;
			logger->debug("recv a msg, but destination is not OCR");
			continue;
		}
		pdata += 1;

		if (uchar(*pdata) != MSG_IMG2OCR_REQ)
		{
			//std::cout << "get a message, function is invalidation" << std::endl;
			logger->debug("recv a msg, but function is invalidation");
			continue;
		}
		pdata += 1;

		//std::cout << "loading msg to task" << std::endl;
		logger->info("Recv a msg from imageserver, create a main task!");
		//创建任务
		
		for (int i=0;i<6;i++)
		{
			ptask->m_image_id[i] = (uchar)(*pdata);
			pdata++;
		}

		unsigned int v = 0;
		ulong _imageid = 0;
		for (int i = 0; i < 6; i++)
		{

			v = ptask->m_image_id[i];
			v *= pow(256, 5 - i);
			_imageid += v;
		}
		ptask->m_image_id_num = _imageid;

		ptask->m_requestType = uchar(*pdata); pdata++;
		uchar image_num = uchar(*pdata); pdata++;
		uchar img_len_H = uchar(*pdata); pdata++;
		uchar img_len_L = uchar(*pdata); pdata++;
		ulong img_path_len = img_len_H * 256 + img_len_L;
		std::string img_pathes = std::string(pdata, img_path_len);

		pdata += img_path_len;
		std::vector<std::string> img_pathes_vec = ccutil::split(img_pathes, ";");
		for (std::vector<std::string>::iterator it=img_pathes_vec.begin();it!=img_pathes_vec.end();)
		{
			if (it->empty())
			{
				it = img_pathes_vec.erase(it);
			}
			else
			{
				it++;
			}
		}
		if (img_pathes_vec.size() != image_num)
		{
			logger->warn("image num {} requested is not equal to path {} num!", image_num, img_pathes_vec.size());
			image_num = (image_num <= img_pathes_vec.size()) ? img_pathes_vec.size() : image_num;
		}
		ptask->m_task_id = _task_id; //总任务id
		_task_id++;
		uchar top_img_ind = uchar(*pdata); pdata++; //顶部视图索引

		uchar bottom_img_ind = 0;
		if (parent->m_param.support_bottom_view)
		{
			bottom_img_ind = uchar(*pdata); pdata++; //底部视图索引
		}
		uchar barcode_len = uchar(*pdata); pdata++;
		ptask->m_chsBarcode = std::string(pdata, barcode_len);
		pdata += barcode_len;
		ptask->m_iOCRID = ocr_id;
		ptask->m_image_total_num = image_num;
		ptask->m_time_start = stl::time::tick();

		TaskData* subtask = new TaskData[image_num];
		TaskData* postask = subtask;
		for (int i=0;i<image_num;i++) //拆分任务
		{
			TaskData* subtask = new TaskData();
			memcpy(subtask->m_image_id, ptask->m_image_id, 6);
			subtask->m_image_id_num = ptask->m_image_id_num;
			subtask->m_requestType = ptask->m_requestType;
			subtask->m_task_id = ptask->m_task_id;
			subtask->m_chsBarcode = ptask->m_chsBarcode;
			subtask->m_iOCRID = ptask->m_iOCRID;
			subtask->m_image_total_num = ptask->m_image_total_num;
			subtask->m_time_start = ptask->m_time_start;

			subtask->m_sub_task_id = i;  //任务子id
			subtask->m_enImageView = IMAGE_VIEW_INDEX::LEFT;
			subtask->m_enImageView = ((i + 1) == top_img_ind) ? IMAGE_VIEW_INDEX::TOP : subtask->m_enImageView;
			subtask->m_enImageView = ((i + 1) == bottom_img_ind) ? IMAGE_VIEW_INDEX::BOTTOM : subtask->m_enImageView;
			subtask->m_chsRemoteImgPath = img_pathes_vec[i];
			parent->_start_one_task(subtask);
			parent->_update_one_task_sp(subtask, PROCESS_STATE::LOAD_IMAGE);
			stl::time::sleep(3);
		}


		logger->debug("success push a main task, image id:{}", ptask->m_image_id_num);
	}

}

void OCR_MainWoker::thread_send_msg(OCR_MainWoker* parent)
{
	TaskData *ptask;
	
	while (true)
	{
		int res = parent->_acquire_one_task(&ptask, PROCESS_STATE::SEND_TASK);
		if (!res)
		{
			stl::time::sleep(5);
			continue;
		}

		//生成消息
		std::string msg = parent->_convert_task2msg(ptask);
		msg.push_back(NetworkServer::generate_msg_validation_code_china_post(msg));
		msg = NetworkServer::encode_data_hex(msg); //编码数据

		//提供界面显示
		ptask->m_time_send = stl::time::tick();

		parent->total_task_num += 1;
		if (ptask->m_postcodeNum>0)
		{
			parent->ocr_ok_num += 1;
		}

		unsigned int v = 0;
		unsigned int taskid = 0;
		for (int i = 0; i < 6; i++)
		{

			v = ptask->m_image_id[i];
			v *= pow(256, 5 - i);
			taskid += v;
		}
		

		parent->_push_postcode(" [id-"+ std::to_string(taskid) +"] " +ptask->m_chsOcrPostcode);
		logger->debug("sending a msg to image servre, image id:{}", ptask->m_image_id_num);


		//发送消息
		if (parent->pClient->m_is_connected)
		{
			parent->pClient->send_message(msg);
		}

		//注销消息
		parent->_erase_one_task(ptask);
		//delete[] ptask->m_Pointer;
	}

}

void OCR_MainWoker::thread_detect_tags(OCR_MainWoker* parent, int thread_id)
{
	std::string exe_dir = CommonFunc::get_exe_dir();
	std::string model_path = CommonFunc::joinFilePath(exe_dir, "model_file\\efficientdetd0_one.onnx");
	wchar_t modelfilew[512] = { 0 };
	CommonFunc::MCharToWChar(model_path.c_str(), modelfilew);
	const int image_num = 1;
	TagDetector parcelRecop(modelfilew, parent->m_param.tag_detect_confidence, 0, image_num);
	parcelRecop.initial();
	TaskData *ptask;

	while (true)
	{
		int res = parent->_acquire_one_task(&ptask, PROCESS_STATE::DETECTION);
		if (!res)
		{
			continue;
		}
		logger->debug("thread detection tag: get a task, taskid:{}-{}/{}", ptask->m_image_id_num, \
			ptask->m_sub_task_id, ptask->m_image_total_num);


		std::vector<cv::Mat> mats;
		for (int j = 0; j < image_num; j++)
		{
			if (ptask->parcelMat)
			{
				mats.push_back(*(ptask->parcelMat));
			}
		}
		std::vector<std::vector<cv::RotatedRect>> rrects(image_num);
		std::vector<std::vector<int>> cls_inds(image_num);
		for (int j = 0; j < image_num; j++)
		{
			rrects[j].reserve(MAX_BOX_NUM);
			cls_inds[j].reserve(MAX_BOX_NUM);
		}
		//parcelRecop.detect_mats(mats, points);
		if (mats.size()>0)
		{
			try
			{
				parcelRecop.detectParcels(mats, rrects, cls_inds);
			}
			catch (...)
			{
				logger->warn("some exception raised in detection tag, image_id:{}, subid:{}",\
					ptask->m_image_id_num,ptask->m_sub_task_id);
			}
			
		}
		
		for (int i=0;i<rrects[0].size();i++)
		{
			//ImageProcessFunc::drawRotateRect(mats[0], rrects[0][i], 2, cv::Scalar(125, 255, 255));
			switch (cls_inds[0][i])
			{
			case TAG_INDEX::STD_TAG:
			{
				if (!ptask->m_is_get_std_tag)
				{
					ptask->m_is_get_std_tag = 1;
					ptask->m_rc_std_tag = rrects[0][i];
				}
				break;
			}
			case TAG_INDEX::WIN_TAG:
			{
				if (!ptask->m_is_get_win_tag)
				{
					ptask->m_is_get_win_tag = 1;
					ptask->m_rc_window_tag = rrects[0][i];
				}
				break;
			}
			case TAG_INDEX::ARB_TAG:
			{
				if (ptask->m_is_get_arb_tag!=2)
				{
					ptask->m_rc_arb_tag[ptask->m_is_get_arb_tag] = rrects[0][i];
					ptask->m_is_get_arb_tag += 1;
				}
				break;
			}
			case TAG_INDEX::HDW_BOX:
			{
				if (ptask->m_is_get_hwrt_tag != 2)
				{
					ptask->m_rc_hwrite_tag[ptask->m_is_get_hwrt_tag] = rrects[0][i];
					ptask->m_is_get_hwrt_tag += 1;
				}
				break;
			}
			default:
				break;
			}
		}
		ptask->m_time_detect_tag = stl::time::tick();
// 		cv::imshow("src", mats[0]);
// 		cv::waitKey(0);

		if (ptask->m_is_get_std_tag && parent->m_param.is_ocr_standard_tag)
		{
			parent->_update_one_task_sp(ptask, PROCESS_STATE::OCR_STD_TAG);
		}
		else
		{
			if (ptask->m_is_get_win_tag && parent->m_param.is_ocr_window_tag)
			{
				parent->_update_one_task_sp(ptask, PROCESS_STATE::OCR_WIN_TAG);
			}
			else
			{
				if (ptask->m_is_get_arb_tag && parent->m_param.is_ocr_arb_tag)
				{
					parent->_update_one_task_sp(ptask, PROCESS_STATE::OCR_ARB_TAG);
				}
				else
				{
					if (ptask->m_is_get_hwrt_tag && parent->m_param.is_ocr_handwrite)
					{
						parent->_update_one_task_sp(ptask, PROCESS_STATE::OCR_HANDWRITE_BOX);
					}
					else
					{
						parent->_update_one_task_sp(ptask, PROCESS_STATE::PACKAGE);
					}
					
				}
			}
		}
	//while
	}
}

void OCR_MainWoker::thread_load_image(OCR_MainWoker* parent)
{
	std::string serverIP = parent->m_param.image_server_ip;
	TaskData *ptask;
	while (true)
	{
		int res = parent->_acquire_one_task(&ptask, PROCESS_STATE::LOAD_IMAGE);
		if (!res)
		{
			continue;
		}

		logger->debug("thread load image: get a task, taskid:{}-{}/{}", ptask->m_image_id_num, \
			ptask->m_sub_task_id, ptask->m_image_total_num);


		auto pos = ptask->m_chsRemoteImgPath.find(':');
		if (pos!=std::string::npos)
		{
			ptask->m_chsRemoteImgPath = ptask->m_chsRemoteImgPath.substr(pos+1);
		}
		ptask->m_chsRemoteImgPath = "\\\\" + serverIP + "\\" + ptask->m_chsRemoteImgPath;
		ptask->m_chsLocalImgPath = ptask->m_chsRemoteImgPath;
		
		cv::Mat tmp_mat = cv::imread(ptask->m_chsLocalImgPath);

		ptask->m_time_load_image = stl::time::tick();
		if (tmp_mat.empty())
		{
			logger->warn("Load image fail:{}", ptask->m_chsLocalImgPath);
			//std::cout << "task " << ptask->m_task_id << " loading image fail" << std::endl;
			parent->_update_one_task_sp(ptask, PROCESS_STATE::PACKAGE);
		}
		else
		{
			ptask->pSrcMat = new cv::Mat(tmp_mat);
			parent->_update_one_task_sp(ptask, PROCESS_STATE::CUT_PARCEL);
		}
		
	}

}

void OCR_MainWoker::thread_cut_parcel(OCR_MainWoker* parent)
{
	float tray_lf = parent->m_param.tray_lf;
	float tray_rt = parent->m_param.tray_rt;
	int tray_pix = parent->m_param.tray_pix;
	TaskData*ptask;
	
	while (true)
	{
		int res = parent->_acquire_one_task(&ptask, PROCESS_STATE::CUT_PARCEL);
		if (!res)
		{
			stl::time::sleep(5);
			continue;
		}

// 		ptask->parcelMat = new cv::Mat();
// 		*ptask->parcelMat = *ptask->pSrcMat;
		logger->debug("thread cut parcel: get a task, taskid:{}-{}/{}", ptask->m_image_id_num, \
			ptask->m_sub_task_id, ptask->m_image_total_num);


		cv::Mat _parcel;
		try
		{
			res = CutParcelBox::cutParcelMat(*ptask->pSrcMat, _parcel, tray_lf, \
				tray_rt, tray_pix, ptask->m_enImageView, 0);
		}
		catch (...)
		{
			logger->warn("some error raised in cutParcel!");
		}
		if (res)
		{
			if (!ptask->parcelMat)
			{
				ptask->parcelMat = new cv::Mat();
				*ptask->parcelMat = _parcel;
			}
		}

		ptask->m_time_cut_parcel = stl::time::tick();

		if (res)
		{
			parent->_update_one_task_sp(ptask, PROCESS_STATE::DETECTION);
		}
		else
		{
			parent->_update_one_task_sp(ptask, PROCESS_STATE::PACKAGE);
		}

	}
}

void OCR_MainWoker::thread_ocr_std_tag(OCR_MainWoker* parent)
{
	std::string exe_dir = CommonFunc::get_exe_dir();
	OCRStandardTag stocr;
	OcrAlgorithm_config cfg;

	cfg.strict_mode = parent->m_param.use_strict_mode;
	cfg.support_postcode_10 = parent->m_param.support_code_num_10;
	cfg.support_postcode_4 = parent->m_param.support_code_num_4;


	tesseract::TessBaseAPI tess;
	if (tess.Init((exe_dir + "\\tessdata").c_str(), "digits", tesseract::OcrEngineMode::OEM_LSTM_ONLY, NULL, 0, NULL, NULL, false))
	{
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
		return;
	}

	cfg.pTessEn = &tess;
	int res = cfg.match_data.getMatchDataFromImg_tag_line(exe_dir + "\\resource\\match_line_image.jpg");
	if (!res)
	{
		std::cout << "OCR std tag: cannot load match_line_image!" << std::endl;
		return;
	}
	TaskData* ptask;
	size_t noread_num = 0;


	while (true)
	{
		res = parent->_acquire_one_task(&ptask, PROCESS_STATE::OCR_STD_TAG);
		if (!res)
		{
			stl::time::sleep(5);
			continue;
		}

		if (!ptask->m_is_get_std_tag)
		{
			continue;
		}


		std::cout << "" << std::endl;
		logger->debug("thread ocr std tag: get a task, taskid:{}-{}/{}", ptask->m_image_id_num, \
			ptask->m_sub_task_id, ptask->m_image_total_num);

		cv::Mat std_tag_m;
		ImageProcessFunc::getMatFromRotatedRect(*ptask->parcelMat, std_tag_m, ptask->m_rc_std_tag, 125);
// 		cv::imshow("std tag", std_tag_m);
// 		cv::Mat showm;
// 		cv::resize(*ptask->parcelMat, showm, cv::Size(), 0.1, 0.1);
// 		cv::imshow("parcel", showm);
// 		cv::waitKey(0);
		
		std::string postcode;
		try
		{
			postcode = stocr.get_postcode_string(std_tag_m, &cfg);
		}
		catch (...)
		{
			logger->warn("Some exception raised in ocr std tag, imageid:{}, subid:{}", \
				ptask->m_image_id_num, ptask->m_sub_task_id);
		}
		
		if (!postcode.empty())
		{

			if (postcode.length()==10 && parent->m_param.support_code_num_10 && postcode.find('-') != std::string::npos)
			{

			}
			else
			{
				postcode = postcode.substr(postcode.size() - 5);
			}


			ptask->m_chsOcrPostcode = postcode;
			ptask->m_postcode_of_tag_type = TAG_INDEX::STD_TAG;
			ptask->m_postcodeNum = 1;

		}
		else
		{
			cv::imwrite(exe_dir + "\\saved_file\\" + std::to_string(noread_num % 5000) + ".jpg", std_tag_m);
			logger->debug("thread ocr std tag: get no postcode, {}", stocr.get_last_log());
			noread_num += 1;
		}
		ptask->m_time_ocr = stl::time::tick();
		parent->_update_one_task_sp(ptask, PROCESS_STATE::PACKAGE);
	}
}

cv::Point2f OCR_MainWoker::__transform_angle(const cv::Point2f &spt, float theta)
{
	float x = std::cos(theta)*spt.x + std::sin(theta)*spt.y;
	float y =  std::cos(theta)*spt.y - std::sin(theta)*spt.x;
	return cv::Point2f(-x, -y);
}


int OCR_MainWoker::__judge_right_bottom(const cv::RotatedRect &a, const cv::Rect &parcelrc)
{
	cv::Point centp = parcelrc.tl() + cv::Point(parcelrc.width / 2, parcelrc.height / 2);
	float angler = -a.angle / 180.0*CV_PI;//标签-角度转弧度
	angler = format_angle_rad(angler);
	int is_bottomright = 0;
	if (parcelrc.width > parcelrc.height)
	{
		if (angler<CV_PI / 8 || angler>15*CV_PI / 8)//水平向右
		{
			if (a.center.x > (centp.x+parcelrc.width*0.1) && a.center.y > (centp.y + parcelrc.height*0.1))
				is_bottomright = 1;
		}
		if (angler > 7 * CV_PI / 8 && angler < 9 * CV_PI / 8)//水平向左
		{
			if (a.center.x < (centp.x - parcelrc.width*0.1) && a.center.y < (centp.y - parcelrc.height*0.1))
				is_bottomright = 1;
		}
	}
	else
	{
		if (angler < 5 * CV_PI / 8 && angler>4 * CV_PI / 8)//向上
		{
			if (a.center.x > (centp.x + parcelrc.width*0.1) && a.center.y < (centp.y - parcelrc.height*0.1))
				is_bottomright = 1;
		}
		if (angler > 11 * CV_PI / 8 && angler < 13 * CV_PI / 8)//向下
		{
			if (a.center.x < (centp.x - parcelrc.width*0.1) && a.center.y > (centp.y + parcelrc.height*0.1))
				is_bottomright = 1;
		}
	}

	return is_bottomright;
}

int OCR_MainWoker::set_test_mode(int test_mode /*= 0*/)
{
	this->m_param.is_test_mode = test_mode;
	logger->warn("Set test mode:{}", test_mode);
	return 1;
}

void OCR_MainWoker::thread_ocr_arb_tag(OCR_MainWoker* parent)
{
	std::string exe_dir = CommonFunc::get_exe_dir();
	OCRArbitaryTag arbocr;
	OcrAlgorithm_config cfg;
	cfg.strict_mode = parent->m_param.use_strict_mode;
	bool support_10bit = parent->m_param.support_code_num_10;
	int configs_size = 1;
	tesseract::TessBaseAPI tess;
	if (tess.Init((exe_dir + "/tessdata").c_str(), "tha", tesseract::OcrEngineMode::OEM_LSTM_ONLY))
	{
		std::cout << "OCRTesseract: Could not initialize tesseract for arb tag." << std::endl;
		return;
	}

	cfg.pTessThld = &tess;
	TaskData*ptask;

	//key words
	std::vector<std::string> keywords;
	std::ifstream ifs;
	ifs.open((exe_dir + "/resource/key words.txt").c_str());
	int noread_num = 0;

	if (ifs.is_open())
	{
		while (ifs.peek() != EOF)
		{
			char tmpstr[512] = { 0 };
			ifs.getline(tmpstr, 511);
			std::string kstr(tmpstr);
			if (!kstr.empty())
			{
				keywords.push_back(kstr);
				std::cout << "arb tag key words:" << kstr << std::endl;
			}
		}
		ifs.close();
	}

	while (true)
	{

		int res = parent->_acquire_one_task(&ptask, (PROCESS_STATE)(PROCESS_STATE::OCR_ARB_TAG | PROCESS_STATE::OCR_WIN_TAG));
		if (!res)
		{
			stl::time::sleep(5);
			continue;
		}


		logger->debug("thread ocr arb tag: get a task, taskid:{}-{}/{}", ptask->m_image_id_num, \
			ptask->m_sub_task_id, ptask->m_image_total_num);

		if (ptask->m_is_get_win_tag) //窗口标签
		{
			std::cout << "thread ocr arb tag: get a win tag:" << std::endl;
			cv::Mat win_tag_m;
			ImageProcessFunc::getMatFromRotatedRect(*ptask->parcelMat, win_tag_m, ptask->m_rc_window_tag, 125);
			std::string ss;
			try
			{
				ss = arbocr.get_postcode_string(win_tag_m, &cfg);
			}
			catch (...)
			{
				logger->warn("Some exception raised in ocr win tag, imageid:{}, subid:{}", \
					ptask->m_image_id_num, ptask->m_sub_task_id);
			}
			
			if (ss.length() == 5 || (ss.length() == 10 && support_10bit))
			{
				ptask->m_postcodeNum = 1;
				ptask->m_chsOcrPostcode = ss;
				ptask->m_postcode_of_tag_type = TAG_INDEX::WIN_TAG;
			}
			std::cout << "thread ocr arb tag: get a win tag postcode: "<<ss << std::endl;
			ptask->m_time_ocr = stl::time::tick();
			parent->_update_one_task_sp(ptask, PROCESS_STATE::PACKAGE);
			continue;
		}

		//任意打印标签
		if (ptask->m_is_get_arb_tag==1) //只有一个
		{
			bool is_ok = false;

 			cv::Mat arb_tag_m;
			ImageProcessFunc::getMatFromRotatedRect(*ptask->parcelMat, arb_tag_m, ptask->m_rc_arb_tag[0], 0);
// 			cv::imshow("arb", arb_tag_m);
// 			cv::waitKey(0);
			std::string ss;

			try
			{
				ss = arbocr.get_postcode_string(arb_tag_m, &cfg);
			}
			catch (...)
			{
				logger->warn("Some exception raised in ocr arb tag, imageid:{}, subid:{}", \
					ptask->m_image_id_num, ptask->m_sub_task_id);
			}

			if (ss.length()==5 || (ss.length()==10 && support_10bit))
			{
				std::string ocrdata = arbocr.get_last_full_ocr_data();//通过关键字判断是否是收件信息
				bool is_recv = false;
				for (int i=0;i<keywords.size();i++)
				{
					size_t pos = ocrdata.find(keywords[i]);
					if (pos!= std::string::npos)
					{
						is_recv = true;
						break;
					}
				}
				if (is_recv || parent->m_param.is_test_mode)
				{
					ptask->m_postcodeNum = 1;
					ptask->m_chsOcrPostcode = ss;
					ptask->m_postcode_of_tag_type = TAG_INDEX::ARB_TAG;
					std::cout << "Thread ocr arb tag: get a arb tag postcode: " << ss << std::endl;
				}
			}
			else
			{
				cv::imwrite(exe_dir + "\\saved_file\\arb_" + std::to_string(noread_num % 100) + ".jpg", arb_tag_m);
				logger->debug("thread ocr std tag: get no postcode, {}", arbocr.get_last_log());
				noread_num += 1;
			}

		}
		else if(ptask->m_is_get_arb_tag == 2)//两个
		{


			int rb_ind = OCR_MainWoker::__get_right_bottom_rrect(ptask->m_rc_arb_tag[0], \
				ptask->m_rc_arb_tag[1]);
			if (rb_ind)
			{
				cv::Mat arb_tag_m;
				ImageProcessFunc::getMatFromRotatedRect(*ptask->parcelMat, arb_tag_m, \
					ptask->m_rc_arb_tag[rb_ind - 1], 125);
				std::string postcode;

				try
				{
					postcode = arbocr.get_postcode_string(arb_tag_m, &cfg);
				}
				catch (...)
				{
					logger->warn("Some exception raised in ocr arb tag2, imageid:{}, subid:{}", \
						ptask->m_image_id_num, ptask->m_sub_task_id);
				}

				if (postcode.length() == 5 || (support_10bit && postcode.length() == 10))
				{
					ptask->m_postcodeNum = 1;
					ptask->m_chsOcrPostcode = postcode;
					ptask->m_postcode_of_tag_type = TAG_INDEX::ARB_TAG;
					std::cout << "Thread ocr arb tag: get a arb tag postcode: " << postcode << std::endl;
				}
			}


		}
		ptask->m_time_ocr = stl::time::tick();
		parent->_update_one_task_sp(ptask, PROCESS_STATE::PACKAGE);
	}
}

void OCR_MainWoker::thread_ocr_hwrt_box(OCR_MainWoker* parent)
{
	TaskData *ptask;
	int res = 0;
	OCRHandWriteBox hwocr;

	std::string exe_dir = CommonFunc::get_exe_dir();
	std::string modelfile = "\\model_file\\last.onnx";
	//int cuda_ind = 0;
	modelfile = exe_dir + modelfile;
	std::cout << "handwrite ocr model path:" << modelfile << std::endl;
	wchar_t modelfilew[512] = { 0 };
	CommonFunc::MCharToWChar(modelfile.c_str(), modelfilew);
	hwocr.initial_model(modelfilew, 0.3);

	tesseract::TessBaseAPI tess;
	if (tess.Init((exe_dir + "/tessdata").c_str(), "eng", tesseract::OcrEngineMode::OEM_LSTM_ONLY))
	{
		std::cout << "handwrite tesseract engine do not loaded" << std::endl;
	}

	std::vector<std::string> keywords = { "to","10" };

	int noread_num = 0;
	//等待完成
	while (true)
	{
		res = parent->_acquire_one_task(&ptask, PROCESS_STATE::OCR_HANDWRITE_BOX);
		if (!res)
		{
			stl::time::sleep(5);
			continue;
		}
		std::string ocr_res;


		std::cout << "" << std::endl;
		logger->debug("thread ocr hwt tag: get a task, taskid:{}-{}/{}", ptask->m_image_id_num, \
			ptask->m_sub_task_id, ptask->m_image_total_num);


		//正常模式
		if (ptask->m_is_get_hwrt_tag == 1)
		{
			cv::Mat hwer;
			cv::RotatedRect rc = ptask->m_rc_hwrite_tag[0];
			rc.size.height += 10;
			rc.size.width += 10;
			//ImageProcessFunc::drawRotateRect(show_mat, rc, 4, cv::Scalar(0, 255, 0));
			ImageProcessFunc::getMatFromRotatedRect(*ptask->parcelMat, hwer, rc, 125);
			if (hwocr.identify_handbox_type(hwer) == 2)//有边框才判断
			{
				int res = 0;
				try
				{
					res = hwocr.find_key_words(hwer, keywords, &tess); //查找关键字
				}
				catch (...)
				{
					logger->warn("Some exception raised in find keywords of handwrite ocr, imageid:{}, subid:{}", \
						ptask->m_image_id_num, ptask->m_sub_task_id);
				}

				std::string log_str1 = hwocr.get_last_log();
				if (res)
				{
					ocr_res = hwocr.get_postcode_string(hwer);
					if (ocr_res.empty())
					{
						std::string log_str = hwocr.get_last_log();
						std::cout << "logstr:" << log_str << std::endl;
					}
				}
			}
		}
		else if (ptask->m_is_get_hwrt_tag == 2) //两个手写框
		{
			int rb_ind = OCR_MainWoker::__get_right_bottom_rrect(ptask->m_rc_hwrite_tag[0], \
				ptask->m_rc_hwrite_tag[1]);
			if (rb_ind)
			{
				cv::Mat hwer;
				cv::RotatedRect rc = ptask->m_rc_hwrite_tag[rb_ind-1];
				rc.size.height += 10;
				rc.size.width += 10;
				//ImageProcessFunc::drawRotateRect(show_mat, rc, 4, cv::Scalar(0, 255, 0));
				ImageProcessFunc::getMatFromRotatedRect(*ptask->parcelMat, hwer, rc, 125);

				int boxtype = hwocr.identify_handbox_type(hwer);
				try
				{
					if (boxtype == 1)
					{
						ocr_res = hwocr.get_postcode_string_test_v2(hwer);
					}
					else if (boxtype == 2)
					{
						ocr_res = hwocr.get_postcode_string(hwer);
					}
				}
				catch (...)
				{
					logger->warn("Some exception raised in handwrite ocr, imageid:{}, subid:{}", \
						ptask->m_image_id_num, ptask->m_sub_task_id);
				}


				if (ocr_res.empty())
				{
					std::string log_str = hwocr.get_last_log();
					std::cout << "logstr:" << log_str << std::endl;
					logger->debug("imageid:{}/subid:{}, hwrt box ocr no res:{}", \
						ptask->m_image_id_num, ptask->m_sub_task_id, log_str);
				}
				if (!hwer.empty() && ocr_res.empty())
				{
					cv::imwrite(exe_dir + "\\saved_file\\box_" + std::to_string(noread_num) + ".jpg", hwer);
					noread_num += 1;
					if (noread_num > 100) noread_num = 0;
				}
			}
		}

		if (ocr_res.size()>0) //检测所有数字是否一样，排除规律污染物的干扰
		{
			bool is_same = true; 
			char c = ocr_res[0];
			for (size_t i = 1; i < ocr_res.size(); i++)
			{
				if (c != ocr_res[i])
				{
					is_same = false;
					break;
				}
			}
			if (is_same)
			{
				ocr_res = "";
			}
		}
// 			cv::resize(show_mat, show_mat, cv::Size(), 0.4, 0.4);
// 			cv::imshow("showmat", show_mat);
// 			cv::waitKey(0);

		if (!ocr_res.empty())
		{
			ptask->m_postcodeNum = 1;
			ptask->m_postcode_of_tag_type = TAG_INDEX::HDW_BOX;
			ptask->m_chsOcrPostcode = ocr_res;
		}

		ptask->m_time_ocr = stl::time::tick();
		parent->_update_one_task_sp(ptask, PROCESS_STATE::PACKAGE);

	}

}

void OCR_MainWoker::thread_package_tasks(OCR_MainWoker* parent)
{
	std::cout << "enter thread_package_tasks" << std::endl;
	TaskData*ptask;
	while (true)
	{
		std::vector<TaskData*> tasks;
		int res = parent->_acquire_one_task(&ptask, PROCESS_STATE::PACKAGE);
		if (!res)
		{
			stl::time::sleep(5);
			continue;
		}
		res = parent->_acquire_stem_tasks(tasks, ptask->m_task_id, PROCESS_STATE::PACKAGE);
		if (res < ptask->m_image_total_num)
		{
			stl::time::sleep(10);
			continue;
		}
		TaskData* mtask = parent->_merge_tasks(tasks); //合并子任务
		if (!mtask)
		{
			mtask = tasks[0];
		}
		for (int i=0;i<tasks.size();i++)
		{
			if (mtask == tasks[i]) //保留一个子任务作为主任务
			{
				continue;
			}
			parent->_erase_one_task(tasks[i]);
		}

		parent->_update_one_task_sp(mtask, PROCESS_STATE::SEND_TASK);
	}

}

void OCR_MainWoker::thread_check_state(OCR_MainWoker* parent)
{
	bool is_connect_to_imageserver = false;
	while (true)
	{
		bool con_stat = parent->pClient->m_is_connected;
		if (con_stat!=is_connect_to_imageserver)
		{
			is_connect_to_imageserver = con_stat;
			if (con_stat)
			{
				logger->warn("Connected to Imageserver: {}:{}.", parent->m_param.image_server_ip, parent->m_param.image_server_port);
			}
			else
			{
				logger->warn("Lost connection to Imageserver.");
			}
		}

		stl::time::sleep(1000);
	}
}

int OCR_MainWoker::__get_right_bottom_rrect(const cv::RotatedRect &a, const cv::RotatedRect &b)
{

	float angle_1 = a.angle / 180.0*CV_PI;//标签-角度转弧度
	float angle_2 = b.angle / 180.0*CV_PI;

	float angle_v = std::abs(angle_2 - angle_1);
	if (angle_v > CV_PI) angle_v = 2 * CV_PI - angle_v;

	//两个标签角度之间的夹角需要小于45度
	if (angle_v > CV_PI / 4.0)
	{
		return 0;
	}

	//两个标签的中心点
	cv::Point2f parcel_cent = (a.center + b.center) / 2;

	//标签相对于中心点的方向
	cv::Point2f tag1_pos_vec = a.center - parcel_cent;
	cv::Point2f tag2_pos_vec = b.center - parcel_cent;

	if (std::abs(angle_2 - angle_1) > CV_PI)
	{
		if (angle_1>angle_2)
		{
			angle_1 = 2 * CV_PI - angle_1;
		}
		else
		{
			angle_2 = 2 * CV_PI - angle_2;
		}
	}


	tag1_pos_vec = OCR_MainWoker::__transform_angle(tag1_pos_vec, (angle_1 + angle_2) / 2.0);
	tag2_pos_vec = OCR_MainWoker::__transform_angle(tag2_pos_vec, (angle_1 + angle_2) / 2.0);

	if (tag2_pos_vec.y > tag1_pos_vec.y && tag2_pos_vec.x > tag1_pos_vec.x)
	{
		return 1;
	}
	else if (tag2_pos_vec.y < tag1_pos_vec.y && tag2_pos_vec.x < tag1_pos_vec.x)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

std::string OCR_MainWoker::_convert_task2msg(TaskData*ptask)
{
	std::string msg;
	msg.push_back(MSG_IMG_ADDR);
	msg.push_back(MSG_OCR2IMG_RES);
	for (int i=0;i<6;i++)
	{
		msg.push_back(ptask->m_image_id[i]);
	}
	msg.push_back(ptask->m_iOCRID);
	BYTE rescode= RES_OCR_NO_POSTCODE;
	switch (ptask->m_postcodeNum)
	{
	case 0:
		rescode = RES_OCR_NO_POSTCODE; //没有邮编
		break;
	case 1:
		rescode = RES_OCR_POSTCODE_1; //一个邮编
		break;
	default:
		rescode = RES_OCR_POSTCODE_2; //多个邮编
		break;
	}
	if (rescode == RES_OCR_NO_POSTCODE && !ptask->pSrcMat) rescode = RES_OCR_NO_IMG;
	msg.push_back(rescode);  //邮编结果
	msg.push_back((rescode == RES_OCR_POSTCODE_1) ? ptask->m_PostcodeIndextImage : 0); //邮编所在图像索引
	msg.push_back(ptask->m_chsBarcode.size());
	msg += ptask->m_chsBarcode;
	msg.push_back((uchar)ptask->m_chsOcrPostcode.size());
	msg += ptask->m_chsOcrPostcode;
	msg.push_back(_get_task_num());
	return msg;
}

ulong OCR_MainWoker::_get_task_num()
{
	std::unique_lock <std::mutex> lck(this->m_mutex);
	return m_tasks_pool.size();
}

size_t OCR_MainWoker::get_total_task_num()
{
	return total_task_num;
}

size_t OCR_MainWoker::get_ocr_ok_num()
{
	return ocr_ok_num;
}

int OCR_MainWoker::get_last_postcode(std::string &postcode)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_ex);
	if (m_ocr_postcode.size()>0)
	{
		postcode = m_ocr_postcode[0];
		m_ocr_postcode.erase(m_ocr_postcode.begin());
		return 1;
	}
	return 0;
}

int OCR_MainWoker::_push_postcode(const std::string &postcode)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_ex);
	m_ocr_postcode.push_back(postcode);
	return 1;
}

void OCR_MainWoker::reset_count()
{
	total_task_num = 0;
	ocr_ok_num = 0;
}

void OCR_MainWoker::push_debug_image(const std::string &postcode)
{
	TaskData *ptask = new TaskData();
	ptask->m_image_id_num = 999999;
	ptask->m_image_total_num = 1;
	ptask->m_enImageView = IMAGE_VIEW_INDEX::TOP;
	ptask->m_sub_task_id = 0;
	ptask->m_task_id = 99999999;
	ptask->m_time_start = stl::time::tick();
	ptask->m_time_load_image = stl::time::tick();
	
	cv::Mat m = cv::imread(postcode);
	if (m.empty())
	{
		std::cout << "image load is empty, :"<<postcode<<std::endl;
		return;
	}
	ptask->pSrcMat = new cv::Mat(m);

	this->_start_one_task(ptask);
	this->_update_one_task_sp(ptask, PROCESS_STATE::CUT_PARCEL);

}

TaskData* merge_same_type_task(std::vector<TaskData*> tasks)
{
	tasks[0]->m_resultState = 1;
	bool diff = 0;
	std::string postcode = tasks[0]->m_chsOcrPostcode;
	tasks[0]->m_PostcodeIndextImage = tasks[0]->m_sub_task_id;
	for (int i = 1; i < tasks.size(); i++)
	{
		if (postcode != tasks[i]->m_chsOcrPostcode)
		{
			tasks[0]->m_chsOcrPostcode += ";";
			tasks[0]->m_chsOcrPostcode += tasks[i]->m_chsOcrPostcode;
			tasks[0]->m_postcodeNum += 1;
			diff = 1;
			tasks[0]->m_resultState = 13;
			break;
		}
	}
	return tasks[0];
}


TaskData* OCR_MainWoker::_merge_tasks(std::vector<TaskData*> tasks)
{
	std::vector<TaskData*> _std_tag;
	std::vector<TaskData*> _win_tag;
	std::vector<TaskData*> _arb_tag;
	std::vector<TaskData*> _hwrt_box;
	for (int i=0;i<tasks.size();i++)
	{
		if (tasks[i]->m_postcodeNum == 0)//邮编数量等于0的不计
		{
			continue;
		}
		switch (tasks[i]->m_postcode_of_tag_type)
		{
		case TAG_INDEX::STD_TAG:
			_std_tag.push_back(tasks[i]);
			break;
		case TAG_INDEX::WIN_TAG:
			_win_tag.push_back(tasks[i]);
			break;
		case TAG_INDEX::ARB_TAG:
			_arb_tag.push_back(tasks[i]);
			break;
		case TAG_INDEX::HDW_BOX:
			_hwrt_box.push_back(tasks[i]);
			break;
		default:
			break;
		}
	}
	if (!_std_tag.empty()) //存在标准标签
	{
		return merge_same_type_task(_std_tag);
	}
	if (!_win_tag.empty()) //只可能有一个
	{
		return merge_same_type_task(_win_tag);
	}
	if (m_param.use_strict_mode)//如果严格模式，只保留标准标签 和 窗口标签
	{
		return nullptr;
	}
	if (!_arb_tag.empty())
	{
		return merge_same_type_task(_arb_tag);
	}
	if (!_hwrt_box.empty())
	{
		return merge_same_type_task(_hwrt_box);
	}
	return nullptr;
}

std::string OCR_MainWoker::log_task_tostring(TaskData*ptask)
{
	//imageid:123456-1/2,
	std::string log = "imageid:";
	unsigned int v = 0;
	unsigned int taskid = 0;
	for (int i=0;i<6;i++)
	{

		v = ptask->m_image_id[i];
		v *= pow(256, 5 - i);
		taskid += v;
	}
	log += std::to_string(taskid);
	log += "-" + std::to_string((int)ptask->m_sub_task_id) + "/"\
		+ std::to_string((int)ptask->m_image_total_num) + ", ";

	//view:top, 
	if (ptask->m_enImageView==IMAGE_VIEW_INDEX::TOP)
	{
		log += std::string("view:") + "top, ";
	}
	else if (ptask->m_enImageView == IMAGE_VIEW_INDEX::BOTTOM)
	{
		log += std::string("view:") + "bottom, ";
	}
	else
	{
		log += std::string("view:") + "side, ";
	}
	
	//是否存在图像
	log += std::string("image_exist:") + ((ptask->pSrcMat) ? "yes, " : "no, ");

	//是否存在包裹
	log += std::string("parcel_exist:") + ((ptask->parcelMat) ? "yes, " : "no, ");

	//是否检测到标签
	log += std::string("detection_tag:") + std::to_string(ptask->m_is_get_std_tag) + std::to_string(ptask->m_is_get_win_tag)\
		+ std::to_string(ptask->m_is_get_arb_tag) + std::to_string(ptask->m_is_get_hwrt_tag)+"(swah), ";

	//是否检测到邮编
	log += std::string("postcode_num:") + std::to_string(ptask->m_postcodeNum) + ", ";

	//邮编
	log += "postcode:" + ptask->m_chsOcrPostcode + ", ";

	//邮编来源：
	log += std::string("postcode from:");
	if (ptask->m_postcodeNum>0)
	{
		switch (ptask->m_postcode_of_tag_type)
		{
		case TAG_INDEX::STD_TAG:
			log += std::string("std_tag, ");
			break;
		case TAG_INDEX::WIN_TAG:
			log += std::string("win_tag, ");
			break;
		case TAG_INDEX::ARB_TAG:
			log += std::string("arb_tag, ");
			break;
		case TAG_INDEX::HDW_BOX:
			log += std::string("hdw_tag, ");
			break;
		default:
			log += std::string("None, ");
			break;
		}
	}
	else
	{
		log += std::string("None, ");
	}

	//时间消耗
	size_t time_accum = 0;
	size_t time_comsume = (ptask->m_time_load_image > ptask->m_time_start) ? \
		(ptask->m_time_load_image - ptask->m_time_start -time_accum) : 0;
	log += std::string("load time:") + std::to_string(time_comsume) + ", ";
	time_accum += time_comsume;

	time_comsume = (ptask->m_time_cut_parcel > ptask->m_time_start) ? \
		(ptask->m_time_cut_parcel - ptask->m_time_start - time_accum) : 0;
	log += std::string("cutparcel time:") + std::to_string(time_comsume) + ", ";
	time_accum += time_comsume;

	time_comsume = (ptask->m_time_detect_tag > ptask->m_time_start) ? \
		(ptask->m_time_detect_tag - ptask->m_time_start - time_accum) : 0;
	log += std::string("detection time:") + std::to_string(time_comsume) + ", ";
	time_accum += time_comsume;

	time_comsume = (ptask->m_time_ocr > ptask->m_time_start) ? \
		(ptask->m_time_ocr - ptask->m_time_start - time_accum) : 0;
	log += std::string("ocr time:") + std::to_string(time_comsume) + ", ";
	time_accum += time_comsume;

	time_comsume = (ptask->m_time_send > ptask->m_time_start) ? \
		(ptask->m_time_send - ptask->m_time_start - time_accum) : 0;
	log += std::string("send time:") + std::to_string(time_comsume) + ", ";
	time_accum += time_comsume;

	log += std::string("total time:") + std::to_string(time_accum) + ", ";

	if (ptask->m_sub_task_id==0)
	{
		log += std::string("main task complete!");
	}

	return log;

}

int OCR_MainWoker::_acquire_one_task(TaskData** task, PROCESS_STATE process_state)
{
	std::unique_lock <std::mutex> lck(this->m_mutex);
	//�����������

	if (process_state == PROCESS_STATE::DETECTION)
	{
		while (!__check_one_task(task, PROCESS_STATE::DETECTION))
		{
			m_cond_var_detection.wait(lck);
		}
		(*task)->m_flag_state = PROCESS_STATE::IS_DETECTION;
		return 1;
	}

	if (process_state == PROCESS_STATE::LOAD_IMAGE)
	{
		while (!__check_one_task(task, PROCESS_STATE::LOAD_IMAGE))
		{
			m_cond_var_load_image.wait(lck);
		}
		(*task)->m_flag_state = PROCESS_STATE::IS_LOAD_IMAGE;
		return 1;
	}

	return __check_one_task(task, process_state);

}

int OCR_MainWoker::_acquire_one_task_sp(TaskData** task, ulong task_id, ulong sub_task_id, PROCESS_STATE process_state)
{
	std::unique_lock <std::mutex> lck(this->m_mutex);
	std::vector<TaskData*>::iterator it = m_tasks_pool.begin();
	for (; it != m_tasks_pool.end(); it++)
	{
		if ((*it)->m_flag_state & process_state)
		{
			if (((*it)->m_task_id == task_id) && ((*it)->m_sub_task_id == sub_task_id))
			{
				*task = *it;
				//(*it)->m_flag_state = get_next_state(*(*it));
				return 1;
			}
		}
	}
	return 0;
}

int OCR_MainWoker::_acquire_stem_tasks(std::vector<TaskData*>& tasks, ulong task_id, PROCESS_STATE process_state)
{
	std::unique_lock <std::mutex> lck(this->m_mutex);

	std::vector<TaskData*>::iterator it = m_tasks_pool.begin();
	for (; it != m_tasks_pool.end(); it++)
	{
		if ((*it)->m_flag_state & process_state)
		{
			if (((*it)->m_task_id == task_id))
			{
				tasks.push_back(*it);
			}
		}
	}

	return tasks.size();
}

int OCR_MainWoker::_update_one_task(TaskData* task)
{

}

int OCR_MainWoker::_update_one_task_sp(TaskData* task, PROCESS_STATE process_state)
{
	{
		std::unique_lock <std::mutex> lck(this->m_mutex);
		if (!__isvalid_task(task)) return 0;
		task->m_flag_state = process_state;
	}
	if (task->m_flag_state == PROCESS_STATE::DETECTION)
	{
		m_cond_var_detection.notify_all();
	}
	if (task->m_flag_state == PROCESS_STATE::LOAD_IMAGE)
	{
		m_cond_var_load_image.notify_all();
	}
	return 1;
}

int OCR_MainWoker::_start_one_task(TaskData* task)
{
	std::unique_lock <std::mutex> lck(this->m_mutex);
	task->m_flag_state = PROCESS_STATE::OPEN;
	m_tasks_pool.push_back(task);
	return 1;
}

int OCR_MainWoker::_erase_one_task(TaskData* task)
{
	bool is_valid_task = false;
	{
		std::unique_lock <std::mutex> lck(this->m_mutex);
		std::vector<TaskData*>::iterator it = m_tasks_pool.begin();
		for (; it != m_tasks_pool.end(); it++)
		{
			if ((*it) == task)
			{
				m_tasks_pool.erase(it); //�Ӷ������Ƴ�
				is_valid_task = true;
				break;
			}
		}
	}
	if (is_valid_task)
	{
		std::string logstr = this->log_task_tostring(task);
		std::cout << logstr << std::endl;
		logger->info(logstr);
		__destroy_one_task(task);
	}

	return 1;
}

int OCR_MainWoker::_check_res_stem_task_num()
{





}

int OCR_MainWoker::__destroy_one_task(TaskData* task)
{
	if (task)
	{
		if (task->pSrcMat) delete  task->pSrcMat;
		if(task->parcelMat!=nullptr && task->pSrcMat!=task->parcelMat) delete  task->parcelMat;
		delete task;
		return 1;
	}
	return 0;
}

int OCR_MainWoker::__check_one_task(TaskData** task, PROCESS_STATE process_state)
{
	std::vector<TaskData*>::iterator it = m_tasks_pool.begin();
	for (; it != m_tasks_pool.end(); it++)
	{
		if ((*it)->m_flag_state & process_state)
		{
			*task = *it;
			return 1;
		}
	}
	return 0;
}

bool OCR_MainWoker::__isvalid_task(TaskData* task)
{
	if (!task) return false;
	std::vector<TaskData*>::iterator it = m_tasks_pool.begin();
	for (; it != m_tasks_pool.end(); it++)
	{
		if ((*it) == task)
		{
			return true;
		}
	}
	return false;
}
