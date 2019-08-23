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


/*������ͬpid vid ���ڵĸ���*/
int HidDevice::GetComPortByVidPid(CString strVidPid)
{
	// ��ȡ��ǰϵͳ����ʹ�õ��豸
	int					nPort	= -1;
	int					nStart	= -1;
	int					nEnd	= -1;
	int					i		= 0;
	CString				strTemp, strName;
	DWORD				dwFlag = (DIGCF_ALLCLASSES | DIGCF_PRESENT);
	HDEVINFO			hDevInfo = INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA		sDevInfoData;
	TCHAR				szDis[MAX_PATH] = {0x00};// �洢�豸ʵ��ID
	TCHAR				szFN[MAX_PATH]  = {0x00};// �洢�豸ʵ������
	DWORD				nSize = 0 ;
	DWORD               nCount = 0;

	// ׼�����������豸����USB
	hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, dwFlag);
	if( INVALID_HANDLE_VALUE == hDevInfo )
		goto STEP_END;

	// ��ʼ���������豸
	memset(&sDevInfoData, 0x00, sizeof(SP_DEVICE_INTERFACE_DATA));
	sDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for(i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &sDevInfoData); i++ )
	{
		nSize = 0;

		// ��Ч�豸
		if ( !SetupDiGetDeviceInstanceId(hDevInfo, &sDevInfoData, szDis, sizeof(szDis), &nSize) )			
			goto STEP_END;

		// �����豸��ϢѰ��VID PIDһ�µ��豸
		strTemp.Format(_T("%s"),szDis);
		strTemp.MakeUpper();
		if ( strTemp.Find(strVidPid, 0) == -1 )
			continue;

		AfxMessageBox(strTemp);
		// �����豸����
		nSize = 0;
		SetupDiGetDeviceRegistryProperty(hDevInfo, &sDevInfoData,SPDRP_FRIENDLYNAME,0, (PBYTE)szFN, sizeof(szFN), &nSize);

		// "XXX Virtual Com Port (COM7)"
		strName.Format(_T("%s"),szFN);
		if(strName.IsEmpty())
			goto STEP_END;

		// Ѱ�Ҵ�����Ϣ
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
	// �ر��豸��Ϣ�����
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

	//���ұ�ϵͳ��HID���GUID��ʶ
	HidD_GetHidGuid(&guidHID);

	//���ҷ���HID�淶��USB�豸
	hDevInfo = SetupDiGetClassDevs(&guidHID, NULL, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	while (1)
	{
		//����USB�豸�ӿ�
		SP_DEVICE_INTERFACE_DATA tInterfaceData;
		tInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);//��������䣬�����±�ö�ٻ�ʧ��
		//ö�ٽӿڣ�TRUE��ʾ�ɹ�
		BOOL result = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guidHID, InterfaceIndex, &tInterfaceData);
		if (!result)
		{
			break;
		}

		InterfaceIndex++;
		int bufferSize = 0;
		//���ҵ��˽ӿڣ����ȡ�豸(�ӿ�)·����
		SP_DEVINFO_DATA tDevInfoData;
		tDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		//��һ�ε��ó��������Է�����ȷ��Size
		result = SetupDiGetDeviceInterfaceDetail(hDevInfo, &tInterfaceData, NULL, 0, (PDWORD)&bufferSize, &tDevInfoData);
		//�ڶ��ε��ô��ݷ���ֵ�����ü��ɳɹ�
		/* Allocate memory for Device Detailed Data */
		//�˴������ö�̬��������ܶ���ֲ���������ΪҪ�ڴ˽ṹ���ߴ�źܴ�����ݣ����ֲ�����û����˴�Ŀռ䣡
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
	//�ͷ��豸��Դ(hDevInfo��SetupDiGetClassDevs��ȡ��)
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
		
	//�ͷ�HidD_GetPreparsedDataʹ�õ���Դ
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
