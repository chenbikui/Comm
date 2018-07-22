// CommDlg.h : 头文件
//

#pragma once

#include "SerialPort.h"
#include "ComPort.h"

// CCommDlg 对话框
class CCommDlg : public CDialog
{
// 构造
public:
	CCommDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_COMM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	CSerialPort m_csPort;
	ComPort m_comPort;

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
	afx_msg void OnBnClickedButtonNoBlock();
	afx_msg LRESULT GetCommData(WPARAM wParam,LPARAM lParam);
	afx_msg void OnBnClickedButtonBlock();
};
