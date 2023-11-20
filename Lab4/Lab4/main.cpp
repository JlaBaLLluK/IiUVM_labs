#include <Windows.h>
#include <setupapi.h>
#include <locale.h>
#include <string.h> 
#include <iostream>
#include <wdmguid.h>
#include <devguid.h>
#include <iomanip>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#pragma comment(lib, "setupapi.lib")

using namespace std;
using namespace cv;

void CamInfo()
{
	HDEVINFO devInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_CAMERA, "USB", NULL, DIGCF_PRESENT);
	if (devInfo == INVALID_HANDLE_VALUE)
		return;

	SP_DEVINFO_DATA devInfoData;
	WCHAR buffer[512];

	char instanceIDBuffer[1024];

	for (int i = 0; ; i++)
	{
		devInfoData.cbSize = sizeof(devInfoData);
		if (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData) == FALSE)
			break;

		memset(buffer, 0, sizeof(buffer));
		SetupDiGetDeviceRegistryProperty(devInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (BYTE*)buffer, 512, NULL);

		wstring name, ids;
		for (int i = 0; i < 512; i++)
			name += buffer[i];

		memset(buffer, 0, sizeof(buffer));
		SetupDiGetDeviceRegistryProperty(devInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (BYTE*)buffer, 512, NULL);
		for (int i = 0; i < 512; i++)
			ids += buffer[i];
		wstring ven(ids.substr(ids.find(L"VID_") + 4, 4));
		wstring dev(ids.substr(ids.find(L"PID_") + 4, 4));

		memset(buffer, 0, sizeof(buffer));
		SetupDiGetDeviceInstanceIdA(devInfo, &devInfoData, (PSTR)instanceIDBuffer, 512, NULL);
		string instanceID(instanceIDBuffer);

		if (name.substr(name.size() - 4, 4) == dev)
			name = name.substr(0, name.size() - 7);

		std::cout << "Information about camera:" << endl;
		wcout << L"Name: " << name << endl;
		wcout << L"Vendor ID: " << ven << endl;
		wcout << L"Device ID: " << dev << endl;
		cout << "Instance ID: " << instanceID << endl;

		SetupDiDeleteDeviceInfo(devInfo, &devInfoData);
	}

	SetupDiDestroyDeviceInfoList(devInfo);
}

int main()
{
	CamInfo();
	Mat matrix;
	VideoCapture capture(0 + CAP_DSHOW);
	cout << "1. PHOTO" << endl << "2. HIDDEN PHOTO CAPTURE" << endl << "0. EXIT" << endl;
	while (true)
	{
		int input;
		cin >> input;
		if (input == 1)
		{
			capture >> matrix;
			capture.read(matrix);
			imwrite("photo.jpg", matrix);
		}
		else if (input == 2)
		{
			ShowWindow(GetConsoleWindow(), SW_HIDE);
			for (int i = 0; i < 5; i++)
			{
				string filename = "hidden_photo" + to_string(i) + ".jpg";
				capture >> matrix;
				capture.read(matrix);
				imwrite(filename, matrix);
				Sleep(2000);
			}

			return 0;
		}
		else if (input == 0)
		{
			break;
		}
		else
		{
			cout << "INCORRECT DATA" << endl;
			break;
		}
	}

	return 0;
}
