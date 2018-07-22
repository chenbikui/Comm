#include "StdAfx.h"
#include "ComPort.h"

ComPort::ComPort(void)
{
	Init();
}

ComPort::~ComPort(void)
{
	ClosePort();
}

void ComPort:: Init()
{
	m_hCom = NULL;
	m_nPort = 1;
	m_dwCommEventsMask = 0;
	memset(&m_osRead,0,sizeof(m_osRead));
	memset(&m_osWrite,0,sizeof(m_osWrite));
}

int ComPort::CharToWideChar(char * pchar,WCHAR * pwchar)
{
	int dwnum = MultiByteToWideChar(CP_ACP,0,pchar,-1,NULL,0);
	MultiByteToWideChar(CP_ACP,0,pchar,-1,pwchar,dwnum);
	pwchar[dwnum]='\0';
	return dwnum-1;
}

int ComPort::WideCharToChar(WCHAR * pwchar, char * pchar)
{
	int dwnum = WideCharToMultiByte(CP_ACP,0,pwchar,-1,NULL,0,NULL,FALSE);
	WideCharToMultiByte(CP_ACP,0,pwchar,-1,pchar,dwnum,NULL,FALSE);
	pchar[dwnum]='\0';
	return dwnum-1;
}

void ComPort::SaveLog(TCHAR *pszText)
{
	char szWrite[256]={0};

#ifdef UNICODE
	WideCharToChar(pszText, szWrite);
#else
	memcpy(szWrite,pszText,_tcslen(pszText));
#endif

	DWORD nCount =0;
	char szPath[MAX_PATH] = {0};
	GetCurrentDirectoryA(sizeof(szPath),szPath);
	strcat_s(szPath,"\\log.txt");
	FILE *logFile=NULL;
	if (0 != fopen_s(&logFile,szPath,"ab"))
	{
		return ;
	}
	fseek(logFile,0,SEEK_END);
	int nLen = strlen(szWrite);
	nCount = fwrite(szWrite,nLen,1,logFile);
	if (!nCount)
	{
		fclose(logFile);
		logFile = NULL;
		return;
	}
	fclose(logFile);
	logFile = NULL;
}


BOOL ComPort::IsPortOpen()
{
	if (NULL == m_hCom)
		return FALSE;
	else
	    return TRUE;
}

