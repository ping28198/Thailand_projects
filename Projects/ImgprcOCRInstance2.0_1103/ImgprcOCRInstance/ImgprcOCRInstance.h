// ImgprcOCRInstance.h : ImgprcOCRInstance Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������


// CImgprcOCRInstanceApp:
// �йش����ʵ�֣������ ImgprcOCRInstance.cpp
//

class CImgprcOCRInstanceApp : public CWinApp
{
public:
	CImgprcOCRInstanceApp();


// ��д
public:
	virtual BOOL InitInstance();

// ʵ��
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnAppExit();
};

extern CImgprcOCRInstanceApp theApp;