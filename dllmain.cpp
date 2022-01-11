// dllmain.cpp : Defines the entry point for the DLL application.
#pragma warning(disable: 26812)
#pragma warning(disable: 33005)

#include "framework.h"
#include "stdafx.h"

#pragma comment(lib, "comsuppwd.lib") //for <comdef.h>
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "libMinHook.x86.lib")



//Functions definitions
DWORD WINAPI InitializationThread(LPVOID lpParam);


//IWbemClassObject::Get (Wbemcli.h)
typedef HRESULT(WINAPI *IWBEMCLASSOBJECT_F_GET)(IWbemClassObject *, LPCWSTR, LONG, VARIANT *, CIMTYPE *, long *);
static IWBEMCLASSOBJECT_F_GET fpIWBEMCLASSOBJECT_F_GET = NULL;
static IWBEMCLASSOBJECT_F_GET fpIWBEMCLASSOBJECT_F_GET_UNHOOKED = NULL;

//IWbemClassObject::Next (Wbemcli.h)
typedef HRESULT(WINAPI *IWBEMCLASSOBJECT_F_NEXT)(IWbemClassObject *, long lFlags, BSTR *strName, VARIANT *pVal, CIMTYPE *pType, long *plFlavor);
static IWBEMCLASSOBJECT_F_NEXT fpIWBEMCLASSOBJECT_F_NEXT = NULL;
static IWBEMCLASSOBJECT_F_NEXT fpIWBEMCLASSOBJECT_F_NEXT_UNHOOKED = NULL;

//UrlDownloadToCacheFileA (Urlmon.h)
typedef HRESULT(WINAPI *URLDOWNLOADTOCACHEFILEA)(LPUNKNOWN, LPCSTR, LPSTR, DWORD, DWORD, LPBINDSTATUSCALLBACK);
static URLDOWNLOADTOCACHEFILEA fpURLDOWNLOADTOCACHEFILEA = NULL;
static URLDOWNLOADTOCACHEFILEA fpURLDOWNLOADTOCACHEFILEA_UNHOOKED = NULL;


//Variables
HINSTANCE hSelfInstance = NULL;
std::string contracteActivePath;
bool FlagRecord = false;


INT FMT_MSG_ERROR(LPCTSTR szFormat, ...)
{
	va_list vagrc;
	va_start(vagrc, szFormat);
	TCHAR szParamText[2048] = { NULL };
	StringCbVPrintf(szParamText, sizeof(szParamText), szFormat, vagrc);
	va_end(vagrc);

	MessageBoxW(HWND_DESKTOP, szParamText, L"Error", MB_OK);
	ExitProcess(1);
	return 1;
}
INT FMT_OUTDBG_STRING(LPCTSTR szFormat, ...)
{
	va_list vagrc;
	va_start(vagrc, szFormat);
	TCHAR szParamText[2048] = { NULL };
	StringCbVPrintf(szParamText, sizeof(szParamText), szFormat, vagrc);
	va_end(vagrc);
	OutputDebugString(szParamText);
	return 1;
}

inline std::string WideStrToNarrowStrTrunc(std::wstring const &widestr)
{
	std::string nstr(widestr.length(), 0);
	std::transform(widestr.begin(), widestr.end(), nstr.begin(), [ ](wchar_t c)
				   {
					   return (char)c;
				   });
	return nstr;
}

