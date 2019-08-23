#include "StdAfx.h"
#include "HidDevice.h"
#include <Dbt.h>
#include <SetupAPI.h>
extern "C"
{
#include <hidsdi.h>
}


#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

HidDevice::HidDevice(void)
{
	m_hHid = INVALID_HANDLE_VALUE;
	m_InputReportLength = 65;
	m_OutputReportLength = 65;
}


HidDevice::~HidDevice(void)
{
}


/*返回相同pid vid 串口的个数*/
int HidDevice::GetComPortByVidPid(CString strVidPid)
{
	// 获取当前系统所有使用的设备
	int					nPort	= -1;
	int					nStart	= -1;
	int					nEnd	= -1;
	int					i		= 0;
	CString				strTemp, strName;
	DWORD				dwFlag = (DIGCF_ALLCLASSES | DIGCF_PRESENT);
	HDEVINFO			hDevInfo = INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA		sDevInfoData;
	TCHAR				szDis[MAX_PATH] = {0x00};// 存储设备实例ID
	TCHAR				szFN[MAX_PATH]  = {0x00};// 存储设备实例属性
	DWORD				nSize = 0 ;
	DWORD               nCount = 0;

	// 准备遍历所有设备查找USB
	hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, dwFlag);
	if( INVALID_HANDLE_VALUE == hDevInfo )
		goto STEP_END;

	// 开始遍历所有设备
	memset(&sDevInfoData, 0x00, sizeof(SP_DEVICE_INTERFACE_DATA));
	sDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for(i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &sDevInfoData); i++ )
	{
		nSize = 0;

		// 无效设备
		if ( !SetupDiGetDeviceInstanceId(hDevInfo, &sDevInfoData, szDis, sizeof(szDis), &nSize) )			
			goto STEP_END;

		// 根据设备信息寻找VID PID一致的设备
		strTemp.Format(_T("%s"),szDis);
		strTemp.MakeUpper();
		if ( strTemp.Find(strVidPid, 0) == -1 )
			continue;

		AfxMessageBox(strTemp);
		// 查找设备属性
		nSize = 0;
		SetupDiGetDeviceRegistryProperty(hDevInfo, &sDevInfoData,SPDRP_FRIENDLYNAME,0, (PBYTE)szFN, sizeof(szFN), &nSize);

		// "XXX Virtual Com Port (COM7)"
		strName.Format(_T("%s"),szFN);
		if(strName.IsEmpty())
			goto STEP_END;

		// 寻找串口信息
		nStart = strName.Find(_T("(COM"), 0);
		nEnd = strName.Find(_T(")"), 0);
		if(nStart == -1 || nEnd == -1)
			goto STEP_END;

		strTemp = strName.Mid(nStart + 4, nEnd - nStart - 2);
		nPort = _wtoi(strTemp);

		if (nPort > 0)
		{
			nCount++;
		}
	}

STEP_END:
	// 关闭设备信息集句柄
	if(hDevInfo != INVALID_HANDLE_VALUE)
	{
		SetupDiDestroyDeviceInfoList(hDevInfo);
		hDevInfo = INVALID_HANDLE_VALUE;
	}

	return nCount;
}

