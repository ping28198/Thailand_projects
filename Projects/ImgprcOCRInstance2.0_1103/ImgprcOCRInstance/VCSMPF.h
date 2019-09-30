//*********************************************************************//
//2015-03-06 SHUJING 
//********************************************************************//

#ifndef _VCS_SF_H
#define _VCS_SF_H

#define BARCODE_LEN	12			//���볤�� 
#define DESNUM_LEN	3			//���볤�� 

//����λ��־
//bool bLoc_Form;

//��������ʶ����
struct RES_BAR
{
	bool bBar;
	char cBarcode[BARCODE_LEN];
};
//Ŀ�ĵش���ʶ����
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


//��ʼ������ʶ�����ж�ȡ����ʶ�������г�ʼ�����ú�����ϵͳ����ʱֻ����һ��
// extern "C" _declspec(dllexport) int SF_InfoInit(VCS_RES *pRes);
extern "C" _declspec(dllimport) int SF_InfoInit(VCS_RES *pRes);
//��ǰһ��ͼ���ʶ�������������ú�����ÿ�ε�����������MPF_SF_Processǰ���е���
//extern "C" _declspec(dllexport) int SF_InfoClear(VCS_RES *pRes);
extern "C" _declspec(dllimport) int SF_InfoClear(VCS_RES *pRes);

#define MPF_SF_LOCATE_OK 0 
#define MPF_SF_OPENFILE_FAILED -1
#define MPF_SF_LOCATE_FAILED -2
#define MPF_SF_SAVEFILE_FAILED -3

//�����������������Ķ�λ����ת�������������ִ���ʶ��Ŀ�ĵر�ŵ�ʶ��ʶ��������VCS_RES�ṹ��
//extern "C" _declspec(dllexport) int MPF_SF_Process( int iImageNo,VCS_RES *pRes);
//extern "C" _declspec(dllimport) int MPF_SF_Process( int iImageNo,VCS_RES *pRes);
extern "C" _declspec(dllexport) int MPF_SF_Process( int iObrNo, int iImageNo,VCS_RES *pRes);




#endif