BOOL APIENTRY DllMain(HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			OutputDebugStringW(L"Saga library injected..initialising environment.");
			DisableThreadLibraryCalls(hModule);

			hSelfInstance = hModule;
			DWORD dwThreadId = 0;
			HANDLE hInitializeThread = CreateThread(
				NULL,                   // default security attributes
				0,                      // use default stack size  
				InitializationThread,   // thread function name
				NULL,					// argument to thread function 
				0,                      // use default creation flags 
				&dwThreadId);			// returns the thread identifier 

			if (hInitializeThread)
			{
				CloseHandle(hInitializeThread);
				return TRUE;
			}
			else
			{
				FMT_OUTDBG_STRING(L"Failed creating initialization thread");
				ExitProcess(1);
				return TRUE;
			}
		}
		break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		{
			if (fpIWBEMCLASSOBJECT_F_GET && fpIWBEMCLASSOBJECT_F_GET_UNHOOKED)
			{
				MH_DisableHook(fpIWBEMCLASSOBJECT_F_GET);
				MH_RemoveHook(fpIWBEMCLASSOBJECT_F_GET);
			}

			if (fpIWBEMCLASSOBJECT_F_NEXT && fpIWBEMCLASSOBJECT_F_NEXT_UNHOOKED)
			{
				MH_DisableHook(fpIWBEMCLASSOBJECT_F_NEXT);
				MH_RemoveHook(fpIWBEMCLASSOBJECT_F_NEXT);
			}

			if (fpURLDOWNLOADTOCACHEFILEA && fpURLDOWNLOADTOCACHEFILEA_UNHOOKED)
			{
				MH_DisableHook(fpURLDOWNLOADTOCACHEFILEA);
				MH_RemoveHook(fpURLDOWNLOADTOCACHEFILEA);
			}

			MH_Uninitialize();
		}
		break;
	}
	return TRUE;
}


HRESULT WINAPI DETOUR_URLDOWNLOADTOCACHEFILEA(LPUNKNOWN lpUnkcaller,
											  LPCSTR szURL,
											  LPSTR szFileName,
											  DWORD cchFileName,
											  DWORD dwReserved,
											  LPBINDSTATUSCALLBACK pBSC)
{
	OutputDebugStringW(L"DETOUR_URLDOWNLOADTOCACHEFILEA --------------------=");
	FMT_OUTDBG_STRING(L"  szURL=%S", szURL);
	FMT_OUTDBG_STRING(L"  pBSC=%s", NOT_NULL(pBSC) ? L"NOT NULL" : L"(%null%)");

	HRESULT hResult = fpURLDOWNLOADTOCACHEFILEA_UNHOOKED(lpUnkcaller, szURL, szFileName, cchFileName, dwReserved, pBSC);

	FMT_OUTDBG_STRING(L"  path: %S", szFileName);


	/*
	* REPORT MESSAGE
	* FMT_OUTDBG_STRING(L"contracteActivePath=%s, contracteActivePath.size()=%d, cchFileName=%d", contracteActivePath.data(),
	* 				  contracteActivePath.size(), cchFileName);
	*/
	if (!FlagRecord && contracteActivePath.size() > 0)
	{
		const std::string_view haystack { szURL };
		const std::string_view needle { "get_contracte_active.php?" };

		auto it = std::search(haystack.begin(),
							  haystack.end(),
							  std::boyer_moore_searcher(needle.begin(), needle.end()));

		//Found
		if (it != haystack.end())
		{
			//If I want to spoof data then I will copy spoofed file to cache folder and replace the cached file.
			if (!CopyFileA(contracteActivePath.c_str(), szFileName, false))
			{
				//Fail
				return FMT_MSG_ERROR(L"Couldn't copy fake file '%S' to '%S'.", contracteActivePath.c_str(), szFileName);
			}
			else
			{
				FMT_OUTDBG_STRING(L"  Spoofed file into cache folder: %S", szFileName);
			}
		}
	}

	FMT_OUTDBG_STRING(L"\n");

	return S_OK;
}


