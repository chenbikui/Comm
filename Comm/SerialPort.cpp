/*
**	FILENAME			CSerialPort.cpp
**
**	PURPOSE				This class can read, write and watch one serial port.
**						It sends messages to its owner when something happends on the port
**						The class creates a thread for reading and writing so the main
**						program is not blocked.
**
**	CREATION DATE		15-09-1997
**	LAST MODIFICATION	12-11-1997
**
**	AUTHOR				Remon Spekreijse
**
**
*/

#include "stdafx.h"
#include "SerialPort.h"
#include <process.h>
#include <assert.h>
 
//
// Constructor
//
CSerialPort::CSerialPort()
{
	m_hComm = NULL;
    m_hThread = NULL;
	// initialize overlapped structure members to zero
	m_ov.Offset = 0;
	m_ov.OffsetHigh = 0;

	// create events
	m_ov.hEvent = NULL;
	m_hWriteEvent = NULL;
	m_hShutdownEvent = NULL;
	for (int i = 0;i<3;i++)
	{
		m_hEventArray[i] = NULL;
	}

	memset(m_szWriteBuffer,0,sizeof(m_szWriteBuffer));
    memset(m_szReadBuffer,0,sizeof(m_szReadBuffer));
	m_bThreadAlive = FALSE;

	m_hMutxOfSerial = NULL;
	m_blWriteFlag = FALSE;
	m_blReadFlag = FALSE;
	m_pOwner = NULL;
	m_u32LengthOfBuffer = 0;
	m_u32LengthOfReadData = 0;
}

//
// Delete dynamic memory
//
CSerialPort::~CSerialPort()
{
	ClosePort();
}


//
// Initialize the port. This can be port 1 to 4.
//
BOOL CSerialPort::InitPort(CWnd* pPortOwner,UINT nPort,UINT  baud,TCHAR  parity,UINT  databits,
						   UINT  stopbits,DWORD dwCommEvents)
{
	// if the thread is alive: Kill
	//if (m_bThreadAlive)
	//{
	//	do
	//	{
	//		SetEvent(m_hShutdownEvent);
	//		Sleep(100);
	//	} while (m_bThreadAlive);
	//	TRACE(_T("Thread ended\n"));
	//}
	if (IsPortOpen())
	{
		return TRUE;
	}

	// create events
	if (m_ov.hEvent != NULL)
		CLOSE_HANDLE(m_ov.hEvent);
	m_ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hWriteEvent != NULL)
		CLOSE_HANDLE(m_hWriteEvent);
	m_hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	if (m_hShutdownEvent != NULL)
		CLOSE_HANDLE(m_hShutdownEvent);
	m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// initialize the event objects
	m_hEventArray[0] = m_hShutdownEvent;	// highest priority
	m_hEventArray[1] = m_ov.hEvent;
	m_hEventArray[2] = m_hWriteEvent;

	// initialize critical section
	InitializeCriticalSection(&m_csCommunicationSync);
	
	// set buffersize for writing and save the owner
	m_pOwner = pPortOwner;
		
	m_u32SizeOfWriteBuffer = MAX_WRITE_BUF_SIZE;

	m_nPort = nPort;

	m_dwCommEvents = dwCommEvents;

	BOOL bResult = FALSE;
	TCHAR szPort[256] = {0};
	TCHAR szBaud[256] = {0};


	// now it critical!
	EnterCriticalSection(&m_csCommunicationSync);

	// if the port is already opened: close it
	if (m_hComm != NULL)
	{
		CLOSE_HANDLE(m_hComm);
	}

	// prepare port strings
    _stprintf_s(szPort, _T("\\\\.\\COM%d"),nPort);
	_stprintf_s(szBaud, _T("baud=%d parity=%c data=%d stop=%d"), baud, parity, databits, stopbits);

	// get a handle to the port
	m_hComm = CreateFile(szPort,						// communication port string (COMX)
					     GENERIC_READ | GENERIC_WRITE,	// read/write types
					     0,								// comm devices must be opened with exclusive access
					     NULL,							// no security attributes
					     OPEN_EXISTING,					// comm devices must use OPEN_EXISTING
					     FILE_FLAG_OVERLAPPED,			// Async I/O
					     0);							// template must be 0 for comm devices

	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		// port not found
		m_hComm = NULL;
		return FALSE;
	}

	// set the timeout values
	m_CommTimeouts.ReadIntervalTimeout = MAXDWORD;/*1000*/;
	m_CommTimeouts.ReadTotalTimeoutConstant = 0/*1000*/;
	m_CommTimeouts.ReadTotalTimeoutMultiplier = 0/*1000*/;
	m_CommTimeouts.WriteTotalTimeoutMultiplier = 50/*1000*/;
	m_CommTimeouts.WriteTotalTimeoutConstant = 2000/*1000*/;

	// configure
	if (SetCommTimeouts(m_hComm, &m_CommTimeouts))
	{						   
		if (SetCommMask(m_hComm, dwCommEvents))
		{
			if (GetCommState(m_hComm, &m_dcb))
			{
				m_dcb.fRtsControl = RTS_CONTROL_ENABLE;		// set RTS bit high!
				if (BuildCommDCB(szBaud, &m_dcb))
				{
					if (SetCommState(m_hComm, &m_dcb))
						; // normal operation... continue
					else
						SaveLog(_T("SetCommState()\n"));
				}
				else
					SaveLog(_T("BuildCommDCB()\n"));
			}
			else
				SaveLog(_T("GetCommState()\n"));
		}
		else
			SaveLog(_T("SetCommMask()\n"));
	}
	else
		SaveLog(_T("SetCommTimeouts()\n"));


	m_blWriteFlag =FALSE;
	// flush the port
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	// release critical section
	LeaveCriticalSection(&m_csCommunicationSync);

	if(m_hMutxOfSerial == NULL)
		m_hMutxOfSerial=CreateMutex(NULL,FALSE,_T("Serial_TX_MUTX"));

	TRACE(_T("Initialisation for communication port COM%d completed.\nUse Startmonitor to communicate.\n"), nPort);

	return TRUE;
}

