// ImgprcOCRInstanceDoc.cpp : CImgprcOCRInstanceDoc ���ʵ��
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


// CImgprcOCRInstanceDoc ����/����

CImgprcOCRInstanceDoc::CImgprcOCRInstanceDoc()
{
	// TODO: �ڴ����һ���Թ������

}

CImgprcOCRInstanceDoc::~CImgprcOCRInstanceDoc()
{
}

BOOL CImgprcOCRInstanceDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: �ڴ�������³�ʼ������
	// (SDI �ĵ������ø��ĵ�)

	return TRUE;
}




// CImgprcOCRInstanceDoc ���л�

void CImgprcOCRInstanceDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: �ڴ���Ӵ洢����
	}
	else
	{
		// TODO: �ڴ���Ӽ��ش���
	}
}


// CImgprcOCRInstanceDoc ���

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


// CImgprcOCRInstanceDoc ����
