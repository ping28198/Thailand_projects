// ImgprcOCRInstanceDoc.h : CImgprcOCRInstanceDoc ��Ľӿ�
//


#pragma once

class CImgprcOCRInstanceDoc : public CDocument
{
protected: // �������л�����
	CImgprcOCRInstanceDoc();
	DECLARE_DYNCREATE(CImgprcOCRInstanceDoc)

// ����
public:



// ����
public:

// ��д
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// ʵ��
public:
	virtual ~CImgprcOCRInstanceDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// ���ɵ���Ϣӳ�亯��
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void SetTitle(LPCTSTR lpszTitle);
};


