// SagaWMIStarter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "stdafx.h"
#pragma comment(lib, "Shlwapi.lib")

bool InjectLibraryIntoSuspendedProcess(HANDLE hProcess, PWCHAR wszLibraryPath, SIZE_T nPathLength)
{
	LPVOID lpLoadLibraryW;
	LPVOID lpRemoteAddress;
	HMODULE hModKernel32;

	ZeroMemory(&hModKernel32, sizeof(hModKernel32));
	hModKernel32 = GetModuleHandle(L"Kernel32.dll");

	if (!hModKernel32)
	{
		std::wcout << L"Failed getting Kernel32.dll module handle" << std::endl;
		return FALSE;
	}

	lpLoadLibraryW = GetProcAddress(hModKernel32, "LoadLibraryW");

	if (!lpLoadLibraryW)
	{
		std::wcout << L"GetProcAddress for LoadLibraryW failed" << std::endl;

		return FALSE;
	}

	// Create enough space to hold library path name
	lpRemoteAddress = VirtualAllocEx(hProcess, NULL, nPathLength + 1, MEM_COMMIT, PAGE_READWRITE);

	if (!lpRemoteAddress)
	{
		std::wcout << L"VirtualAllocEx in remote process failed" << std::endl;;

		return FALSE;
	}

	// Write library path name at alloted space
	if (!WriteProcessMemory(hProcess, lpRemoteAddress, wszLibraryPath, nPathLength, NULL))
	{
		std::wcout << L"WriteProcessMemory failed writing into remote process" << std::endl;;

		// free allocated memory
		VirtualFreeEx(hProcess, lpRemoteAddress, 0, MEM_RELEASE);

		return FALSE;
	}


	HANDLE hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)lpLoadLibraryW, lpRemoteAddress, NULL, NULL);
	BOOL fResult = true;

	if (!hThread)
	{
		std::wcout << L"CreateRemoteThread failed" << std::endl;;
		fResult = false;
	}
	else
	{
		WaitForSingleObject(hThread, 4000);
		CloseHandle(hThread);
	}

	// Free allocated memory
	VirtualFreeEx(hProcess, lpRemoteAddress, 0, MEM_RELEASE);

	return fResult;
}