BOOL HidDevice::FindUSBDevice(TCHAR* Vid, TCHAR* Pid, TCHAR *out_devPath, int out_len)
{
	GUID guidHID;
	HDEVINFO hDevInfo;
	int InterfaceIndex = 0;

	//查找本系统中HID类的GUID标识
	HidD_GetHidGuid(&guidHID);

	//查找符合HID规范的USB设备
	hDevInfo = SetupDiGetClassDevs(&guidHID, NULL, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	while (1)
	{
		//查找USB设备接口
		SP_DEVICE_INTERFACE_DATA tInterfaceData;
		tInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);//必须有这句，否则下边枚举会失败
		//枚举接口，TRUE表示成功
		BOOL result = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guidHID, InterfaceIndex, &tInterfaceData);
		if (!result)
		{
			break;
		}

		InterfaceIndex++;
		int bufferSize = 0;
		//若找到了接口，则读取设备(接口)路径名
		SP_DEVINFO_DATA tDevInfoData;
		tDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		//第一次调用出错，但可以返回正确的Size
		result = SetupDiGetDeviceInterfaceDetail(hDevInfo, &tInterfaceData, NULL, 0, (PDWORD)&bufferSize, &tDevInfoData);
		//第二次调用传递返回值，调用即可成功
		/* Allocate memory for Device Detailed Data */
		//此处必须用动态分配而不能定义局部变量，因为要在此结构体后边存放很大的内容，而局部变量没有如此大的空间！
		PSP_DEVICE_INTERFACE_DETAIL_DATA tDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(bufferSize);
		/* Set cbSize in the DevDetail structure */
		tDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		result = SetupDiGetDeviceInterfaceDetail(hDevInfo, &tInterfaceData, tDetailData, bufferSize, (PDWORD)&bufferSize, &tDevInfoData);
		if (result == FALSE)
		{
			free(tDetailData);
			tDetailData = NULL;
			break;
		}

		TCHAR tmpstr[128] = {0};
		_stprintf_s(tmpstr, _T("hid#vid_%s&pid_%s"), Vid, Pid);
		TCHAR *p = _tcsstr(tDetailData->DevicePath, tmpstr);
		if (p)
		{
			int len = _tcslen(tDetailData->DevicePath );
			if (_tcslen(tDetailData->DevicePath )> out_len)
			{
				goto FIND_END;
			}
			memcpy(out_devPath, tDetailData->DevicePath, _tcslen(tDetailData->DevicePath) * sizeof(TCHAR));
			if (hDevInfo != INVALID_HANDLE_VALUE)
				SetupDiDestroyDeviceInfoList(hDevInfo);

			return TRUE;
		}
	}//End of while

FIND_END:
	//释放设备资源(hDevInfo是SetupDiGetClassDevs获取的)
	if (hDevInfo != INVALID_HANDLE_VALUE)
		SetupDiDestroyDeviceInfoList(hDevInfo);
	
	return FALSE;
}

BOOL HidDevice::OpenUSBDevice(TCHAR *DeviceName)
{
	BOOL ret= FALSE;

	//Create File for Device Read/Write
	m_hHid = CreateFile(DeviceName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, (LPSECURITY_ATTRIBUTES)NULL,
					OPEN_EXISTING, 0/*FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED*/,
					NULL);
	if (m_hHid == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	HIDD_ATTRIBUTES attri;
	ret = HidD_GetAttributes(m_hHid, &attri);
	if(!ret)
	{
		return FALSE;
	}
	/*if ((attri.VendorID != 3412) || (attri.ProductID != 1111))
	{
		return FALSE;
	}*/
	//Set Input Buffer Size
	/*int NumInputBuffers = 2;
	ret = HidD_SetNumInputBuffers(m_hHid, NumInputBuffers);
	if(!ret)
	{
		return FALSE;
	}*/

	//Get Preparsed Data
	PHIDP_PREPARSED_DATA PreparsedData;
	ret = HidD_GetPreparsedData(m_hHid, &PreparsedData);
	if(!ret)
	{
		return FALSE;
	}

	//Get Device's Capabilities
	HIDP_CAPS Capabilities;
	HidP_GetCaps(PreparsedData, &Capabilities);
	m_InputReportLength = Capabilities.InputReportByteLength;
	m_OutputReportLength = Capabilities.OutputReportByteLength;
		
	//释放HidD_GetPreparsedData使用的资源
	HidD_FreePreparsedData(PreparsedData);

	return TRUE;
}

void HidDevice::CloseUSBDevice()
{
	if (m_hHid != INVALID_HANDLE_VALUE)
		CloseHandle(m_hHid);
	m_hHid = INVALID_HANDLE_VALUE;
}

int HidDevice::UsbDeviceWrite(BYTE* lpBuffer, DWORD dwSize)
{
	DWORD dwRet = 0;
	BOOL bRet;
	DWORD len = min(m_OutputReportLength, dwSize);

	bRet = WriteFile(m_hHid, lpBuffer, len/*dwSize*/, &dwRet, NULL);
	if (!bRet)
	{
		return -1;
	}

	return dwRet;
}

int HidDevice::UsbDeviceRead(BYTE* lpBuffer, DWORD dwSize)
{
	BOOL bRet = FALSE;
	DWORD dwRet = 0;
	DWORD len = min(m_InputReportLength, dwSize);

	bRet = ReadFile(m_hHid, lpBuffer, len/*dwSize*/, &dwRet, NULL);
	if (!bRet)
	{
		return -1;
	}

	return dwRet;
}
