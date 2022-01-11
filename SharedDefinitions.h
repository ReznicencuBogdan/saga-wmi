#pragma once

//HELPERS
#define NOT_NULL(x) ((x) != NULL)
#define STR_FOUND(x) NOT_NULL(x)
#define DEREF_BSTR(x) *(x)

//EVENT NAMES
#define EVENT_SAGA_WMI_HOOK L"EventSagaWmiHook"

//KEYES
#define REG_SUBKEY_SAGAWMIHOOK L"SOFTWARE\\SAGA_WMI_HOOK"

//VALUES
#define REG_SUBKEY_VAL_SAGA_PATH L"SagaPath"
#define REG_SUBKEY_VAL_SAGA_LIB_PATH L"SagaLibPath"
#define REG_SUBKEY_VAL_GET_CONTRACTE_ACTIVE_PATH L"GetContracteActivePath"

#define REG_SUBKEY_VAL_FLAG_RECORD L"FlagRecord"