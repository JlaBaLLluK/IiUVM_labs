#include <iostream>
#include <Windows.h>
#pragma comment(lib, "PowrProf.lib")
#include <powrprof.h>
#include <poclass.h>
#include <setupapi.h>
#pragma comment(lib, "SetupAPI.lib")
#include <devguid.h>
#include <string>

using namespace std;

SYSTEM_POWER_STATUS GetPowerStatus()
{
	SYSTEM_POWER_STATUS powerStatus;
	if (!GetSystemPowerStatus(&powerStatus))
	{
		cout << "Unable to get power status!" << endl;
		exit(EXIT_FAILURE);
	}

	return powerStatus;
}

void PrintSystemChargingStatus(SYSTEM_POWER_STATUS powerStatus)
{
	if (powerStatus.ACLineStatus == 1)
	{
		cout << "Battery is charging." << endl;
	}
	else if (powerStatus.ACLineStatus == 0)
	{
		cout << "Battery isn't charging. " << endl;
	}
	else
	{
		cout << "Unknown status!" << endl;
	}
}

void PrintBatterySaver(SYSTEM_POWER_STATUS powerStatus)
{
	if (powerStatus.SystemStatusFlag)
	{
		cout << "Battery saver is on." << endl;
	}
	else
	{
		cout << "Battery saver is off." << endl;
	}
}

void PrintBatteryLiftime(SYSTEM_POWER_STATUS powerStatus)
{
	if (powerStatus.BatteryLifeTime != -1)
	{
		auto remainingLifetime = powerStatus.BatteryLifeTime;
		int hours = remainingLifetime / 3600;
		remainingLifetime -= hours * 3600;
		int minutes = remainingLifetime / 60;
		cout << "Remaining battery life time: " << hours << " hours " << minutes << " minutes." << endl;
	}
}

void HibernateOrSleep()
{
	string option;
	cout << "H/h - hibernate; S/s - sleep";
	getline(cin, option);
	if (option == "h" || option == "H")
	{
		SetSuspendState(true, false, false);
	}
	else if (option == "s" || option == "S")
	{
		SetSuspendState(false, false, false);
	}
}

void PrintBatteryPercentage(SYSTEM_POWER_STATUS powerStatus)
{
	cout << "Current battery percentage: " << (int)powerStatus.BatteryLifePercent << "%." << endl;
}

void PrintBatteryType()
{
	HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (INVALID_HANDLE_VALUE == hdev)
	{
		cout << "hdev error" << endl;
		exit(EXIT_FAILURE);
	}

	SP_DEVICE_INTERFACE_DATA did = { 0 };
	did.cbSize = sizeof(did);

	if (!SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVCLASS_BATTERY, 0, &did))
	{
		cout << "SetupDiEnumDeviceInterfaces error" << endl;
		exit(EXIT_FAILURE);
	}

	DWORD cbRequired = 0;
	SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0);
	if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
	{
		PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
		if (!pdidd)
		{
			cout << "pdidd error" << endl;
			exit(EXIT_FAILURE);
		}

		pdidd->cbSize = sizeof(*pdidd);
		if (!SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, 0))
		{
			cout << "SetupDiGetDeviceInterfaceDetail error" << endl;
			exit(EXIT_FAILURE);
		}

		HANDLE hBattery = CreateFile(pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hBattery)
		{
			cout << "hBattery error" << endl;
			exit(EXIT_FAILURE);
		}

		BATTERY_QUERY_INFORMATION bqi = { 0 };
		DWORD dwWait = 0;
		DWORD dwOut;
		if (!DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(dwWait), &bqi.BatteryTag,
			sizeof(bqi.BatteryTag), &dwOut, NULL) && bqi.BatteryTag)
		{
			cout << "DeviceIoControl error" << endl;
			exit(EXIT_FAILURE);
		}

		BATTERY_INFORMATION bi = { 0 };
		bqi.InformationLevel = BatteryInformation;
		if (!DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), &bi, sizeof(bi), &dwOut, NULL))
		{
			cout << "DeviceIoControl error" << endl;
			exit(EXIT_FAILURE);
		}

		cout << "Battery type: " << bi.Chemistry << "." << endl;
		CloseHandle(hBattery);
		LocalFree(pdidd);
		SetupDiDestroyDeviceInfoList(hdev);
	}
}

int main()
{
	while (true)
	{
		SYSTEM_POWER_STATUS powerStatus = GetPowerStatus();
		PrintSystemChargingStatus(powerStatus);
		PrintBatteryType();
		PrintBatterySaver(powerStatus);
		PrintBatteryLiftime(powerStatus);
		PrintBatteryPercentage(powerStatus);
		HibernateOrSleep();
		cout << "-------------------------------" << endl;
	}

	return 0;
}