int main(int argc, const char *argv[])
{
	//Getting some data
	winreg::RegKey key;
	if (!key.TryOpen(HKEY_CURRENT_USER, REG_SUBKEY_SAGAWMIHOOK))
	{
		//Try create key then
		winreg::RegResult resultCreate = key.TryCreate(HKEY_CURRENT_USER, REG_SUBKEY_SAGAWMIHOOK);
		if (!resultCreate)
		{
			std::wcout << L"Failed creating key " << REG_SUBKEY_SAGAWMIHOOK << L" in regedit hive." << std::endl;
			return 0;
		}
		else
		{
			std::wcout << L"Created key " << REG_SUBKEY_SAGAWMIHOOK << L" in regedit hive." << std::endl;
		}
	}

	argparse::ArgumentParser program("Saga WMI starter");
	program.add_description("Use this program to start SAGA in hook mode. :-) ");
	program.add_argument("-r", "--record")
		.help("Hook and intercept methods but do not change behaviour. Record all data and dump to disk.")
		.default_value(0)
		.scan<'i', int>();

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error &err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	if (program.is_used("-h"))
	{
		std::cout << program << std::endl;
		std::exit(0);
	}


	auto FlagRecord = program.get<int>("-r");
	key.SetDwordValue(REG_SUBKEY_VAL_FLAG_RECORD, static_cast<DWORD>(FlagRecord));
	if (FlagRecord)
	{
		std::wcout << L"--> Saga program will only record and intercept function calls." << std::endl;
	}


	wchar_t LIBRARY_PATH_FASTPROX[] = L"C:\\Windows\\System32\\wbem\\fastprox.dll";
	wchar_t LIBRARY_PATH_URLMON[] = L"C:\\Windows\\System32\\urlmon.dll";



	std::wstring APPLICATION_PATH_SAGA;
	std::wstring LIBRARY_PATH_HOOK;

	if (auto value = key.TryGetStringValue(REG_SUBKEY_VAL_SAGA_PATH))
	{
		APPLICATION_PATH_SAGA = std::move(value.value());
	}
	else
	{
		std::wcout << L"Failed reading value " << REG_SUBKEY_VAL_SAGA_PATH << L" in regedit hive." << std::endl;
		return 0;
	}

	if (auto value = key.TryGetStringValue(REG_SUBKEY_VAL_SAGA_LIB_PATH))
	{
		LIBRARY_PATH_HOOK = std::move(value.value());
	}
	else
	{
		std::wcout << L"Failed reading value " << REG_SUBKEY_VAL_SAGA_LIB_PATH << L" in regedit hive." << std::endl;
		return 0;
	}

	//Create global event before creating the process and injecting
	HANDLE gInjectFinishedEvent = CreateEventW(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is non-signaled
		EVENT_SAGA_WMI_HOOK // object name
	);
	if (gInjectFinishedEvent == NULL)
	{
		std::wcout << L"Failed creating named global event 'EventSagaWmiHook' (" << GetLastError()  << L")." << std::endl;
		return 0;
	}

	//Create process in suspended mode and start injecting libraries
	PROCESS_INFORMATION processInformation;
	STARTUPINFO startupInfo;
	ZeroMemory(&processInformation, sizeof(processInformation));
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	if (!CreateProcess(
		APPLICATION_PATH_SAGA.c_str(),
		NULL,
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED,
		NULL,
		NULL,
		&startupInfo,
		&processInformation))
	{
		std::wcout << L"Failed spawning remote process." << std::endl;
		return 1;
	}

	std::wcout << L"[OK] Process created succesfully - ready for injection." << std::endl;

	//Inject important libraries now
	if (!InjectLibraryIntoSuspendedProcess(processInformation.hProcess, LIBRARY_PATH_FASTPROX, wcslen(LIBRARY_PATH_FASTPROX) * sizeof(WCHAR)))
	{
		std::wcout << L"Failed injecting library " << LIBRARY_PATH_FASTPROX << L" into process." << std::endl;
		goto CLEANUP_PROCESS_HANDLES_AND_EXIT;
	}
	else
	{
		std::wcout << L"[OK] Forced injection of " << PathFindFileName(LIBRARY_PATH_FASTPROX) << L"." << std::endl;
	}

	//Inject important libraries now
	if (!InjectLibraryIntoSuspendedProcess(processInformation.hProcess, LIBRARY_PATH_URLMON, wcslen(LIBRARY_PATH_URLMON) * sizeof(WCHAR)))
	{
		std::wcout << L"Failed injecting library " << LIBRARY_PATH_URLMON << L" into process." << std::endl;
		goto CLEANUP_PROCESS_HANDLES_AND_EXIT;
	}
	else
	{
		std::wcout << L"[OK] Forced injection of " << PathFindFileName(LIBRARY_PATH_URLMON) << L"." << std::endl;
	}

	//Inject hooking library
	if (!InjectLibraryIntoSuspendedProcess(processInformation.hProcess, const_cast<PWCHAR>(LIBRARY_PATH_HOOK.c_str()), LIBRARY_PATH_HOOK.size() * sizeof(WCHAR)))
	{
		std::wcout << L"Failed injecting WMI hooking library." << std::endl;
		goto CLEANUP_PROCESS_HANDLES_AND_EXIT;
	}
	else
	{
		std::wcout << L"[OK] Forced injection of " << PathFindFileName(LIBRARY_PATH_HOOK.c_str()) << L"." << std::endl;
	}

	std::wcout << L"-------------------------" << std::endl;
	std::wcout << L"Waiting for the injection library to finish before resuming programm." << std::endl;

	if (WaitForSingleObject(gInjectFinishedEvent, 70000) == WAIT_OBJECT_0) //signaled
	{
		std::wcout << L"::Event object was signaled." << std::endl;
		std::wcout << L"Resuming process from suspended mode." << std::endl;
	}
	else
	{
		std::wcout << L"::ERROR: the library didn't finish seting up the hooks in due time or it failed.";
		TerminateProcess(processInformation.hProcess, 1);
		//In case TerminateProcess didn't work let it call ResumeThread and crash on it's own.
	}

	ResumeThread(processInformation.hThread);

CLEANUP_PROCESS_HANDLES_AND_EXIT:
	CloseHandle(processInformation.hProcess);
	CloseHandle(processInformation.hThread);

	return 0;
}