//
//  The CommThread Function.
//
UINT WINAPI CommThread(LPVOID pParam)
{
	// Cast the void pointer passed to the thread back to
	// a pointer of CSerialPort class
	CSerialPort *port = (CSerialPort*)pParam;
	
	if (!port->m_hComm)
	{
		return 0;
	}
	// Set the status variable in the dialog class to
	// TRUE to indicate the thread is running.
	port->m_bThreadAlive = TRUE;	
		
	// Misc. variables
	DWORD BytesTransfered = 0; 
	DWORD Event = 0;
	DWORD CommEvent = 0;
	DWORD dwError = 0;
	COMSTAT comstat;
	BOOL  bResult = TRUE;
		
	// Clear comm buffers at startup
	if (port->m_hComm)		// check if the port is opened
		PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	// begin forever loop.  This loop will run as long as the thread is alive.
	while (port->m_bThreadAlive) 
	{ 

		// Make a call to WaitCommEvent().  This call will return immediatly
		// because our port was created as an async port (FILE_FLAG_OVERLAPPED
		// and an m_OverlappedStructerlapped structure specified).  This call will cause the 
		// m_OverlappedStructerlapped element m_OverlappedStruct.hEvent, which is part of the m_hEventArray to 
		// be placed in a non-signeled state if there are no bytes available to be read,
		// or to a signeled state if there are bytes available.  If this event handle 
		// is set to the non-signeled state, it will be set to signeled when a 
		// character arrives at the port.

		// we do this for each port!

		bResult = WaitCommEvent(port->m_hComm, &Event, &port->m_ov);

		if (!bResult)  
		{ 
			// If WaitCommEvent() returns FALSE, process the last error to determin
			// the reason..
			switch (dwError = GetLastError()) 
			{ 
			case ERROR_IO_PENDING: 	
				{ 
					// This is a normal return value if there are no bytes
					// to read at the port.
					// Do nothing and continue
					break;
				}
			case 87:
				{
					// Under Windows NT, this value is returned for some reason.
					// I have not investigated why, but it is also a valid reply
					// Also do nothing and continue.
					break;
				}
			default:
				{
					// All other error codes indicate a serious error has
					// occured.  Process this error.
					port->SaveLog(_T("WaitCommEvent()\n"));
					break;
				}
			}
		}
		else
		{
			// If WaitCommEvent() returns TRUE, check to be sure there are
			// actually bytes in the buffer to read.  
			//
			// If you are reading more than one byte at a time from the buffer 
			// (which this program does not do) you will have the situation occur 
			// where the first byte to arrive will cause the WaitForMultipleObjects() 
			// function to stop waiting.  The WaitForMultipleObjects() function 
			// resets the event handle in m_OverlappedStruct.hEvent to the non-signelead state
			// as it returns.  
			//
			// If in the time between the reset of this event and the call to 
			// ReadFile() more bytes arrive, the m_OverlappedStruct.hEvent handle will be set again
			// to the signeled state. When the call to ReadFile() occurs, it will 
			// read all of the bytes from the buffer, and the program will
			// loop back around to WaitCommEvent().
			// 
			// At this point you will be in the situation where m_OverlappedStruct.hEvent is set,
			// but there are no bytes available to read.  If you proceed and call
			// ReadFile(), it will return immediatly due to the async port setup, but
			// GetOverlappedResults() will not return until the next character arrives.
			//
			// It is not desirable for the GetOverlappedResults() function to be in 
			// this state.  The thread shutdown event (event 0) and the WriteFile()
			// event (Event2) will not work if the thread is blocked by GetOverlappedResults().
			//
			// The solution to this is to check the buffer with a call to ClearCommError().
			// This call will reset the event handle, and if there are no bytes to read
			// we can loop back through WaitCommEvent() again, then proceed.
			// If there are really bytes to read, do nothing and proceed.
		
			bResult = ClearCommError(port->m_hComm, &dwError, &comstat);

			if (comstat.cbInQue == 0)
				continue;
		}	// end if bResult

		// Main wait function.  This function will normally block the thread
		// until one of nine events occur that require action.
		Event = WaitForMultipleObjects(3, port->m_hEventArray, FALSE, INFINITE);

		switch (Event)
		{
		case WAIT_OBJECT_0 + 0:
			{
				// Shutdown event.  This is event zero so it will be
				// the higest priority and be serviced first.
			 	port->m_bThreadAlive = FALSE;
				break;;
			}
		case WAIT_OBJECT_0 + 1:	// read event
			{
				GetCommMask(port->m_hComm, &CommEvent);
			  /*  if (port->m_pOwner)
			    {
					if (CommEvent & EV_CTS)
						::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_CTS_DETECTED, (WPARAM) 0, (LPARAM) port->m_nPort);
					if (CommEvent & EV_RXFLAG)
						::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_RXFLAG_DETECTED, (WPARAM) 0, (LPARAM) port->m_nPort);
					if (CommEvent & EV_BREAK)
						::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_BREAK_DETECTED, (WPARAM) 0, (LPARAM) port->m_nPort);
					if (CommEvent & EV_ERR)
						::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_ERR_DETECTED, (WPARAM) 0, (LPARAM) port->m_nPort);
					if (CommEvent & EV_RING)
						::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_RING_DETECTED, (WPARAM) 0, (LPARAM) port->m_nPort);
			    }*/
				
				// Receive character event from port
				if (CommEvent & EV_RXCHAR)
				{
					port->m_blReadFlag = TRUE;
					memset(port->m_szReadBuffer,0,sizeof(port->m_szReadBuffer));
					port->m_u32LengthOfReadData = 0;
					port->ReceiveChar(comstat);
				}
				else
				{
					TCHAR szTemp[64]={0};
					if (CommEvent & EV_CTS)
						_stprintf_s(szTemp,_T("CommEvent(EV_CTS):%d\n"),CommEvent);
					else if (CommEvent & EV_RXFLAG)
						_stprintf_s(szTemp,_T("CommEvent(EV_RXFLAG):%d\n"),CommEvent);
					else if (CommEvent & EV_BREAK)
						_stprintf_s(szTemp,_T("CommEvent(EV_BREAK):%d\n"),CommEvent);
					else if (CommEvent & EV_ERR)
						_stprintf_s(szTemp,_T("CommEvent(EV_ERR):%d\n"),CommEvent);
					else if (CommEvent & EV_RING)
				    	_stprintf_s(szTemp,_T("CommEvent(EV_RING):%d\n"),CommEvent);
					else 
						_stprintf_s(szTemp,_T("CommEvent(Unkown):%d\n"),CommEvent);

					port->SaveLog(szTemp);
				}
					
				break;
			}  
		case WAIT_OBJECT_0 + 2: // write event
			{
				// Write character event from port
				port->WriteChar();
				break;
			}

		} // end switch

	} // close forever loop

	return 0;
}

