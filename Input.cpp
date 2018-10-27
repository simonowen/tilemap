#include "stdafx.h"
#include "Input.h"

#define EVENT_BUFFER_SIZE	16

#define DIRECTINPUT_VERSION		0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

static const std::vector<UINT>sSpectrumKeyboard =
{
	VK_SHIFT, 'Z', 'X', 'C', 'V',
	'A', 'S', 'D', 'F', 'G',
	'Q', 'W', 'E', 'R', 'T',
	'1', '2', '3', '4', '5',
	'0', '9', '8', '7', '6',
	'P', 'O', 'I', 'U', 'Y',
	VK_RETURN, 'L', 'K', 'J', 'H',
	' ', VK_CONTROL, 'M', 'N', 'B'
};

ComPtr<IDirectInputDevice8> g_pKeyboard;

void Input_Init(HWND hwnd, HINSTANCE hinst, KeyMatrix &matrix)
{
	ComPtr<IDirectInput8> g_pDirectInput;
	auto hr = DirectInput8Create(hinst, DIRECTINPUT_VERSION, IID_IDirectInput8, &g_pDirectInput, nullptr);
	hr = g_pDirectInput->CreateDevice(GUID_SysKeyboard, &g_pKeyboard, nullptr);
	hr = g_pKeyboard->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	hr = g_pKeyboard->SetDataFormat(&c_dfDIKeyboard);

	DIPROPDWORD dipdw = { { sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE, }, EVENT_BUFFER_SIZE, };
	hr = g_pKeyboard->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

	Input_ReleaseKeys(matrix);
}

void Input_Exit()
{
	g_pKeyboard = nullptr;
}

void Input_ReleaseKeys(KeyMatrix &matrix)
{
	matrix.fill(0xff);
}

void Input_ReadKeyboard(KeyMatrix &matrix)
{
	if (!g_pKeyboard)
		return;

	auto hr = g_pKeyboard->Acquire();
	if (FAILED(hr))
		return;

	DIDEVICEOBJECTDATA abEvents[EVENT_BUFFER_SIZE];
	DWORD dwItems = _countof(abEvents);

	hr = g_pKeyboard->GetDeviceData(sizeof(abEvents[0]), abEvents, &dwItems, 0);
	if (FAILED(hr) || !dwItems)
		return;

	for (DWORD i = 0; i < dwItems; i++)
	{
		auto virtualKey = MapVirtualKey(abEvents[i].dwOfs, MAPVK_VSC_TO_VK);
		bool pressed = (abEvents[i].dwData & 0x80) != 0;

		auto it = std::find(sSpectrumKeyboard.begin(), sSpectrumKeyboard.end(), virtualKey);
		if (it != sSpectrumKeyboard.end())
		{
			auto index = std::distance(sSpectrumKeyboard.begin(), it);
			auto row = index / 5;
			auto mask = 1 << (index % 5);

			if (pressed)
				matrix[row] &= ~mask;
			else
				matrix[row] |= mask;
		}
	}
}
