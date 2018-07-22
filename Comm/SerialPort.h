
#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#define WM_COMM_BREAK_DETECTED		WM_USER+1	// A break was detected on input.
#define WM_COMM_CTS_DETECTED		WM_USER+2	// The CTS (clear-to-send) signal changed state. 
#define WM_COMM_DSR_DETECTED		WM_USER+3	// The DSR (data-set-ready) signal changed state. 
#define WM_COMM_ERR_DETECTED		WM_USER+4	// A line-status error occurred. Line-status errors are CE_FRAME, CE_OVERRUN, and CE_RXPARITY. 
#define WM_COMM_RING_DETECTED		WM_USER+5	// A ring indicator was detected. 
#define WM_COMM_RLSD_DETECTED		WM_USER+6	// The RLSD (receive-line-signal-detect) signal changed state. 
#define WM_COMM_RXCHAR				WM_USER+7	// A character was received and placed in the input buffer. 
#define WM_COMM_RXFLAG_DETECTED		WM_USER+8	// The event character was received and placed in the input buffer.  
#define WM_COMM_TXEMPTY_DETECTED	WM_USER+9	// The last character in the output buffer was sent.  

#define MAX_WRITE_BUF_SIZE     1024   //��������С��Ĭ��1024
#define MAX_READ_BUF_SIZE      1024   //��������С��Ĭ��1024

#define CLOSE_HANDLE(x)   if (x){	CloseHandle(x);	(x) = NULL;}

class CSerialPort
{														 
public:
	// contruction and destruction
	CSerialPort();
	virtual		~CSerialPort();

	// port initialisation	
	/*
	* Function: InitPort(CWnd* pPortOwner, UINT nPort = 1, UINT baud = 115200, TCHAR parity = 'N', UINT databits = 8, UINT stopsbits = 1, DWORD dwCommEvents = EV_RXCHAR | EV_CTS);
	* Para:   pPortOwner ---- ���ڵ�ӵ���߾��
	*		  portnr -------- ���ںţ�
	*         baud   --------  ���ڲ����ʣ��ӻ�Ϊ115200
	*		  parity --------   У�鷽ʽ����ǰ�ӻ��ķ�ʽΪ'N'
	*		  databits ------   ����λ������ǰ�ӻ�Ϊ8
	*		  stopsbits -----   ��ǰΪ1
	*		  dwCommEvents ----- Ӧ�ò�����Ϣ�¼�,EV_RXCHAR
	* Return: TRUE ---- �򿪴��ڳɹ�
	*		  FALSE ----�򿪴���ʧ��
	* Description: �򿪴��ڣ������úô��ڲ���
	*/
	BOOL InitPort(CWnd* pPortOwner, UINT nPort = 1, UINT baud = 115200, TCHAR parity = 'N', UINT databits = 8, UINT stopsbits = 1, DWORD dwCommEvents = EV_RXCHAR | EV_CTS);
	/*
	*Function: void OnReceviedChar(WPARAM wpRam,LPARAM lpRam)
	*Para: wpRam ---- ��Ϣ������յ����ݡ�
	*	   lpRam ---- ��Ϣ�����������δ�õ�
	*Return: None
	*Description: ���ڽ����ַ���Ϣ��Ӧ�������˺���Ϊ�麯������������д�滻������ǰ������Ϊ��
	*/
	void OnReceviedChar(WPARAM wpRam,LPARAM lpRam);
	// start/stop comm watching
	/*
	*Function: BOOL		StartMonitoring();
	*Para: NONE
	*Return: None
	*Description: �������ڹ����̣߳����ҿ����ں��������������ϵͳ������ӦӦ�ô��ڵ��κ���Ϣ
	*/
	BOOL		StartMonitoring();
	/*
	*Function: BOOL		RestartMonitoring();
	*Para: NONE
	*Return: None
	*Description: �����������ڹ����̣߳������ڵ���StopMonitoring()֮�󣬿ɵ��øú��������������߳�
	*/
	BOOL		RestartMonitoring();
	/*
	*Function: BOOL		StopMonitoring();
	*Para: NONE
	*Return: None
	*Description: �������߳���������ֹͣ�̹߳���
	*/
	BOOL		StopMonitoring();

