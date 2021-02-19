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
	COLORREF m_crText;          // 字体颜色  
	COLORREF m_crBackColor;     // 背景颜色  
	HBRUSH   m_hBrush;          // 画刷  
	LOGFONT  m_lf;              // 字体大小  
	CFont    m_font;            // 字体

	bool    m_bTran;            // 是否透明
public:
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
};