//
// start comm watching
//
BOOL CSerialPort::StartMonitoring()
{
	m_hThread = (HANDLE)_beginthreadex(NULL,0,CommThread,(LPVOID)this,0,NULL);
	if (!m_hThread)
	{
		return FALSE;
	}
	TRACE(_T("Thread started\n"));
	return TRUE;	
}

//
// Restart the comm thread
//
BOOL CSerialPort::RestartMonitoring()
{
	TRACE(_T("Thread resumed\n"));
	ResumeThread(m_hThread);
	return TRUE;	
}


//
// Suspend the comm thread
//
BOOL CSerialPort::StopMonitoring()
{
	TRACE(_T("Thread suspended\n"));
	SuspendThread(m_hThread); 
	return TRUE;	
}

//
// Write a character.
//
void CSerialPort::WriteChar()
{
	BOOL bWrite = TRUE;
	BOOL bResult = TRUE;

	DWORD BytesSent = 0;
	TCHAR szTemp[256]={0};

	ResetEvent(m_hWriteEvent);

	//WaitForSingleObject(port->m_hMutxOfSerial,INFINITE);

	// Gain ownership of the critical section
	EnterCriticalSection(&m_csCommunicationSync);

	if (bWrite)
	{
		// Initailize variables
		m_ov.Offset = 0;
		m_ov.OffsetHigh = 0;

		// Clear buffer
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
        
		bResult = WriteFile(m_hComm,m_szWriteBuffer,m_u32LengthOfBuffer,&BytesSent,&m_ov);

		// deal with any error codes
		if (!bResult)  
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
				case ERROR_IO_PENDING:
					{
						// continue to GetOverlappedResults()
						BytesSent = 0;
						bWrite = FALSE;

						TRACE(_T("Write: IS IO Pend!! BytesSent:%d,m_u32LengthOfBuffer:%d\n"), BytesSent, m_u32LengthOfBuffer);
						break;
					}
				default:
					{
						// all other error codes
						SaveLog(_T("WriteFile()\n"));
					}
			}
		} 
		else
		{
			LeaveCriticalSection(&m_csCommunicationSync);
		}
	} // end if(bWrite)

	if (!bWrite)
	{
		bWrite = TRUE;
	
		bResult = GetOverlappedResult(m_hComm,	// Handle to COMM port 
									  &m_ov,		// Overlapped structure
									  &BytesSent,		// Stores number of bytes sent
									  TRUE); 			// Wait flag

		LeaveCriticalSection(&m_csCommunicationSync);
		TRACE(_T("Write:GetOverlappedResult():bResult:%d,BytesSent:%d,m_u32LengthOfBuffer:%d\n"), bResult,BytesSent, m_u32LengthOfBuffer);
		// deal with the error code 
		if (!bResult)  
		{
			SaveLog(_T("GetOverlappedResults() in WriteFile()\n"));
		}	
	} // end if (!bWrite)
	
	// Verify that the data size send equals what we tried to send
	if (BytesSent < m_u32LengthOfBuffer)
	{
	//	PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
		TRACE(_T("WARNING: WriteFile() error.. Bytes Sent: %d; Message Length: %d\n"), BytesSent, m_u32LengthOfBuffer);
		//未发送完成
		//重发未发送的数据

		memcpy(m_szWriteBuffer, m_szWriteBuffer+BytesSent,m_u32LengthOfBuffer-BytesSent);
		m_u32LengthOfBuffer = m_u32LengthOfBuffer - BytesSent;			
		// set event for write
		SetEvent(m_hWriteEvent);

	}
	else
	{
		m_blWriteFlag=FALSE;
		ReleaseMutex(m_hMutxOfSerial);	
	}
	
}

