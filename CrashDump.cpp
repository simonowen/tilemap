// Windows crash dump support, to save a minidump image for debugging

#include <windows.h>
#include <DbgHelp.h>
#include <string>

#pragma comment(lib, "DbgHelp.lib")

BOOL CreateMiniDump (EXCEPTION_POINTERS* pep)
{
	BOOL ret = false;

	WCHAR szModule[MAX_PATH];
	GetModuleFileName(nullptr, szModule, ARRAYSIZE(szModule));

	std::wstring path = szModule;
	auto dumppath = path.substr(0, path.find_last_of(L'.') + 1) + L"dmp";

	HANDLE hfile = CreateFile(dumppath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	// If the location is read-only, create in the temporary directory instead
	if (hfile == INVALID_HANDLE_VALUE)
	{
		GetTempPath(MAX_PATH, szModule);
		auto module = path.substr(path.find_last_of(L'\\') + 1);
		auto dumpfile = module.substr(0, module.find_last_of(L'.') + 1) + L"dmp";
		dumppath = std::wstring(szModule) + dumpfile;
		hfile = CreateFile(dumppath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	}

	if (hfile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pep;
		mdei.ClientPointers = TRUE;

		ret = MiniDumpWriteDump(GetCurrentProcess(),
								GetCurrentProcessId(),
								hfile,
								MiniDumpWithDataSegs,  //MiniDumpWithFullMemory,
								pep ? &mdei : nullptr,
								nullptr,
								nullptr);
		// Close the file
		CloseHandle(hfile);

		if (!ret)
			DeleteFile(path.c_str());

		auto message = L"Diagnostics file saved to:\n\n" + dumppath + L"\n\nPlease report the crash with this file.";
		MessageBox(NULL, message.c_str(), L"Crashed", MB_ICONSTOP);
	}

	return ret;
}

LONG WINAPI CrashDumpUnhandledExceptionFilter (EXCEPTION_POINTERS * ExceptionInfo)
{
	SetUnhandledExceptionFilter(nullptr);
	CreateMiniDump(ExceptionInfo);
	return EXCEPTION_EXECUTE_HANDLER;
}