HRESULT WINAPI DETOUR_IWBEMCLASSOBJECT_F_NEXT(IWbemClassObject *pThis,
											  long lFlags,
											  BSTR *strName,
											  VARIANT *pVal,
											  CIMTYPE *pType,
											  long *plFlavor)
{
	HRESULT hResult = fpIWBEMCLASSOBJECT_F_NEXT_UNHOOKED(pThis, lFlags, strName, pVal, pType, plFlavor);

	FMT_OUTDBG_STRING(L"DETOUR_IWBEMCLASSOBJECT_F_NEXT --------------------=");

	if (hResult >= WBEM_S_NO_ERROR)
	{
		FMT_OUTDBG_STRING(L"  strName=%s", *strName);
		FMT_OUTDBG_STRING(L"  pVal->vt=0x%x", pVal->vt);
		FMT_OUTDBG_STRING(L"  is_VT_BSTR=%s", (pVal->vt == VT_BSTR ? L"TRUE" : L"FALSE"));

		if (pVal && pVal->vt == VT_BSTR)
		{
			FMT_OUTDBG_STRING(L"pVal->bstrVal=%s", pVal->bstrVal);
		}

		FMT_OUTDBG_STRING(L"\n");

		//if (wcscmp(*strName, L"NumberOfCores") == 0)
		//{
		//	VariantClear(pVal);
		//	pVal->vt = VT_UINT;
		//	pVal->uintVal = 5;
		//}
	}

	return hResult;
}

LPCWSTR VCHAR_T_TO_STRING(VARTYPE vt)
{
	switch (vt)
	{
		case VARENUM::VT_EMPTY:            return L"VT_EMPTY";
		case VARENUM::VT_NULL:             return L"VT_NULL";
		case VARENUM::VT_I2:               return L"VT_I2";
		case VARENUM::VT_I4:               return L"VT_I4";
		case VARENUM::VT_R4:               return L"VT_R4";
		case VARENUM::VT_R8:               return L"VT_R8";
		case VARENUM::VT_CY:               return L"VT_CY";
		case VARENUM::VT_DATE:             return L"VT_DATE";
		case VARENUM::VT_BSTR:             return L"VT_BSTR";
		case VARENUM::VT_DISPATCH:         return L"VT_DISPATCH";
		case VARENUM::VT_ERROR:            return L"VT_ERROR";
		case VARENUM::VT_BOOL:             return L"VT_BOOL";
		case VARENUM::VT_VARIANT:          return L"VT_VARIANT";
		case VARENUM::VT_UNKNOWN:          return L"VT_UNKNOWN";
		case VARENUM::VT_DECIMAL:          return L"VT_DECIMAL";
		case VARENUM::VT_I1:               return L"VT_I1";
		case VARENUM::VT_UI1:              return L"VT_UI1";
		case VARENUM::VT_UI2:              return L"VT_UI2";
		case VARENUM::VT_UI4:              return L"VT_UI4";
		case VARENUM::VT_I8:               return L"VT_I8";
		case VARENUM::VT_UI8:              return L"VT_UI8";
		case VARENUM::VT_INT:              return L"VT_INT";
		case VARENUM::VT_UINT:             return L"VT_UINT";
		case VARENUM::VT_VOID:             return L"VT_VOID";
		case VARENUM::VT_HRESULT:          return L"VT_HRESULT";
		case VARENUM::VT_PTR:              return L"VT_PTR";
		case VARENUM::VT_SAFEARRAY:        return L"VT_SAFEARRAY";
		case VARENUM::VT_CARRAY:           return L"VT_CARRAY";
		case VARENUM::VT_USERDEFINED:      return L"VT_USERDEFINED";
		case VARENUM::VT_LPSTR:            return L"VT_LPSTR";
		case VARENUM::VT_LPWSTR:           return L"VT_LPWSTR";
		case VARENUM::VT_RECORD:           return L"VT_RECORD";
		case VARENUM::VT_INT_PTR:          return L"VT_INT_PTR";
		case VARENUM::VT_UINT_PTR:         return L"VT_UINT_PTR";
		case VARENUM::VT_FILETIME:         return L"VT_FILETIME";
		case VARENUM::VT_BLOB:             return L"VT_BLOB";
		case VARENUM::VT_STREAM:           return L"VT_STREAM";
		case VARENUM::VT_STORAGE:          return L"VT_STORAGE";
		case VARENUM::VT_STREAMED_OBJECT:  return L"VT_STREAMED_OBJECT";
		case VARENUM::VT_STORED_OBJECT:    return L"VT_STORED_OBJECT";
		case VARENUM::VT_BLOB_OBJECT:      return L"VT_BLOB_OBJECT";
		case VARENUM::VT_CF:               return L"VT_CF";
		case VARENUM::VT_CLSID:            return L"VT_CLSID";
		case VARENUM::VT_VERSIONED_STREAM: return L"VT_VERSIONED_STREAM";
		case VARENUM::VT_BSTR_BLOB:        return L"VT_BSTR_BLOB";
		case VARENUM::VT_VECTOR:           return L"VT_VECTOR";
		case VARENUM::VT_ARRAY:            return L"VT_ARRAY";
		case VARENUM::VT_BYREF:            return L"VT_BYREF";
		case VARENUM::VT_RESERVED:         return L"VT_RESERVED";
		case VARENUM::VT_ILLEGAL:          return L"VT_ILLEGAL";
	}
	return L"UNKOWN";
}

