#pragma once
#include <vector>
#include <string>
#include "CPTENetworkServer.h"
#include "opencv2/opencv.hpp"
#include <mutex>
#include <condition_variable>
#include <atomic> 




#ifndef ulong
typedef unsigned long ulong;
#endif // !ulong

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef ulong
typedef unsigned long ulong;
#endif


#define IS_MAX_PATH_LENGTH		512			//文件路径名最大长度
#define IS_MAX_IMG_PER_TASK		6			//文件路径名最大长度
#define IS_BAR_MAX_LEN  51
#define IS_POSTCODE_LEN 5					//标准邮编长度

#define MSG_OCR_ADDR            0xf0
#define MSG_IMG2OCR_REQ         0xc4
#define MSG_IMG_ADDR            0xa0
#define MSG_OCR2IMG_RES         0xc5

#define RES_OCR_POSTCODE_1      1
#define RES_OCR_NO_IMG          11
#define RES_OCR_NO_POSTCODE     12
#define RES_OCR_POSTCODE_2      13





struct sysParameter
{
	std::string image_server_ip;
	unsigned long image_server_port=9999;
	unsigned long local_port=20001;
	float tag_detect_confidence=0.5;
	int ocr_id = 0;
	int is_ocr_standard_tag =1;
	int is_ocr_arb_tag=1;
	int is_ocr_window_tag=1;
	int is_ocr_handwrite=1;
	int is_ocr_10bit_pcode=0;
	
	//邮编设置相关
	int use_strict_mode = 0;//使用严格模式，邮编的校验更加严格，不支持4位邮编

	int support_code_num_10 = 0; //支持10位邮编模式，仅对标准标签有用
	int support_code_num_4 = 0;

	//测试相关
	int is_test_mode = 0;
	int support_bottom_view = 0;

	int log_level = 1;

	//
	/*
	rigid_level
	0, 低限制：
		任意标签：1个标签，需要检测标签关键字；2个标签，两个标签需要处于左上、右下位置，右下标签存在标签即可。
	
	1，中等限制：
		任意标签：1个标签，满足0，标签处于包裹的右下区域，且距离大于100； 2个标签，满足0，并要求两个标签被与其平行的中线上下分割，且距离大于100

	2，高限制：
		任意标签：1个标签，无效；2个标签，满足1，同时标签被垂直中线分割，同时对两个标签运行ocr，两个标签都存在邮编，

	*/
	int rigid_level = 0;  //0, 

	//托盘相关
	float tray_lf=0.14; //0-1,托盘左侧占图片宽度方向比例，
	float tray_rt=0.88;//0-1，托盘右侧占图片宽度方向比例，
	int tray_pix = 30; //托盘的平均像素值
};

enum TAG_INDEX
{
	STD_TAG=0,
	ARB_TAG=2,
	HDW_BOX=3,
	WIN_TAG=4,
};

enum IMAGE_VIEW_INDEX
{
	TOP=0,
	BOTTOM=1,
	FRONT,
	BACK,
	LEFT,
	RIGHT,
};


static const ulong PROCESS_BASE_NUM = 1;

enum PROCESS_STATE
{
	OPEN				 = PROCESS_BASE_NUM << 1,
	LOAD_IMAGE			 = PROCESS_BASE_NUM << 2,
	IS_LOAD_IMAGE		 = PROCESS_BASE_NUM << 3,
	CUT_PARCEL			 = PROCESS_BASE_NUM << 4,
	DETECTION			 = PROCESS_BASE_NUM << 5,
	IS_DETECTION		 = PROCESS_BASE_NUM << 6,
	OCR_STD_TAG			 = PROCESS_BASE_NUM << 7,
	OCR_ARB_TAG			 = PROCESS_BASE_NUM << 8,
	OCR_WIN_TAG			 = PROCESS_BASE_NUM << 9,
	OCR_HANDWRITE_BOX	 = PROCESS_BASE_NUM << 10,
	PACKAGE				 = PROCESS_BASE_NUM << 11,
	SEND_TASK			 = PROCESS_BASE_NUM << 12,
	CLOSE				 = PROCESS_BASE_NUM << 13,
};

class TaskData
{
public:
	TaskData* m_Pointer = nullptr;
	ulong m_task_id; //用于区分总任务ID
	ulong m_sub_task_id; //子任务id
	BYTE m_image_id[6]; //image id
	ulong m_image_id_num;
	BYTE m_iOCRID=0; //机器编号
	BYTE m_requestType=1; // 请求类型：=1 OBR拒识无条码，=4 有条码无目的地信息
	BYTE m_resultState=12; //补码结果：=1 补码成功， = 11 无图像，= 12 未识别邮编，=13 识别多个邮编
	BYTE m_PostcodeIndextImage=0; //邮编所在图像索引，未识别为0
	
	enum IMAGE_VIEW_INDEX m_enImageView;
	BYTE m_image_total_num=0; //任务中的总图片数量。

