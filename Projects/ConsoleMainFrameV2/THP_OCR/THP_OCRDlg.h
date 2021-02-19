
// THP_OCRDlg.h: 头文件
//

#pragma once
#include <vector>
#include <string>
#include "CColorStatic.h"
#include "MainWorker.h"
#include "Timer.h"
// CTHPOCRDlg 对话框
class CTHPOCRDlg : public CDialogEx
{
// 构造
public:
	CTHPOCRDlg(CWnd* pParent = nullptr);	// 标准构造函数
	~CTHPOCRDlg();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THP_OCR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持



public:
	size_t m_total_num = 0;
	size_t m_ok_num = 0;
	float m_ok_percent = 0;
	bool m_connect_state = 0;

	Timer m_MMT1;


	void on_connect_state(int state = 0);
	void update_statistic_info(size_t total_num, size_t ok_num);

	OCR_MainWoker *pMainWorker;

	void update_state();
	void ouput_msg(const std::string &msg);
	CString s_tmp;

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:


	CStatic st_ocrid_ctr;
	CStatic st_ip_ctr;

	afx_msg void OnBnClickedButton1();
	CColorStatic st_connect_state;
	CEdit m_edit_msg_box;


	afx_msg void OnBnClickedModeNormal();
	afx_msg void OnBnClickedModeTest();
	int ratio_mode;
	CStatic st_total_num_ctr;
	CStatic st_ok_num_ctr;
	CStatic st_percent_ctr;
	afx_msg void OnClose();
};
