
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

#define MAX_WRITE_BUF_SIZE     1024   //缓冲区大小，默认1024
#define MAX_READ_BUF_SIZE      1024   //缓冲区大小，默认1024

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
	* Para:   pPortOwner ---- 串口的拥有者句柄
	*		  portnr -------- 串口号，
	*         baud   --------  串口波特率，从机为115200
	*		  parity --------   校验方式，当前从机的方式为'N'
	*		  databits ------   数据位数，当前从机为8
	*		  stopsbits -----   当前为1
	*		  dwCommEvents ----- 应该产生消息事件,EV_RXCHAR
	* Return: TRUE ---- 打开串口成功
	*		  FALSE ----打开串口失败
	* Description: 打开串口，并配置好串口参数
	*/
	BOOL InitPort(CWnd* pPortOwner, UINT nPort = 1, UINT baud = 115200, TCHAR parity = 'N', UINT databits = 8, UINT stopsbits = 1, DWORD dwCommEvents = EV_RXCHAR | EV_CTS);
	/*
	*Function: void OnReceviedChar(WPARAM wpRam,LPARAM lpRam)
	*Para: wpRam ---- 消息传入接收的数据。
	*	   lpRam ---- 消息传入参数，暂未用到
	*Return: None
	*Description: 串口接收字符消息响应函数。此函数为虚函数可以子类重写替换掉。当前函数体为空
	*/
	void OnReceviedChar(WPARAM wpRam,LPARAM lpRam);
	// start/stop comm watching
	/*
	*Function: BOOL		StartMonitoring();
	*Para: NONE
	*Return: None
	*Description: 启动串口管理线程，在找开串口后必须启动，否则系统将不响应应该串口的任何消息
	*/
	BOOL		StartMonitoring();
	/*
	*Function: BOOL		RestartMonitoring();
	*Para: NONE
	*Return: None
	*Description: 重新启动串口管理线程，当串口调用StopMonitoring()之后，可调用该函数来重启管理线程
	*/
	BOOL		RestartMonitoring();
	/*
	*Function: BOOL		StopMonitoring();
	*Para: NONE
	*Return: None
	*Description: 当串口线程已启动，停止线程管理
	*/
	BOOL		StopMonitoring();

	DWORD		GetCommEvents();
	DCB			GetDCB();
   
	/*
	*Function: BOOL IsPortOpen();
	*Para: NONE
	*Return:  TRUE ----串口已打开
	*		  FALSE -----串口已关闭
	*Description: 检测串口是否已打开
	*/
	BOOL IsPortOpen();
	/*
	*Function: void ClosePort();
	*Para: NONE
	*Return:  NONE
	*Description: 关闭当前打开的串口
	*/
	void ClosePort();
	/*
	*Function: BOOL WriteDatatoPort(BYTE* pu8Data,UINT32 u32Length);	
	*Para: pu8Data ---- 要发送的数组指针
	*      u32Length ----  数组的长度
	*Return:  NONE
	*Description: 将长度为u32Length的数组pu8Data通过串口发送出去。
	*/
	BOOL WriteDatatoPort(BYTE* pu8Data,UINT32 u32Length);	

	/*
	*Function: BOOL		WriteStringToPort(TCHAR* pszString);
	*Para: string ----要写入的字符串
	*Return:  NONE
	*Description: 将字符串string发送出去
	*/
	BOOL  WriteStringToPort(TCHAR* pszString);

	/************************************************************************/
	/*Para: pReadBuf---接收数据缓冲区
		    dwLen---缓冲区长度
	*Return:  接收数据长度
	*Description: 接收数据                                                                   */
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


