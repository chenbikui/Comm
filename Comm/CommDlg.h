// CommDlg.h : ͷ�ļ�
//

#pragma once

#include "SerialPort.h"
#include "ComPort.h"

// CCommDlg �Ի���
class CCommDlg : public CDialog
{
// ����
public:
	CCommDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_COMM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

	CSerialPort m_csPort;
	ComPort m_comPort;

// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
