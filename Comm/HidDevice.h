#pragma once
class HidDevice
{
public:
	HidDevice(void);
	~HidDevice(void);

	HANDLE m_hHid;
	int m_InputReportLength;
	int m_OutputReportLength;


	int GetComPortByVidPid(CString strVidPid);
	BOOL FindUSBDevice(TCHAR* Vid, TCHAR* Pid, TCHAR *out_devPath, int out_len);
	BOOL OpenUSBDevice(TCHAR *DeviceName);
	void CloseUSBDevice();
	int UsbDeviceWrite(BYTE* lpBuffer, DWORD dwSize);
	int UsbDeviceRead(BYTE* lpBuffer, DWORD dwSize);
};

