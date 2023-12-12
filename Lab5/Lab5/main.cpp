#include <windows.h>
#include <dbt.h>
#include <strsafe.h>
#include <WinUser.h>
#include <iostream>
#include <map>
#include <initguid.h>
#include <Usbiodef.h>
#include <iostream>
#include <SetupAPI.h>
#include <vector>
#include <Cfgmgr32.h>
#include <conio.h>
#include <processthreadsapi.h>

#pragma comment(lib, "user32.lib" )
#pragma comment (lib, "Setupapi.lib")

using namespace std;

#define CLS_NAME "DUMMY_CLASS"

struct UsbDeviceDesc
{
	DEVINST devInst;
	string friendlyName;
	wstring name;
	HANDLE handle;
	bool safety;
	bool removable;
};

vector<UsbDeviceDesc> deviceArray;

bool caseUnsensCmp(wstring s1, wstring s2)
{
	if (s1.size() != s2.size())
		return false;
	for (int i = 0; i < s1.size(); i++)
		if (toupper(s1[i]) != toupper(s2[i]))
			return false;
	return true;
}
string handleToFriendlyName(HANDLE handle)
{
	for (auto it : deviceArray)
		if (it.handle == handle)
			return it.friendlyName;
}
bool nameToSafety(wstring name)
{
	for (auto it : deviceArray)
		if (caseUnsensCmp(it.name, name))
			return it.safety;
	return false;
}
void setSafety(HANDLE handle, bool safety)
{
	for (int i = 0; i < deviceArray.size(); i++)
		if (deviceArray[i].handle == handle)
			deviceArray[i].safety = safety;
}
void deleteByName(wstring name)
{
	for (int i = 0; i < deviceArray.size(); i++)
		if (caseUnsensCmp(deviceArray[i].name, name))
		{
			deviceArray.erase(deviceArray.begin() + i);
			return;
		}
}

string getFriendlyName(wchar_t* name)
{
	HDEVINFO deviceList = SetupDiCreateDeviceInfoList(NULL, NULL);
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	SetupDiOpenDeviceInterfaceW(deviceList, name, NULL, &deviceInterfaceData);
	SP_DEVINFO_DATA deviceInfo;
	ZeroMemory(&deviceInfo, sizeof(SP_DEVINFO_DATA));
	deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);
	SetupDiEnumDeviceInfo(deviceList, 0, &deviceInfo);

	DWORD size = 0;
	SetupDiGetDeviceRegistryPropertyA(deviceList, &deviceInfo, SPDRP_DEVICEDESC, NULL, NULL, NULL, &size);
	BYTE* buffer = new BYTE[size];
	SetupDiGetDeviceRegistryPropertyA(deviceList, &deviceInfo, SPDRP_DEVICEDESC, NULL, buffer, size, NULL);
	string deviceDesc = (char*)buffer;
	delete[] buffer;

	return deviceDesc;
}
bool getRemoveability(wchar_t* name)
{
	HDEVINFO deviceList = SetupDiCreateDeviceInfoList(NULL, NULL);
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	SetupDiOpenDeviceInterfaceW(deviceList, name, NULL, &deviceInterfaceData);
	SP_DEVINFO_DATA deviceInfo;
	ZeroMemory(&deviceInfo, sizeof(SP_DEVINFO_DATA));
	deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);
	SetupDiEnumDeviceInfo(deviceList, 0, &deviceInfo);
	DWORD props;
	SetupDiGetDeviceRegistryPropertyA(deviceList, &deviceInfo, SPDRP_CAPABILITIES, NULL, (PBYTE)&props, sizeof(DWORD), NULL);
	return props & CM_DEVCAP_REMOVABLE;
}
string getFriendlyName(PDEV_BROADCAST_DEVICEINTERFACE_A info)
{
	wchar_t* name = (wchar_t*)info->dbcc_name;
	return getFriendlyName(name);
}
string getInstId(const wchar_t* name)
{
	HDEVINFO deviceList = SetupDiCreateDeviceInfoList(NULL, NULL);
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	SetupDiOpenDeviceInterfaceW(deviceList, name, NULL, &deviceInterfaceData);
	SP_DEVINFO_DATA deviceInfo;
	ZeroMemory(&deviceInfo, sizeof(SP_DEVINFO_DATA));
	deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);
	SetupDiEnumDeviceInfo(deviceList, 0, &deviceInfo);

	BYTE buffer[BUFSIZ];
	SetupDiGetDeviceInstanceIdA(deviceList, &deviceInfo, (PSTR)buffer, BUFSIZ, NULL);
	return (char*)buffer;
}

