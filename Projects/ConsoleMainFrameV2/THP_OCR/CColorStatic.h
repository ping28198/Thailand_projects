#pragma once
#include <afxwin.h>
class CColorStatic :
	public CStatic
{
public:
	CColorStatic();
	~CColorStatic();
	DECLARE_MESSAGE_MAP()

public:
	void SetTextColor(COLORREF crText);
	void SetFontSize(int nSize);
	void SetBackColor(COLORREF crBackColor);
	void SetTransparent(bool bTran);



protected:
	CBrush  m_Brush;
	COLORREF m_crText;          // ������ɫ  
	COLORREF m_crBackColor;     // ������ɫ  
	HBRUSH   m_hBrush;          // ��ˢ  
	LOGFONT  m_lf;              // �����С  
	CFont    m_font;            // ����

	bool    m_bTran;            // �Ƿ�͸��
public:
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
};

