#pragma once
#define _WIN32_DCOM
#define WIN32_LEAN_AND_MEAN
#include <comdef.h>
#include <Wbemidl.h>
#include <Windows.h>
#include <urlmon.h>
#include <strsafe.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <functional>

#include "SharedDefinitions.h"
#include "MinHook.h"
#include "winreg.hpp"