//
// Character received. Inform the owner
//
void CSerialPort::ReceiveChar(COMSTAT comstat)
{
	BOOL  bRead = TRUE; 
	BOOL  bResult = TRUE;
	DWORD dwError = 0;
	DWORD BytesRead = 0;
	unsigned char RXBuff;

	while(TRUE)
	{ 
		// Gain ownership of the comm port critical section.
		// This process guarantees no other part of this program 
		// is using the port object. 
		
		EnterCriticalSection(&m_csCommunicationSync);

		// ClearCommError() will update the COMSTAT structure and
		// clear any other errors.
		
		bResult = ClearCommError(m_hComm, &dwError, &comstat);

		LeaveCriticalSection(&m_csCommunicationSync);

		// start forever loop.  I use this type of loop because I
		// do not know at runtime how many loops this will have to
		// run. My solution is to start a forever loop and to
		// break out of it when I have processed all of the
		// data available.  Be careful with this approach and
		// be sure your loop will exit.
		// My reasons for this are not as clear in this sample 
		// as it is in my production code, but I have found this 
		// solutiion to be the most efficient way to do this.
		
		if (comstat.cbInQue == 0)
		{
			// break out when all bytes have been read
			m_blReadFlag = FALSE;
			break;
		}
						
		EnterCriticalSection(&m_csCommunicationSync);

		if (bRead)
		{
			bResult = ReadFile(m_hComm,&RXBuff,1,&BytesRead,&m_ov);
			// deal with the error code 
			if (!bResult)  
			{ 
				switch (dwError = GetLastError()) 
				{ 
					case ERROR_IO_PENDING: 	
						{ 
							// asynchronous i/o is still in progress 
							// Proceed on to GetOverlappedResults();
							bRead = FALSE;
							TRACE(_T("Read IO is pending BytesRead:%d\n"),BytesRead);
							break;
						}
					default:
						{
							// Another error has occured.  Process this error.
							SaveLog(_T("ReadFile()\n"));
							break;
						} 
				}
			}
			else
			{
				// ReadFile() returned complete. It is not necessary to call GetOverlappedResults()
				bRead = TRUE;
			}
		}  // close if (bRead)

		if (!bRead)
		{
			bRead = TRUE;
			bResult = GetOverlappedResult(m_hComm,	// Handle to COMM port 
										  &m_ov,		// Overlapped structure
										  &BytesRead,		// Stores number of bytes read
										  TRUE); 			// Wait flag

			TRACE(_T("Read GetOverlappedResult() bResult:%d,BytesRead:%d\n"),bResult,BytesRead);
			// deal with the error code 
			if (!bResult)  
			{
				SaveLog(_T("GetOverlappedResults() in ReadFile()\n"));
			}	
		}  // close if (!bRead)
				
		LeaveCriticalSection(&m_csCommunicationSync);

		// notify parent that a byte was received
		//::SendMessage(m_pOwner->m_hWnd, WM_COMM_RXCHAR, (WPARAM) RXBuff, (LPARAM) port->m_nPortNr);
		OnReceviedChar(RXBuff,m_nPort);
		
	} // end forever loop

}

