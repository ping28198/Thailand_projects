#include "CColorStatic.h"



CColorStatic::CColorStatic()
{
	m_Brush.CreateSolidBrush(RGB(255,0,0));
}


CColorStatic::~CColorStatic()
{
}
BEGIN_MESSAGE_MAP(CColorStatic, CStatic)
	ON_WM_CTLCOLOR()
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()




HBRUSH CColorStatic::CtlColor(CDC* pDC, UINT nCtlColor)
{
	// TODO:  �ڴ˸��� DC ���κ�����

	if (CTLCOLOR_STATIC == nCtlColor)
	{
		pDC->SelectObject(&m_font);
		pDC->SetTextColor(m_crText);
		pDC->SetBkColor(m_crBackColor);
		if (m_bTran == true)
			pDC->SetBkMode(TRANSPARENT);
	}
	return  (HBRUSH)m_Brush;


	// TODO:  �����Ӧ���ø����Ĵ�������򷵻ط� null ����
}

void CColorStatic::SetTextColor(COLORREF crText)
{
	m_crText = crText;
	RedrawWindow();
}

void CColorStatic::SetFontSize(int nSize)
{
	nSize *= -1;
	m_lf.lfHeight = nSize;
	m_font.DeleteObject();
	m_font.CreateFontIndirect(&m_lf);
	RedrawWindow();
}

void CColorStatic::SetBackColor(COLORREF crBackColor)
{
	m_Brush.CreateSolidBrush(crBackColor);
	m_crBackColor = crBackColor;
	RedrawWindow();
}

void CColorStatic::SetTransparent(bool bTran)
{
	m_bTran = bTran;
	RedrawWindow();
}