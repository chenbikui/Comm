// CommDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "Comm.h"
#include "CommDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CCommDlg �Ի���




CCommDlg::CCommDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCommDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCommDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCommDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_NO_BLOCK, &CCommDlg::OnBnClickedButtonNoBlock)
	ON_MESSAGE(WM_COMM_RXCHAR,GetCommData)
	ON_BN_CLICKED(IDC_BUTTON_BLOCK, &CCommDlg::OnBnClickedButtonBlock)
END_MESSAGE_MAP()


// CCommDlg ��Ϣ�������

BOOL CCommDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CCommDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CCommDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CCommDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


LRESULT CCommDlg::GetCommData(WPARAM wParam,LPARAM lParam)
{
	return 0;
}


void CCommDlg::OnBnClickedButtonNoBlock()
{
	unsigned char cRead[256] ={0};
	int iRet=0,iLength = 3;
	int iCmdLength = iLength +1;
	BYTE cSend1[256] = {0x55,0xaa,0xf0,0xf8,0x00,0x04,0x01,0x25,0x02,0x02,0x28};
	BYTE cSend2[256] = {0x55,0xaa,0xf0,0xf8 ,0x00,0x08,0x04,0x25,0x03,0x02,0x0e,0x08,0xa4,0xc7,0x45};
	BYTE cSend3[256] = {0x55,0xaa,0xf0,0xf8,0x00,0x03,0x07,0x21,0x05,0x28};
	BYTE cSend4[256] = {0x55,0xaa,0xf0,0xf8,0x00,0x03,0x09,0x21,0x03,0x20};
    CString str,strShow;

	if (!m_csPort.IsPortOpen())
	{
		if (m_csPort.InitPort(this,28,19200))
		{
			if (!m_csPort.StartMonitoring())
			{
				m_csPort.ClosePort();
				return;
			}
		}
	}

   

	int nLen = m_csPort.SendDataAndRecv(cSend1,11,cRead,sizeof(cRead));Sleep(200);
	
	if (nLen > 0)
	{
		for (int i=0;i<nLen;i++)
		{
			str.Format(_T("%02x,"),cRead[i]);
			strShow +=str;
		}
		
	}
//	else
	{
		m_csPort.ClosePort();
		AfxMessageBox(strShow);
		return;
	}
	strShow +=_T("\n");

    cSend2[10]=cRead[10];
	cSend2[11]=cRead[11];
	cSend2[12]=cRead[12];
	cSend2[13]=cRead[13];

	nLen = m_csPort.SendDataAndRecv(cSend2,15,cRead,sizeof(cRead));
	if (nLen > 0)
	{
		for (int i=0;i<nLen;i++)
		{
			str.Format(_T("%02x,"),cRead[i]);
			strShow +=str;
		}

	}
	else
	{
		m_csPort.ClosePort();
		AfxMessageBox(strShow);
		return;
	}
	strShow +=_T("\n");

	/*nLen = m_csPort.SendDataAndRecv(cSend3,10,cRead,sizeof(cRead));
	if (nLen > 0)
	{
		for (int i=0;i<nLen;i++)
		{
			str.Format(_T("%02x,"),cRead[i]);
			strShow +=str;
		}

	}
	else
	{
		m_csPort.ClosePort();
		AfxMessageBox(strShow);
		return;
	}
	strShow +=_T("\n");*/

	/*nLen = m_csPort.SendDataAndRecv(cSend4,10,cRead,sizeof(cRead));
	if (nLen > 0)
	{
		for (int i=0;i<nLen;i++)
		{
			str.Format(_T("%02x,"),cRead[i]);
			strShow +=str;
		}

	}*/

	m_csPort.ClosePort();
	AfxMessageBox(strShow);
	
}

void CCommDlg::OnBnClickedButtonBlock()
{
	unsigned char cRead[256] ={0};
	int iRet=0,iLength = 3;
	int iCmdLength = iLength +1;
	BYTE cSend1[256] = {0x55,0xaa,0xf0,0xf8,0x00,0x04,0x01,0x25,0x02,0x02,0x28};
	BYTE cSend2[256] = {0x55,0xaa,0xf0,0xf8 ,0x00,0x08,0x04,0x25,0x03,0x02,0x0e,0x08,0xa4,0xc7,0x45};
	BYTE cSend3[256] = {0x55,0xaa,0xf0,0xf8,0x00,0x03,0x07,0x21,0x05,0x28};
	BYTE cSend4[256] = {0x55,0xaa,0xf0,0xf8,0x00,0x03,0x09,0x21,0x03,0x20};
    CString str,strShow;

	if (!m_comPort.IsPortOpen())
	{
		if (!m_comPort.InitPort(28))
		{
			return;
		}
	}

	int nLen = m_comPort.SendDataAndRecv(cSend1,11,cRead,sizeof(cRead));
	
	if (nLen > 0)
	{
		for (int i=0;i<nLen;i++)
		{
			str.Format(_T("%02x,"),cRead[i]);
			strShow +=str;
		}
		
	}
	else
	{
		m_comPort.ClosePort();
		AfxMessageBox(strShow);
		return;
	}
	strShow +=_T("\n");

    cSend2[10]=cRead[10];
	cSend2[11]=cRead[11];
	cSend2[12]=cRead[12];
	cSend2[13]=cRead[13];

	nLen = m_comPort.SendDataAndRecv(cSend2,15,cRead,sizeof(cRead));
	if (nLen > 0)
	{
		for (int i=0;i<nLen;i++)
		{
			str.Format(_T("%02x,"),cRead[i]);
			strShow +=str;
		}

	}
	else
	{
		m_comPort.ClosePort();
		AfxMessageBox(strShow);
		return;
	}
	strShow +=_T("\n");

	nLen = m_comPort.SendDataAndRecv(cSend3,10,cRead,sizeof(cRead));
	if (nLen > 0)
	{
		for (int i=0;i<nLen;i++)
		{
			str.Format(_T("%02x,"),cRead[i]);
			strShow +=str;
		}

	}
	else
	{
		m_comPort.ClosePort();
		AfxMessageBox(strShow);
		return;
	}
	strShow +=_T("\n");

	nLen = m_comPort.SendDataAndRecv(cSend4,10,cRead,sizeof(cRead));
	if (nLen > 0)
	{
		for (int i=0;i<nLen;i++)
		{
			str.Format(_T("%02x,"),cRead[i]);
			strShow +=str;
		}

	}

	m_comPort.ClosePort();
	AfxMessageBox(strShow);
}