HRESULT WINAPI DETOUR_IWBEMCLASSOBJECT_F_GET(IWbemClassObject *pThis,
											 LPCWSTR wszDesiredPropertyName,
											 LONG lFlags,
											 VARIANT *pVal,
											 CIMTYPE *pType,
											 long *plFlavor)
{
	HRESULT hResult = fpIWBEMCLASSOBJECT_F_GET_UNHOOKED(pThis, wszDesiredPropertyName, lFlags, pVal, pType, plFlavor);

	FMT_OUTDBG_STRING(L"DETOUR_IWBEMCLASSOBJECT_F_GET --------------------=");

	if (SUCCEEDED(hResult))
	{
		VARIANT pClass;
		fpIWBEMCLASSOBJECT_F_GET_UNHOOKED(pThis, L"__CLASS", 0, &pClass, NULL, NULL);

		if (NOT_NULL(pClass.bstrVal))
		{
			FMT_OUTDBG_STRING(L"  pClass.bstrVal=%s\n", pClass.bstrVal);
		}
		else
		{
			FMT_OUTDBG_STRING(L"  CLASS IS NULL");
		}


		if (NOT_NULL(wszDesiredPropertyName))
		{
			FMT_OUTDBG_STRING(L"  wszDesiredPropertyName=%s\n", wszDesiredPropertyName);
		}
		else
		{
			FMT_OUTDBG_STRING(L"  WSZDESIREDPROPERTYNAME IS NULL");
		}

		if (NOT_NULL(pVal) && NOT_NULL(pVal->bstrVal))
		{
			FMT_OUTDBG_STRING(L"  pVal->vt=%s\n", VCHAR_T_TO_STRING(pVal->vt));
			FMT_OUTDBG_STRING(L"  pVal->bstrVal=%s\n", pVal->bstrVal);
		}
		else
		{
			FMT_OUTDBG_STRING(L"  pVal->vt=%s\n", L"(%null%)");
			FMT_OUTDBG_STRING(L"  pVal->bstrVal=%s\n", L"(%null%)");
		}


		if (FlagRecord)
		{
			BSTR pObjectText = NULL;
			if (SUCCEEDED(pThis->GetObjectText(0, &pObjectText)))
			{
				FMT_OUTDBG_STRING(L"  pThis->GetObjectText=%s", pObjectText);
				SysFreeString(pObjectText);
			}
			else
			{
				FMT_OUTDBG_STRING(L"  Failed pThis->GetObjectText");
			}
		}


		if (!FlagRecord)
		{
			//To replace value
			//SysFreeString(pVal->bstrVal);
			//pVal->bstrVal = SysAllocString(L"AG74N032810204708");
			 

			if (NOT_NULL(pVal) && NOT_NULL(pClass.bstrVal) &&
				STR_FOUND(wcsstr(pClass.bstrVal, L"Win32_DiskDrive")) &&
				STR_FOUND(wcsstr(wszDesiredPropertyName, L"deviceid")))
			{
				FMT_OUTDBG_STRING(L"Beg::Win32_DiskDrive");
				VariantClear(pVal);
				pVal->vt = VT_BSTR;
				pVal->bstrVal = SysAllocString(L"\\\\.\\PHYSICALDRIVE0"); // =  \\.\PHYSICALDRIVE0
				FMT_OUTDBG_STRING(L"End::Win32_DiskDrive");
			}


			if (NOT_NULL(pVal) && NOT_NULL(pClass.bstrVal) &&
				STR_FOUND(wcsstr(pClass.bstrVal, L"Win32_DiskPartition")) &&
				STR_FOUND(wcsstr(wszDesiredPropertyName, L"deviceid")))
			{
				FMT_OUTDBG_STRING(L"Beg::Win32_DiskPartition");
				VariantClear(pVal);
				pVal->vt = VT_BSTR;
				pVal->bstrVal = SysAllocString(L"Disk #0, Partition #0");
				//pVal->bstrVal = SysAllocString(L"Disk #0, Partition #1");
				//pVal->bstrVal = SysAllocString(L"Disk #0, Partition #2");
				FMT_OUTDBG_STRING(L"End::Win32_DiskPartition");
			}


			/*
			*
			*	THE REST OF THE CODE HERE - where did it go? :-)
			*
			*/
		}
	}

	_com_error hErrorMessageCom(hResult);

	FMT_OUTDBG_STRING(L"  Return code=%x,(%s)", hResult, hErrorMessageCom.ErrorMessage());
	FMT_OUTDBG_STRING(L"\n");

	return hResult;
}