	std::string m_chsBarcode; //服务器发过来的条码（暂时无用，直接返回）
	std::string m_chsRemoteImgPath;
	std::string m_chsLocalImgPath;
	std::string m_chsOcrPostcode;

	//时间
	ulong m_time_start = 0; //都为处理过程结束的时刻
	ulong m_time_load_image = 0;
	ulong m_time_cut_parcel = 0;
	ulong m_time_detect_tag = 0;
	ulong m_time_ocr= 0;
	ulong m_time_send = 0;

	//ocr结果
	BYTE m_postcodeNum=0;//=0未获得邮编，=1 获得一个邮编，= 2 获得两个邮编..
	int m_postcode_of_tag_type; //TAG_INDEX

	//中间量
	enum PROCESS_STATE m_flag_state;	// =任务进行的状态
	cv::Mat* pSrcMat=nullptr;
	cv::Mat* parcelMat=nullptr;

	//目标检测结果
	int m_is_get_std_tag=0;//0,1
	cv::RotatedRect m_rc_std_tag;
	int m_is_get_win_tag=0;//0,1
	cv::RotatedRect m_rc_window_tag;
	int m_is_get_arb_tag=0; //0,1,2
	cv::RotatedRect m_rc_arb_tag[2];
	int m_is_get_hwrt_tag=0;//0,1,2
	cv::RotatedRect m_rc_hwrite_tag[2];




// 	~TaskData()
// 	{
// 		if (parcelMat) delete parcelMat;
// 		if (pSrcMat) delete pSrcMat;
// 	}

// 	TaskData& operator = (const TaskData &t) {
// 		if (this != &t)
// 		{
// 			memcpy(this, &t, sizeof(TaskData));
// 		}
// 		return *this;
// 	}

};



class OCR_MainWoker
{
public:
	OCR_MainWoker();
	~OCR_MainWoker();

	int load_parameter();
	void initial();
	

	static void thread_recv_msg(OCR_MainWoker* parent);
	static void thread_send_msg(OCR_MainWoker* parent);
	static void thread_detect_tags(OCR_MainWoker* parent, int thread_id);
	//static void thread_copy_file(OCR_MainWoker* parent);
	static void thread_load_image(OCR_MainWoker* parent);
	static void thread_cut_parcel(OCR_MainWoker* parent);
	static void thread_ocr_std_tag(OCR_MainWoker* parent);
	static void thread_ocr_arb_tag(OCR_MainWoker* parent);
	static void thread_ocr_hwrt_box(OCR_MainWoker* parent);
	static void thread_package_tasks(OCR_MainWoker* parent);
	static void thread_check_state(OCR_MainWoker* parent);

	//辅助函数
	//获得右下角的旋转框，范围0，没有，1：a，2：b
	static int __get_right_bottom_rrect(const cv::RotatedRect &a,const cv::RotatedRect &b);
	static cv::Point2f __transform_angle(const cv::Point2f &spt, float theta);
	//判断rect是否处于包裹右下角，（考虑对称情况）
	static int __judge_right_bottom(const cv::RotatedRect &a, const cv::Rect &parcelrc);


	//外部调用
	int set_test_mode(int test_mode = 0);
	size_t get_total_task_num();
	size_t get_ocr_ok_num();
	int get_last_postcode(std::string &postcode);
	int _push_postcode(const std::string &postcode);
	void reset_count();





	std::string _convert_task2msg(TaskData*ptask);
	ulong _get_task_num();
	
	TaskData* _merge_tasks(std::vector<TaskData*> tasks);
	std::string log_task_tostring(TaskData*ptask);

	int _acquire_one_task(TaskData** task, PROCESS_STATE process_state); 
	int _acquire_one_task_sp(TaskData** task, ulong task_id,ulong sub_task_id, PROCESS_STATE process_state);
	int _acquire_stem_tasks(std::vector<TaskData*>& tasks, ulong task_id, PROCESS_STATE process_state);
	int _update_one_task(TaskData* task); //？
	int _update_one_task_sp(TaskData* task, PROCESS_STATE process_state); 
	int _start_one_task(TaskData* task); //启动一个任务
	int _erase_one_task(TaskData* task);
	int _check_res_stem_task_num();



	int __destroy_one_task(TaskData* task);
	int __check_one_task(TaskData** task, PROCESS_STATE process_state);
	bool __isvalid_task(TaskData* task);

public:
	std::atomic_ulong total_task_num = 0;
	std::atomic_ulong ocr_ok_num = 0;
	std::atomic_ulong ocr_res_num = 0;




	std::vector<TaskData*> m_tasks_pool;
	sysParameter m_param;
	NetworkClient *pClient=nullptr;

	std::mutex m_mutex_ex;//外部同步
	std::vector<std::string> m_ocr_postcode;

	//工作线程同步
	std::vector<std::thread*> m_pThreads;
	std::mutex m_mutex;
	std::condition_variable m_cond_var_detection; //
	std::condition_variable m_cond_var_send;
	std::condition_variable m_cond_var_load_image;
	std::condition_variable m_cond_var_package_tasks;
	bool m_is_big_endian = false;
	
};