	DWORD		GetCommEvents();
	DCB			GetDCB();
   
	/*
	*Function: BOOL IsPortOpen();
	*Para: NONE
	*Return:  TRUE ----�����Ѵ�
	*		  FALSE -----�����ѹر�
	*Description: ��⴮���Ƿ��Ѵ�
	*/
	BOOL IsPortOpen();
	/*
	*Function: void ClosePort();
	*Para: NONE
	*Return:  NONE
	*Description: �رյ�ǰ�򿪵Ĵ���
	*/
	void ClosePort();
	/*
	*Function: BOOL WriteDatatoPort(BYTE* pu8Data,UINT32 u32Length);	
	*Para: pu8Data ---- Ҫ���͵�����ָ��
	*      u32Length ----  ����ĳ���
	*Return:  NONE
	*Description: ������Ϊu32Length������pu8Dataͨ�����ڷ��ͳ�ȥ��
	*/
	BOOL WriteDatatoPort(BYTE* pu8Data,UINT32 u32Length);	

	/*
	*Function: BOOL		WriteStringToPort(TCHAR* pszString);
	*Para: string ----Ҫд����ַ���
	*Return:  NONE
	*Description: ���ַ���string���ͳ�ȥ
	*/
	BOOL  WriteStringToPort(TCHAR* pszString);

	/************************************************************************/
	/*Para: pReadBuf---�������ݻ�����
		    dwLen---����������
	*Return:  �������ݳ���
	*Description: ��������                                                                   */
	/************************************************************************/
	UINT  ReadData(unsigned char *pReadBuf,DWORD dwLen,DWORD dwTimeout=5000);

	UINT SendDataAndRecv(BYTE* pu8Data,UINT32 u32Length,unsigned char *pReadBuf,DWORD dwLen,DWORD dwTimeout=5000);	

	void SaveLog(TCHAR *pszText);
	int WideCharToChar(WCHAR * pwchar, char * pchar);
	int CharToWideChar(char * pchar,WCHAR * pwchar);


	BOOL WaitWriteFinish(UINT uTimes = 10000);
	void	ReceiveChar(COMSTAT comstat);
	void	WriteChar();
	
public:
	// thread
	HANDLE			m_hThread;

	// synchronisation objects
	CRITICAL_SECTION	m_csCommunicationSync;
	BOOL				m_bThreadAlive;

	// handles
	HANDLE				m_hShutdownEvent;
	HANDLE				m_hComm;
	HANDLE				m_hWriteEvent;

	// Event array. 
	// One element is used for each event. There are two event handles for each port.
	// A Write event and a receive character event which is located in the overlapped structure (m_ov.hEvent).
	// There is a general shutdown when the port is closed. 
	HANDLE				m_hEventArray[3];
	HANDLE				m_hMutxOfSerial;
	
	// structures
	OVERLAPPED			m_ov;
	COMMTIMEOUTS		m_CommTimeouts;
	DCB					m_dcb;

	// owner window
	CWnd*				m_pOwner;

	// misc
	UINT				m_nPort;

	BYTE				m_szWriteBuffer[MAX_WRITE_BUF_SIZE];
	UINT				m_u32SizeOfWriteBuffer;
	UINT				m_u32LengthOfBuffer;
	DWORD				m_dwCommEvents;
	BOOL				m_blWriteFlag;
	BOOL				m_blReadFlag;
	UINT				m_u32LengthOfReadData;
	BYTE				m_szReadBuffer[MAX_READ_BUF_SIZE];
};

#endif __SERIALPORT_H__