DWORD WINAPI InitializationThread(LPVOID lpParam)
{
	// Initialize MinHook.
	if (MH_Initialize() != MH_OK)
	{
		return FMT_MSG_ERROR(L"Failed initializing hooking library.");
	}
	else
	{
		FMT_OUTDBG_STRING(L"Initialized hooking library.");
	}

	HANDLE gInjectFinishedEvent = NULL;
	HMODULE hFastprox = NULL;
	HMODULE hUrlmon = NULL;

	//Get event via name lookup
	gInjectFinishedEvent = CreateEventW(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is non-signaled
		EVENT_SAGA_WMI_HOOK // object name
	);

	if (gInjectFinishedEvent == NULL)
	{
		return FMT_MSG_ERROR(L"Failed opening the global event handle (%s).", EVENT_SAGA_WMI_HOOK);
	}
	else
	{
		FMT_OUTDBG_STRING(L"Opened the global event handle (%s).", EVENT_SAGA_WMI_HOOK);
	}

	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		return FMT_MSG_ERROR(L"ERROR: Created new event object instead of getting a handle to an existing one.");
	}

	//
	if ((hFastprox = GetModuleHandleA("fastprox.dll")) == NULL)
	{
		return FMT_MSG_ERROR(L"Failed getting a reference to 'fastprox.dll' library.");
	}
	else
	{
		FMT_OUTDBG_STRING(L"Found library 'fastprox.dll' inside saga program.");
	}

	//Initialize IWbemClassObject::Get
	fpIWBEMCLASSOBJECT_F_GET = (IWBEMCLASSOBJECT_F_GET)GetProcAddress(
		hFastprox,
#ifdef _WIN64
		"?Get@CWbemObject@@UEAAJPEBGJPEAUtagVARIANT@@PEAJ2@Z"
#else
		"?Get@CWbemObject@@UAGJPBGJPAUtagVARIANT@@PAJ2@Z"
#endif
	);

	if (fpIWBEMCLASSOBJECT_F_GET == NULL)
	{
		return FMT_MSG_ERROR(L"Failed getting a reference to GET interface");
	}
	else
	{
		FMT_OUTDBG_STRING(L"Got a reference to WMI GET interface");
	}
	MH_CreateHook(fpIWBEMCLASSOBJECT_F_GET, &DETOUR_IWBEMCLASSOBJECT_F_GET, (LPVOID *)&fpIWBEMCLASSOBJECT_F_GET_UNHOOKED);
	MH_EnableHook(fpIWBEMCLASSOBJECT_F_GET);



	//Initialize IWbemClassObject::Next
	fpIWBEMCLASSOBJECT_F_NEXT = (IWBEMCLASSOBJECT_F_NEXT)GetProcAddress(
		hFastprox,
#ifdef _WIN64
		!!"?Next@CWbemObject@@UAGJJPAPAGPAUtagVARIANT@@PAJ2@Z"!! //LOOK IT UP
#else
		"?Next@CWbemObject@@UAGJJPAPAGPAUtagVARIANT@@PAJ2@Z"
#endif
	);

	if (fpIWBEMCLASSOBJECT_F_NEXT == NULL)
	{
		return FMT_MSG_ERROR(L"Failed getting a reference to NEXT interface");
	}
	else
	{
		FMT_OUTDBG_STRING(L"Got a reference to WMI NEXT interface");
	}
	MH_CreateHook(fpIWBEMCLASSOBJECT_F_NEXT, &DETOUR_IWBEMCLASSOBJECT_F_NEXT, (LPVOID *)&fpIWBEMCLASSOBJECT_F_NEXT_UNHOOKED);
	MH_EnableHook(fpIWBEMCLASSOBJECT_F_NEXT);


	if ((hUrlmon = GetModuleHandleA("Urlmon.dll")) == NULL)
	{
		return FMT_MSG_ERROR(L"Failed getting a reference to 'Urlmon.dll' library.");
	}
	else
	{
		FMT_OUTDBG_STRING(L"Found library 'Urlmon.dll' inside saga program.");
	}

	fpURLDOWNLOADTOCACHEFILEA = (URLDOWNLOADTOCACHEFILEA)GetProcAddress(hUrlmon, "URLDownloadToCacheFileA");
	if (fpIWBEMCLASSOBJECT_F_GET == NULL)
	{
		return FMT_MSG_ERROR(L"Failed getting a reference to URLDownloadToCacheFileA.");
	}
	else
	{
		FMT_OUTDBG_STRING(L"Got a reference to URLDownloadToCacheFileA.");
	}

	MH_CreateHook(fpURLDOWNLOADTOCACHEFILEA, &DETOUR_URLDOWNLOADTOCACHEFILEA, (LPVOID *)&fpURLDOWNLOADTOCACHEFILEA_UNHOOKED);
	MH_EnableHook(fpURLDOWNLOADTOCACHEFILEA);


	//Getting some data
	winreg::RegKey key;
	if (!key.TryOpen(HKEY_CURRENT_USER, REG_SUBKEY_SAGAWMIHOOK))
	{
		return FMT_MSG_ERROR(L"Failed opening key %s in regedit hive.", REG_SUBKEY_SAGAWMIHOOK);
	}
	else
	{
		FMT_OUTDBG_STRING(L"Opened key %s in regedit hive.", REG_SUBKEY_SAGAWMIHOOK);
	}

	if (auto valContracteActivePath = key.TryGetStringValue(REG_SUBKEY_VAL_GET_CONTRACTE_ACTIVE_PATH))
	{
		contracteActivePath = WideStrToNarrowStrTrunc(valContracteActivePath.value());
		FMT_OUTDBG_STRING(L"Regedit value:%s has '%s'", REG_SUBKEY_VAL_GET_CONTRACTE_ACTIVE_PATH, valContracteActivePath.value().c_str());
	}

	if (auto valFlagRecord = key.TryGetDwordValue(REG_SUBKEY_VAL_FLAG_RECORD))
	{
		FlagRecord = static_cast<bool>(valFlagRecord.value());
		FMT_OUTDBG_STRING(L"Regedit value:%s has '%d'", REG_SUBKEY_VAL_FLAG_RECORD, valFlagRecord.value());
	}

	//Signal the starter that we finished hooking the functions and reading configuration files
	//The starter can now resume the process
	SetEvent(gInjectFinishedEvent);

	return 0;
}