LRESULT FAR PASCAL WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_DEVICECHANGE)
	{
		PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;
		if (wParam == DBT_DEVICEARRIVAL)
		{
			if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
			{
				PDEV_BROADCAST_DEVICEINTERFACE_A info = (PDEV_BROADCAST_DEVICEINTERFACE_A)lpdb;
				cout << "Device \"" << getFriendlyName(info) << "\" has been connected!\n";

				HANDLE deviceHandle = CreateFileW((wchar_t*)info->dbcc_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				DEV_BROADCAST_HANDLE deviceFilter;
				deviceFilter.dbch_size = sizeof(deviceFilter);
				deviceFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
				deviceFilter.dbch_handle = deviceHandle;
				HDEVNOTIFY notifyHandle = RegisterDeviceNotificationW(hWnd, &deviceFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
				CloseHandle(deviceHandle);

				DEVINST devInst;
				CM_Locate_DevNodeA(&devInst, (DEVINSTID_A)getInstId((wchar_t*)info->dbcc_name).c_str(), CM_LOCATE_DEVNODE_NORMAL);

				UsbDeviceDesc tempDesc;
				tempDesc.devInst = devInst;
				tempDesc.handle = deviceHandle;
				tempDesc.name = (wchar_t*)info->dbcc_name;
				tempDesc.safety = false;
				tempDesc.friendlyName = getFriendlyName(info);
				tempDesc.removable = getRemoveability((wchar_t*)info->dbcc_name);
				deviceArray.push_back(tempDesc);
			}
			if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				PDEV_BROADCAST_VOLUME info = (PDEV_BROADCAST_VOLUME)lpdb;
				if (info->dbcv_unitmask)
				{
					cout << "Logical drives on this device: ";
					for (int i = 0; i < 31; i++)
						if (info->dbcv_unitmask & (DWORD)pow(2, i))
							cout << (char)('A' + i) << ' ';
					cout << '\n';
				}
			}
		}
		if (wParam == DBT_DEVICEQUERYREMOVE)
		{
			if (lpdb->dbch_devicetype == DBT_DEVTYP_HANDLE)
			{
				PDEV_BROADCAST_HANDLE info = (PDEV_BROADCAST_HANDLE)lpdb;
				setSafety(info->dbch_handle, true);
			}
		}
		if (wParam == DBT_DEVICEQUERYREMOVEFAILED)
		{
			if (lpdb->dbch_devicetype == DBT_DEVTYP_HANDLE)
			{
				PDEV_BROADCAST_HANDLE info = (PDEV_BROADCAST_HANDLE)lpdb;
				cout << "Failed to safely remove device \"" << handleToFriendlyName(info->dbch_handle) << "\"\n";
				setSafety(info->dbch_handle, false);
			}
		}
		if (wParam == DBT_DEVICEREMOVECOMPLETE)
		{
			if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
			{
				PDEV_BROADCAST_DEVICEINTERFACE_A info = (PDEV_BROADCAST_DEVICEINTERFACE_A)lpdb;
				cout << "Device \"" << getFriendlyName(info) << "\" has been removed"
					<< (nameToSafety((wchar_t*)info->dbcc_name) ? " safely!" : "!") << '\n';
				deleteByName((wchar_t*)info->dbcc_name);
			}
			if (lpdb->dbch_devicetype == DBT_DEVTYP_HANDLE)
			{
				PDEV_BROADCAST_HANDLE info = (PDEV_BROADCAST_HANDLE)lpdb;
				UnregisterDeviceNotification(info->dbch_hdevnotify);
			}
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter)
{

	HWND hWnd = NULL;
	WNDCLASSEX wx;
	ZeroMemory(&wx, sizeof(wx));

	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = (WNDPROC)(WndProc);
	wx.lpszClassName = (LPCWSTR)CLS_NAME;

	GUID guid = GUID_DEVINTERFACE_USB_DEVICE;

	if (RegisterClassEx(&wx))
		hWnd = CreateWindowW((LPCWSTR)CLS_NAME, (LPCWSTR)("DevNotifWnd"), WS_ICONIC, 0, 0, CW_USEDEFAULT, 0, 0, NULL, GetModuleHandle(0), (void*)&guid);

	DEV_BROADCAST_DEVICEINTERFACE_A filter;
	filter.dbcc_size = sizeof(filter);
	filter.dbcc_classguid = guid;
	filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	RegisterDeviceNotificationW(hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
	HDEVINFO devicesHandle = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	SP_DEVINFO_DATA deviceInfo;
	ZeroMemory(&deviceInfo, sizeof(SP_DEVINFO_DATA));
	deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);
	DWORD deviceNumber = 0;
	SP_DEVICE_INTERFACE_DATA devinterfaceData;
	devinterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	while (SetupDiEnumDeviceInterfaces(devicesHandle, NULL, &GUID_DEVINTERFACE_USB_DEVICE, deviceNumber++, &devinterfaceData))
	{
		DWORD bufSize = 0;
		SetupDiGetDeviceInterfaceDetailW(devicesHandle, &devinterfaceData, NULL, NULL, &bufSize, NULL);
		BYTE* buffer = new BYTE[bufSize];
		PSP_DEVICE_INTERFACE_DETAIL_DATA_W devinterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)buffer;
		devinterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
		SetupDiGetDeviceInterfaceDetailW(devicesHandle, &devinterfaceData, devinterfaceDetailData, bufSize, NULL, NULL);

		wchar_t* name = devinterfaceDetailData->DevicePath;

		DEVINST devInst;
		CM_Locate_DevNodeA(&devInst, (DEVINSTID_A)getInstId(devinterfaceDetailData->DevicePath).c_str(), CM_LOCATE_DEVNODE_NORMAL);

		HANDLE deviceHandle = CreateFileW(name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		DEV_BROADCAST_HANDLE deviceFilter;
		deviceFilter.dbch_size = sizeof(deviceFilter);
		deviceFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
		deviceFilter.dbch_handle = deviceHandle;
		HDEVNOTIFY notifyHandle = RegisterDeviceNotificationW(hWnd, &deviceFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
		CloseHandle(deviceHandle);

		UsbDeviceDesc tmpUsbDev;
		tmpUsbDev.devInst = devInst;
		tmpUsbDev.friendlyName = getFriendlyName(name);
		tmpUsbDev.handle = deviceHandle;
		tmpUsbDev.name = name;
		tmpUsbDev.safety = false;
		tmpUsbDev.removable = getRemoveability(name);
		deviceArray.push_back(tmpUsbDev);
	}

	bool flag = true;

	MSG msg;
	while (GetMessageW(&msg, hWnd, 0, 0) || _kbhit())
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

int main()
{
	setlocale(LC_ALL, "Russian");
	system("chcp 1251");
	system("cls");
	CreateThread(NULL, NULL, ThreadProc, NULL, NULL, NULL);
	while (1)
	{
		int id = 1;
		int choise = 0;
		cin >> choise;
		if (!choise)
		{
			system("cls");
			cout << "List of connected USB devices" << endl;
			for (auto it : deviceArray)
				cout << id++ << ". " << it.friendlyName << (it.removable ? " [REMOVABLE]" : " [NOT REMOVABLE]") << endl;
			cout << endl << "Device number to remove it " << endl;
			cout << endl << "0 to update the list" << endl;
			continue;
		}
		if (deviceArray[choise - 1].removable)
			CM_Request_Device_EjectW(deviceArray[choise - 1].devInst, NULL, NULL, NULL, NULL);
		else
			cout << "This device is NOT removable!\n";
	}
	return 0;
}
