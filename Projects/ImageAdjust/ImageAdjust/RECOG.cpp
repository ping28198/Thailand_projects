/* 
版本号：  1.2.0.3
发行日期：2013-07-11  
作者：    徐海堰 
*/

#include "stdafx.h"
#include <math.h>
#include "RECOG.h"

_declspec(dllexport) BYTE* HW_SlantImage(BYTE *,int &,int &);
_declspec(dllexport) int WINAPI LineAndWordSegment(BYTE *pBlockImage,int nWidth,int nHeight,char *lib_dir);
_declspec(dllexport) void FreeSlantImage();

/////////////////////////////////////////////////////////////////////////////

C_RECOG::C_RECOG(BYTE *mpBIMG,WORD mwPixels_X,WORD mwPixels_Y)
{
	// Initialization
	pBIMG_HWPrint=NULL;
	Reset_Check();

	// Set image varible
	pBIMG=mpBIMG;
	wPixels_X=mwPixels_X;
	wPixels_Y=mwPixels_Y;  
}

C_RECOG::~C_RECOG()
{
	Reset_Check();
}

void C_RECOG::Reset_Check()
{
    pBIMG=NULL;
	wPixels_X=wPixels_Y=0;  
	
	bFlag_HWPrint=0xff;   
	if (pBIMG_HWPrint!=NULL)
	{
		delete pBIMG_HWPrint;
		pBIMG_HWPrint=NULL;
	}
	RECT_HWPrint.top=RECT_HWPrint.bottom=RECT_HWPrint.left=RECT_HWPrint.right=0;
	wPixels_X_HWPrint=wPixels_Y_HWPrint=0;
	dwTime_HWPrint=0;
	sFullPath_HWPrint="";

	// OCR result
	bRESRowNUM_HWPrint=0;
	sRES_HWPrint="";
}

/////////////////////////////////////////////////////////////////////////////

CString C_RECOG::GetRECOGRES()
{
	BOOL BOOLFlag;
	CString sRead,sRTN="";
	CStdioFile SFile;

	if (bFlag_HWPrint!=3) return sRTN;

	BOOLFlag=SFile.Open(sFullPath_HWPrint+"Temp00.txt",CFile::modeNoTruncate|CFile::modeRead|CFile::typeText);
	if (BOOLFlag==TRUE) 
	{
		while(SFile.ReadString(sRead)!=FALSE)
		{
			if (sRead!="")
			{
				if (sRTN!="") sRTN+="[R]";
				sRTN+=sRead;

				if (bRESRowNUM_HWPrint<100) asRES_HWPrint[bRESRowNUM_HWPrint++]=sRead;
			}
		}  // End of while
		SFile.Close();
	}  // End of if
	
	sRES_HWPrint=sRTN;
	return sRTN;
}

void C_RECOG::RECOG_HWPrint_Check(char *mPath)
{
	int nX,nY;
	BYTE *p,bStep=0;
	DWORD dwTemp;
	
	if (bFlag_HWPrint || pBIMG_HWPrint==NULL || !wPixels_X_HWPrint || !wPixels_Y_HWPrint) return;
	
	sFullPath_HWPrint.Format("%s",mPath);	
	if (sFullPath_HWPrint=="") return;
	else
	{
		if (sFullPath_HWPrint.Right(1)!="\\") sFullPath_HWPrint+="\\";
	}

	nX=int(wPixels_X_HWPrint);
	nY=int(wPixels_Y_HWPrint);
	
	dwTemp=GetTickCount();
	try
	{
		p=HW_SlantImage(pBIMG_HWPrint,nX,nY);
		bStep=1;
		LineAndWordSegment(p,nX,nY,mPath);
		bFlag_HWPrint=3;
	}
	catch(...)
	{
		if (bStep==0) bFlag_HWPrint=4;
		else bFlag_HWPrint=5;
	}
	FreeSlantImage();
	dwTime_HWPrint=GetTickCount()-dwTemp;
}

void C_RECOG::TRANS_HWPrint_Check(RECT mRECT)
{
    int nPOS;
	BYTE *p,bEnlarge=8;  // 由于汉王的原因要将区域X方向的像素数量扩大为8的整数倍 
	WORD i,j,k,wTemp,wTemp2;
	
	if (bFlag_HWPrint!=0xff) return;
	
	if (pBIMG==NULL || wPixels_X==0 || wPixels_Y==0) 
	{
		bFlag_HWPrint=1;  // Error
		return;
	}

    RECT_HWPrint=mRECT;
    if (RECT_HWPrint.top<0) RECT_HWPrint.top=0;
	if (RECT_HWPrint.bottom>=wPixels_Y) RECT_HWPrint.bottom=wPixels_Y-1;
    if (RECT_HWPrint.left<0) RECT_HWPrint.left=0;
	if (RECT_HWPrint.right>=wPixels_X) RECT_HWPrint.right=wPixels_X-1;
    if (RECT_HWPrint.top>=RECT_HWPrint.bottom || RECT_HWPrint.left>=RECT_HWPrint.right)
	{
		bFlag_HWPrint=2;  // Error
		return;
	}
	
	wPixels_X_HWPrint=WORD(RECT_HWPrint.right-RECT_HWPrint.left+1);
	wPixels_Y_HWPrint=WORD(RECT_HWPrint.bottom-RECT_HWPrint.top+1);
	while (wPixels_X_HWPrint%bEnlarge!=0) wPixels_X_HWPrint++;

	wTemp=wPixels_X_HWPrint/bEnlarge;
	pBIMG_HWPrint=new BYTE[wTemp*wPixels_Y_HWPrint];

    for (i=0;i<wPixels_Y_HWPrint;i++)
	for (j=0;j<wTemp;j++)
	{
		p=pBIMG_HWPrint+i*wTemp+j;
		*p=0;
		for (k=0;k<8;k++)
		{
			wTemp2=RECT_HWPrint.left+j*8+k;
			if (wTemp2>=wPixels_X) break;  // Exit for
			
			nPOS=(RECT_HWPrint.top+i)*wPixels_X+wTemp2;
			if (*(pBIMG+nPOS)) *p=*p+(BYTE)(pow(2,(7-k)));
		}
	}

	bFlag_HWPrint=0;  // OK
}
