#pragma once
typedef unsigned char       BYTE;

extern "C" BYTE XF_Hwr_Recg_One(BYTE *pbImg, int Type, int up, int down, int left, int right, int Num0, int Num1);
extern "C" BYTE CHQ_Hwr_Recg(BYTE *code_ptr, BYTE up, BYTE down, BYTE right, BYTE left,
	BYTE abc, short v1, short v2);
extern "C" int Prn_Frame_Recg(unsigned char *im6, unsigned char *result1, unsigned char *result2, unsigned char *result3);
extern "C" BYTE WZJ_Hwr_Recg(BYTE *code_ptr, BYTE *thin_ptr, BYTE *org_ptr,
	BYTE up, BYTE down, BYTE right, BYTE left, BYTE flag);
extern "C" int  XL_Hwr_Recg(BYTE *p1, BYTE *result, BYTE ovcs_up,
	BYTE ovcs_down, BYTE ovcs_left, BYTE ovcs_right);
extern "C" int  HWNR_RecognizeVcc(BYTE* pbBmp, int iType, int up, int down, int left, int right);

// DLL for SRI digit OCR
extern "C" void hgp_initial();
extern "C" void Load_tab();
extern "C" int WZJ_Hwr_Init(char *pszPath);