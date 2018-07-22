#ifndef __COM_PORT_H__
#define __COM_PORT_H__

#define CLOSE_HANDLE(x)   if (x){	CloseHandle(x);	(x) = NULL;}

class ComPort
{
public:
	ComPort(void);
	~ComPort(void);


	BOOL IsPortOpen();

	BOOL InitPort(UINT nPort = 1, UINT baud = 115200, TCHAR parity = 'N', UINT databits = 8,
		UINT stopsbits = 1, DWORD dwCommEventsMask = EV_RXCHAR | EV_CTS);

	void ClosePort();

	DWORD WriteDatatoPort(BYTE* pu8Data,UINT32 u32Length);

	UINT  ReadDataFromPort(BYTE *pReadBuf,UINT32 u32Length);

	UINT SendDataAndRecv(BYTE* pWriteData,UINT32 u32WriteLen,BYTE *pReadBuf,UINT32 u32ReadLen,DWORD dwTimeout=5000);	

	DWORD GetCommEventsMask();

	void SaveLog(TCHAR *pszText);

	int WideCharToChar(WCHAR * pwchar, char * pchar);

	int CharToWideChar(char * pchar,WCHAR * pwchar);

	void Init();

public:

	HANDLE m_hCom;
	UINT m_nPort;
	DWORD m_dwCommEventsMask;
	OVERLAPPED m_osRead,m_osWrite;

};



#endif