void CSerialPort::OnReceviedChar(WPARAM wpRam,LPARAM lpRam)
{
	unsigned char ch = (unsigned char)wpRam;
//	if (m_blReadFlag)//串口有数据，则存到m_szReadBuffer
	{
		if (m_u32LengthOfReadData<MAX_READ_BUF_SIZE)//接收的数据如果超过MAX_READ_BUF_SIZE，后面的数据则被忽略
		{
			m_szReadBuffer[m_u32LengthOfReadData] = ch;
			m_u32LengthOfReadData++;
			TRACE(_T("Read:%02x\n"),ch);
		}
	}
	//TCHAR szTemp[256]={0};
	//_stprintf_s(szTemp,_T("%02x\n"),ch);
	//SaveLog(szTemp);
}

UINT CSerialPort::ReadData(unsigned char *pReadBuf,DWORD dwLen,DWORD dwTimeout/*=15000*/)
{
	DWORD dwTemp =0;
	while(dwTemp<dwTimeout)
	{

		//接收完了，并且有数据
		if ((FALSE == m_blReadFlag)&&(m_u32LengthOfReadData > 0))
		{
			if (m_u32LengthOfReadData > dwLen)
			{
				m_u32LengthOfReadData = dwLen;
			}
			memcpy(pReadBuf,m_szReadBuffer,m_u32LengthOfReadData);//把读到的数据返回
			return m_u32LengthOfReadData;
		}
		Sleep(10);
		dwTemp +=10;
	}
	

	return 0;
}

