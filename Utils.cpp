#include "stdafx.h"
#include "Utils.h"

extern HWND g_hwnd;

HRESULT Fail(HRESULT hr, LPCWSTR pszOperation)
{
	if (FAILED(hr))
	{
		WCHAR sz[256];
		swprintf(sz, _countof(sz), L"%s failed with %08lx", pszOperation, hr);
		ShowWindow(g_hwnd, SW_HIDE);
		MessageBox(g_hwnd, sz, L"TileMap", MB_ICONSTOP);
#ifdef _DEBUG
		__debugbreak();
#endif
	}

	return hr;
}

std::vector<uint8_t> FileContents(const std::wstring &filename)
{
	HANDLE hfile = CreateFile(filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		auto str = "File not found: " + to_string(filename) +
			"\n\nYou must provide your own copy of this file as it can't be distributed with TileMap.";
		throw std::runtime_error(str);
	}

	DWORD dwSize = GetFileSize(hfile, NULL), dwRead;
	std::vector<BYTE> file(dwSize);
	ReadFile(hfile, file.data(), dwSize, &dwRead, NULL);
	CloseHandle(hfile);

	return file;
}

uint32_t random_uint32()
{
	static std::mt19937 rnd(static_cast<unsigned>(std::time(nullptr)));
	return rnd();
}

void Test()
{
	auto start_time = std::chrono::system_clock::now();
	auto end_time = std::chrono::system_clock::now();
	auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
}
