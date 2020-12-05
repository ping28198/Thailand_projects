//*********************************************************************//
//2015-03-06 SHUJING 
//********************************************************************//

#ifndef _VCS_SF_H
#define _VCS_SF_H

#define BARCODE_LEN	12			//条码长度 
#define DESNUM_LEN	3			//条码长度 

//表单定位标志
//bool bLoc_Form;

//条码数字识别结果
struct RES_BAR
{
	bool bBar;
	char cBarcode[BARCODE_LEN];
};
//目的地代码识别结果
struct RES_DES
{
	bool bDes;
	char cDes_Num[DESNUM_LEN];
};
struct VCS_RES
{
	int iImageNo;
	RES_BAR mRes_Bar;
	RES_DES mRes_Des;
	void (*hgp_initial)();
//	int  (*WZJ_Hwr_Init)(char *pszPath);
};


//初始化，对识别库进行读取，对识别结果进行初始化，该函数在系统启动时只调用一次
// extern "C" _declspec(dllexport) int SF_InfoInit(VCS_RES *pRes);
extern "C" _declspec(dllimport) int SF_InfoInit(VCS_RES *pRes);
//对前一封图像的识别结果进行清理，该函数在每次调用主处理函数MPF_SF_Process前进行调用
//extern "C" _declspec(dllexport) int SF_InfoClear(VCS_RES *pRes);
extern "C" _declspec(dllimport) int SF_InfoClear(VCS_RES *pRes);

#define MPF_SF_LOCATE_OK 0 
#define MPF_SF_OPENFILE_FAILED -1
#define MPF_SF_LOCATE_FAILED -2
#define MPF_SF_SAVEFILE_FAILED -3

//主处理函数，包括表单的定位、旋转，及条码下数字串的识别，目的地编号的识别，识别结果存入VCS_RES结构体
//extern "C" _declspec(dllexport) int MPF_SF_Process( int iImageNo,VCS_RES *pRes);
//extern "C" _declspec(dllimport) int MPF_SF_Process( int iImageNo,VCS_RES *pRes);
extern "C" _declspec(dllexport) int MPF_SF_Process( int iObrNo, int iImageNo,VCS_RES *pRes);




#endif