UINT CSerialPort::SendDataAndRecv(BYTE* pu8Data,UINT32 u32Length,unsigned char *pReadBuf,DWORD dwLen,DWORD dwTimeout/*=5000*/)
{
	if (!WriteDatatoPort(pu8Data,u32Length))
	{
		return 0;
	}

	DWORD dwTemp =0;
	while(dwTemp<dwTimeout)
	{
		//接收完了，并且有数据
		if ((FALSE == m_blReadFlag)&&(m_u32LengthOfReadData > 0))
		{
			if (m_u32LengthOfReadData > dwLen)
			{
				m_u32LengthOfReadData = dwLen;
			}
			memcpy(pReadBuf,m_szReadBuffer,m_u32LengthOfReadData);//把读到的数据返回
			return m_u32LengthOfReadData;
		}
		Sleep(10);
		dwTemp +=10;
	}

	return 0;
}
//
// Write a string to the port
//
BOOL CSerialPort::WriteStringToPort(TCHAR* pszString)
{		
	assert(m_hComm != 0);
	char szWrite[MAX_WRITE_BUF_SIZE]={0};

#ifdef UNICODE
		WideCharToChar(pszString, szWrite);
#else
		memcpy(szWrite,pszString,_tcslen(pszString));
#endif
	
	WaitForSingleObject(m_hMutxOfSerial,INFINITE);
	m_blWriteFlag=TRUE;
	memset(m_szWriteBuffer, 0, m_u32SizeOfWriteBuffer);
	memcpy(m_szWriteBuffer,szWrite,strlen(szWrite));

	m_u32LengthOfBuffer = strlen(szWrite);
	// set event for write
	SetEvent(m_hWriteEvent);

	return WaitWriteFinish();
}

BOOL CSerialPort::WriteDatatoPort(BYTE* pu8Data,UINT32 u32Length)
{
	if (!m_hComm)
	{
		return FALSE;
	}
	
	WaitForSingleObject(m_hMutxOfSerial,INFINITE);
	m_blWriteFlag=TRUE;
	memset(m_szWriteBuffer, 0, m_u32SizeOfWriteBuffer);
	if(u32Length>m_u32SizeOfWriteBuffer)
		u32Length = m_u32SizeOfWriteBuffer;
	memcpy(m_szWriteBuffer, pu8Data,u32Length);
	m_u32LengthOfBuffer = u32Length;
	// set event for write
	SetEvent(m_hWriteEvent);

	return WaitWriteFinish();
}

BOOL CSerialPort::WaitWriteFinish(UINT uTimes /*= 10000*/)
{
	UINT nTemp = 0;
	while(m_blWriteFlag)
	{
		Sleep(10);
        nTemp +=10;
		if (nTemp>uTimes)
		{
			return FALSE;//time out
		}
	}
   return TRUE;
};

//
// Return the device control block
//
DCB CSerialPort::GetDCB()
{
	return m_dcb;
}

//
// Return the communication event masks
//
DWORD CSerialPort::GetCommEvents()
{
	return m_dwCommEvents;
}

BOOL CSerialPort::IsPortOpen()
{
	if(m_hComm == NULL)
		return FALSE;
	else
		return TRUE;
}

void CSerialPort::ClosePort()
{
	do
	{
		SetEvent(m_hShutdownEvent);//m_hShutdownEvent);
		Sleep(100);
	} while (m_bThreadAlive);
		TRACE(_T("Thread ended\n"));

	if(m_hComm != NULL)
	{
		CLOSE_HANDLE(m_hComm);
		DeleteCriticalSection(&m_csCommunicationSync);
	}
	if(m_hMutxOfSerial !=NULL)
	{
		CLOSE_HANDLE(m_hMutxOfSerial);
	}
	if (m_hThread)
	{
		CLOSE_HANDLE(m_hThread);
	}
	for (int i = 0;i<3;i++)
	{
		if (m_hEventArray[i] != NULL)
		{
			CLOSE_HANDLE(m_hEventArray[i]);
		}
	}
	m_ov.hEvent = NULL;
	m_hWriteEvent = NULL;
	m_hShutdownEvent = NULL;
}

void CSerialPort::SaveLog(TCHAR *pszText)
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

int CSerialPort::CharToWideChar(char * pchar,WCHAR * pwchar)
{
	int dwnum = MultiByteToWideChar(CP_ACP,0,pchar,-1,NULL,0);
	MultiByteToWideChar(CP_ACP,0,pchar,-1,pwchar,dwnum);
	pwchar[dwnum]='\0';
	return dwnum-1;
}

int CSerialPort::WideCharToChar(WCHAR * pwchar, char * pchar)
{
	int dwnum = WideCharToMultiByte(CP_ACP,0,pwchar,-1,NULL,0,NULL,FALSE);
	WideCharToMultiByte(CP_ACP,0,pwchar,-1,pchar,dwnum,NULL,FALSE);
	pchar[dwnum]='\0';
	return dwnum-1;
}