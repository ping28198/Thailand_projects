// ImgprcOCRInstanceDoc.cpp : CImgprcOCRInstanceDoc 类的实现
//

#include "stdafx.h"
#include "ImgprcOCRInstance.h"

#include "ImgprcOCRInstanceDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CImgprcOCRInstanceDoc

IMPLEMENT_DYNCREATE(CImgprcOCRInstanceDoc, CDocument)

BEGIN_MESSAGE_MAP(CImgprcOCRInstanceDoc, CDocument)
END_MESSAGE_MAP()


// CImgprcOCRInstanceDoc 构造/析构

CImgprcOCRInstanceDoc::CImgprcOCRInstanceDoc()
{
	// TODO: 在此添加一次性构造代码

}

CImgprcOCRInstanceDoc::~CImgprcOCRInstanceDoc()
{
}

BOOL CImgprcOCRInstanceDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: 在此添加重新初始化代码
	// (SDI 文档将重用该文档)

	return TRUE;
}




// CImgprcOCRInstanceDoc 序列化

void CImgprcOCRInstanceDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: 在此添加存储代码
	}
	else
	{
		// TODO: 在此添加加载代码
	}
}


// CImgprcOCRInstanceDoc 诊断

#ifdef _DEBUG
void CImgprcOCRInstanceDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CImgprcOCRInstanceDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CImgprcOCRInstanceDoc 命令