BOOL ComPort::InitPort(UINT nPort/* = 1*/, UINT baud/* = 115200*/, TCHAR parity /*= 'N'*/, 
					   UINT databits/* = 8*/, UINT stopsbits/* = 1*/, DWORD dwCommEventsMask/* = EV_RXCHAR | EV_CTS*/)
{
	if (IsPortOpen())
	{
		return TRUE;
	}

	m_nPort = nPort;
	m_dwCommEventsMask = dwCommEventsMask;

	BOOL bResult = FALSE;
	TCHAR szPort[256] = {0};
	TCHAR szBaud[256] = {0};

	if (m_hCom != NULL)
	{
		CLOSE_HANDLE(m_hCom);
	}

	_stprintf_s(szPort, _T("\\\\.\\COM%d"),nPort);
	_stprintf_s(szBaud, _T("%d,%c,%d,%d"), baud, parity, databits, stopsbits);

	m_hCom = CreateFile(szPort,
		GENERIC_READ | GENERIC_WRITE,
		0,								// comm devices must be opened with exclusive access
		NULL,							// no security attributes
		OPEN_EXISTING,					// comm devices must use OPEN_EXISTING
        FILE_FLAG_OVERLAPPED,			// Async I/O
		0);							// template must be 0 for comm devices

	if (m_hCom == INVALID_HANDLE_VALUE)
	{
		m_hCom = NULL;
		return FALSE;
	}

	COMMTIMEOUTS CommTimeouts;
	// set the timeout values
	CommTimeouts.ReadIntervalTimeout = MAXDWORD;/*1000*/;
	CommTimeouts.ReadTotalTimeoutConstant = 0/*1000*/;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0/*1000*/;
	CommTimeouts.WriteTotalTimeoutMultiplier = 50/*1000*/;
	CommTimeouts.WriteTotalTimeoutConstant = 2000/*1000*/;

	if (!SetCommTimeouts(m_hCom, &CommTimeouts))
	{
		return FALSE;
	}		   
	if (!SetCommMask(m_hCom, dwCommEventsMask))
	{
		return FALSE;
	}

	DCB dcb;
	if (!GetCommState(m_hCom, &dcb))
	{
		return FALSE;
	}

	if (!BuildCommDCB(szBaud, &dcb))
	{
		return FALSE;
	}
	if (!SetCommState(m_hCom, &dcb))
	{
		return FALSE;
	}

	// flush the port
	PurgeComm(m_hCom, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	TRACE(_T("Initialisation for communication port COM%d completed\n"), nPort);

	return TRUE;
}

void ComPort::ClosePort()
{
	CLOSE_HANDLE(m_hCom);
	TRACE(_T("closeport()\n"));
}

DWORD ComPort::WriteDatatoPort(BYTE* pu8Data,UINT32 u32Length)
{
	if (NULL == m_hCom) return 0;

	// Clear buffer
	PurgeComm(m_hCom, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	DWORD dwBytesWritten = 0;
	BOOL bResult = WriteFile(m_hCom,pu8Data, u32Length, &dwBytesWritten,&m_osWrite);

	if( !bResult)
	{
		if ( GetLastError() == ERROR_IO_PENDING ) 
		{
			bResult = GetOverlappedResult(m_hCom,&m_osWrite,&dwBytesWritten,TRUE);
			TRACE(_T("ERROR_IO_PENDING bResult:%d,dwBytesWritten:%d\n"),bResult,dwBytesWritten);
			if(!bResult)
			{
				TRACE(_T("WriteFile GetOverlappedResult() error\n"));
				return 0;
			}
		}
		else
		{
			TRACE(_T("WriteFile() error\n"));
			return 0;
		}
		    
	}
	return dwBytesWritten;
}

UINT ComPort::ReadDataFromPort(BYTE *pReadBuf,UINT32 u32Length)
{
	if (NULL == m_hCom) return 0;
	memset(pReadBuf,0,u32Length);

	BOOL bResult = FALSE;;
	DWORD dwErrorFlags = 0;
	DWORD dwBytesRead = 0;
	COMSTAT ComStat;
	ClearCommError(m_hCom, &dwErrorFlags, &ComStat );

	if( !ComStat.cbInQue ) return 0;

	bResult = ReadFile(m_hCom,pReadBuf, u32Length, &dwBytesRead, &m_osRead);
	if(!bResult)
	{
		if(GetLastError() == ERROR_IO_PENDING )
		{
			bResult = GetOverlappedResult(m_hCom,&m_osRead,&dwBytesRead,TRUE);
			TRACE(_T("ERROR_IO_PENDING bResult:%d,dwBytesRead:%d\n"),bResult,dwBytesRead);
			if(!bResult)
			{
				TRACE(_T("ReadFile GetOverlappedResult() error\n"));
				return 0;
			}
		}	
		else
		{
			TRACE(_T("ReadFile() error\n"));
			return 0;
		}	
	}

	return dwBytesRead;
}

UINT ComPort::SendDataAndRecv(BYTE* pWriteData,UINT32 u32WriteLen,BYTE *pReadBuf,UINT32 u32ReadLen,DWORD dwTimeout/*=5000*/)
{
	memset(pReadBuf,0,u32ReadLen);
	DWORD dwRead = 0;

	DWORD dwRet = WriteDatatoPort(pWriteData,u32WriteLen);
	if (dwRet<1)
	{
		return 0;
	}
	dwRet  = 0;
	while (dwRet < dwTimeout)
	{
		dwRead = ReadDataFromPort(pReadBuf,u32ReadLen);
		if (dwRead < 1)
		{
			Sleep(50);
			dwRet+=50;
			continue;
		}

		return dwRead;//ok
	} 
	return 0;
}


DWORD ComPort::GetCommEventsMask()
{
	return m_dwCommEventsMask;
}
