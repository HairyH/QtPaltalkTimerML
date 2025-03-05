/*********************************/
/* (c) 2020 Hairy Soft Solutions */
/*********************************/
#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#define _WIN32_WINNT _WIN32_WINNT_VISTA
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <iostream>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <Richedit.h>
#include <string>

// UIAutomation
#include <UIAutomation.h>
#include <atlbase.h> // For CComPtr
#include <cctype> // For std::iswlower and std::iswupper
#include <iterator> // Add this include for std::begin and std::end
#include <cwctype> // Add this include for std::iswlower and std::iswupper

// Resource Header
#include "resource1.h"

// New version Common Controls, for flat look

#pragma comment(lib,"comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


// TODO: Change these to the App name 
char szAppName[] = "QtPaltalk Timer";
wchar_t wcAppName[] = L"QtPaltalk Timer";

#define msgba(h,x) MessageBoxA(h,x,szAppName,MB_OK)
#define msgbw(h,x) MessageBoxW(h,x,wcAppName,MB_OK)

#define IDT_MICTIMER	5555
#define IDT_MONITORTIMER	6666

