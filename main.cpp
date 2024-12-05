
/*********************************/
/* (c) HairyH 2020               */
/*********************************/
#include "main.h"

#undef DEBUG
//#define DEBUG

// Global Variables
HINSTANCE	hInst = 0;
HWND		ghMain = 0;
HWND        ghPtMain = 0;
HWND		ghEdit = 0;
HWND		ghInterval = 0;
HWND		ghLimit = 0;
HWND		ghClock = 0;
HWND		ghCbSend = 0;

// Paltalk Handles
HWND ghPtRoom = NULL, ghPtLv = NULL;

// Font handles
HFONT ghFntClk = NULL;

// Timer related globals
SYSTEMTIME gsysTime = { 0 };
BOOL gbMonitor = FALSE;
BOOL gbSendTxt = TRUE;
BOOL gbSendLimit = FALSE;

int giMicTimerSeconds = 0;
int giInterval = 30;
int giLimit = 120;
// Nick related globals
char gszSavedNick[MAX_PATH] = { '0' };
char gszCurrentNick[MAX_PATH] = { '0' };
int iMaxNicks = 0;
int iDrp = 0;

//Quick Messagebox macro
#define msga(x) msgba(ghMain,x)
// Quick Messagebox WIDE String
#define msgw(x) msgbw(ghMain,x)

// Function prototypes
BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void GetPaltalkWindows(void);
BOOL CALLBACK EnumPaltalkWindows(HWND hWnd, LPARAM lParam);
BOOL InitClockDis(void);
BOOL InitIntervals(void);
BOOL InitMicLimits(void);
BOOL GetMicUser(void);
void StartStopMonitoring(void);
void MicTimerStart(void);
void MicTimerReset(void);
void MicTimerTick(void);
void MonitorTimerTick(void);

void CopyPaste2Paltalk(char* szMsg);

/// Main entry point of the app 
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	hInst = hInstance;
	InitCommonControls();
	LoadLibrary(L"riched20.dll"); // comment if richedit is not used
	// TODO: Add any initiations as needed
	return DialogBox(hInst, MAKEINTRESOURCE(IDD_DLGMAIN), NULL, (DLGPROC)DlgMain);
}

/// Callback main message loop for the Application
BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ghMain = hwndDlg;
		ghInterval = GetDlgItem(hwndDlg, IDC_COMBO_INTERVAL);
		ghClock = GetDlgItem(hwndDlg, IDC_EDIT_CLOCK);
		ghCbSend = GetDlgItem(hwndDlg, IDC_CHECK1);
		ghLimit = GetDlgItem(hwndDlg, IDC_COMBO_MCLIMIT);
		InitClockDis();
		InitIntervals();
		InitMicLimits();
		SendDlgItemMessageW(hwndDlg, IDC_CHECK1, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
	}
	break; // return TRUE;
	case WM_CLOSE:
	{
		DeleteObject(ghFntClk);
		EndDialog(hwndDlg, 0);
	}
	return TRUE;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
			case IDCANCEL:
				{
					EndDialog(hwndDlg, 0);
				}
				return TRUE;
			case IDOK:
				{
					GetPaltalkWindows();
				}
				return TRUE;
			case IDC_BUTTON_START:
				{
					StartStopMonitoring();
					//GetMicUser();
				}
				return TRUE;
			case IDC_CHECK1: // Send text to Paltalk
				{
					gbSendTxt = IsDlgButtonChecked(ghMain, IDC_CHECK1);
				}
				return TRUE;
			case IDC_CHECK2: // Send Mic time limit to Paltalk
				{
					gbSendLimit = IsDlgButtonChecked(ghMain, IDC_CHECK2);
				}
				return TRUE;
			case IDC_COMBO_INTERVAL:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						int iSel = SendMessage(ghInterval, CB_GETCURSEL, 0, 0);
						giInterval = SendMessage(ghInterval, CB_GETITEMDATA, (WPARAM)iSel, 0);
						char szTemp[100] = { 0 };
						sprintf_s(szTemp, "giInterval = %d \n", giInterval);
						OutputDebugStringA(szTemp);
					}
				}
				return TRUE;
			case IDC_COMBO_MCLIMIT:
			{
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					int iSel = SendMessage(ghLimit, CB_GETCURSEL, 0, 0);
					giLimit = SendMessage(ghLimit, CB_GETITEMDATA, (WPARAM)iSel, 0);
					char szTemp[100] = { 0 };
					sprintf_s(szTemp, "giLimit = %d \n", giLimit);
					OutputDebugStringA(szTemp);
				}
			}
			return TRUE;
		}
	}
	case WM_TIMER:
	{
		if (wParam == IDT_MICTIMER)
		{
			MicTimerTick();
		}
		else if (wParam == IDT_MONITORTIMER)
		{
			MonitorTimerTick();
		}
	}

	default:
		return FALSE;
	}
	return FALSE;
}

/// Initialise Paltalk Control Handles
void GetPaltalkWindows(void)
{
	char szWinText[200] = { '0' };
	char szTitle[256] = { '0' };

	// Paltalk chat room window
	ghPtRoom = NULL; ghPtLv = NULL;

	// Getting the chat room window handle
	ghPtRoom = FindWindowW(L"DlgGroupChat Window Class", 0); // this is to find nick list
	ghPtMain = FindWindowW(L"Qt5150QWindowIcon", 0);  // this is to send text 

	if (ghPtRoom) // we got it
	{
		// Get the current thread ID and the target window's thread ID
		DWORD targetThreadId = GetWindowThreadProcessId(ghPtMain, 0);
		DWORD currentThreadId = GetCurrentThreadId();
		// Attach the input of the current thread to the target window's thread
		AttachThreadInput(currentThreadId, targetThreadId, true);
		// Bring the window to the foreground
		bool bRet = SetForegroundWindow(ghPtMain);
		// Give some time for the window to come into focus
		Sleep(500);
		// Detach input once done
		AttachThreadInput(currentThreadId, targetThreadId, false);
		// Make the Paltalk Room window the HWND_TOPMOST 
		SetWindowPos(ghPtMain, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		GetWindowTextA(ghPtRoom, szWinText, 200);
		// Set the title text to indicate which room we are timing
		sprintf_s(szTitle, 255, "Timing: %s", szWinText);
		SendDlgItemMessageA(ghMain, IDC_STATIC_TITLE, WM_SETTEXT, (WPARAM)0, (LPARAM)szTitle);
		// Finding the chat room window controls handles
		EnumChildWindows(ghPtRoom, EnumPaltalkWindows, 0);

	}
	else
	{
		msga("No Paltalk Window Found!");
	}

}

/// Enumeration Callback to Find the Control Windows
BOOL CALLBACK EnumPaltalkWindows(HWND hWnd, LPARAM lParam)
{
	char szListViewClass[] = "SysHeader32";
	char szMsg[MAX_PATH] = { 0 };
	char szClassNameBuffer[MAX_PATH] = { 0 };

	GetClassNameA(hWnd, szClassNameBuffer, MAX_PATH);

#ifdef DEBUG
	LONG lgIdCnt;
	lgIdCnt = GetWindowLongW(hWnd, GWL_ID);
	wsprintfA(szMsg, "windows class name: %s Control ID: %d \n", szClassNameBuffer, lgIdCnt);
	OutputDebugStringA(szMsg);
#endif // DEBUG

	if (strcmp(szListViewClass, szClassNameBuffer) == 0)
	{
		ghPtLv = hWnd;
#ifdef DEBUG
		sprintf_s(szMsg, "List View window handle: %p \n", ghPtLv);
		OutputDebugStringA(szMsg);
#endif // DEBUG
		return FALSE;
	}

	return TRUE;
}

/// Initialise Intervals Combo
BOOL InitIntervals(void)
{
	wchar_t wsTemp[256] = { '\0' };
	int iIndex = 0;
	int iSec = 0;
	int iMin = 0;

	for (int i = 30; i <= 180; i = i + 30)
	{
		iMin = i / 60;
		iSec = i % 60;
		swprintf_s(wsTemp, L"%d:%02d", iMin, iSec);
		iIndex = SendMessageW(ghInterval, CB_ADDSTRING, (WPARAM)0, (LPARAM)wsTemp);
		SendMessageW(ghInterval, CB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)i);
	}

	SendMessageW(ghInterval, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	giInterval = SendMessageW(ghInterval, CB_GETITEMDATA, (WPARAM)0, (LPARAM)0);
	return TRUE;
}

/// Initialise Mic time Limit selector Combo
BOOL InitMicLimits(void)
{
	wchar_t wsTemp[256] = { '\0' };
	int iIndex = 0;
	int iSec = 0;
	int iMin = 0;

	for (int i = 60; i <= 600; i = i + 60)
	{
		iMin = i / 60;
		iSec = i % 60;
		swprintf_s(wsTemp, L"%d:%02d", iMin, iSec);
		iIndex = SendMessageW(ghLimit, CB_ADDSTRING, (WPARAM)0, (LPARAM)wsTemp);
		giLimit = SendMessageW(ghLimit, CB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)i);
	}

	SendMessageW(ghLimit, CB_SETCURSEL, (WPARAM)1, (LPARAM)1);
	giLimit = SendMessageW(ghLimit, CB_GETITEMDATA, (WPARAM)1, (LPARAM)0);
	return TRUE;
	
}

/// Initialise the Clock Display Window
BOOL InitClockDis(void)
{
	HDC hDC;
	int nHeight = 0;

	hDC = GetDC(ghMain);
	nHeight = -MulDiv(20, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ghFntClk = CreateFont(nHeight, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Courier New"));
	SendMessageA(ghClock, WM_SETFONT, (WPARAM)ghFntClk, (LPARAM)TRUE);
	SendMessageA(ghClock, WM_SETTEXT, (WPARAM)0, (LPARAM)"00:00");

	return TRUE;
}

/// Starts the Mic timer
void MicTimerStart(void)
{
	SendMessageA(ghClock, WM_SETTEXT, 0, (LPARAM)"00:00");
	giMicTimerSeconds = 0;
	SetTimer(ghMain, IDT_MICTIMER, 1000, 0);
}

/// Reset the Timer
void MicTimerReset(void)
{
	KillTimer(ghMain, IDT_MICTIMER);
	giMicTimerSeconds = 0;
	SendMessageA(ghClock, WM_SETTEXT, 0, (LPARAM)"00:00");
}

/// Thi is called every 1sec. 
void MicTimerTick(void)
{
	char szClock[MAX_PATH] = { '\0' };
	char szMsg[MAX_PATH] = { '\0' };
	int iX = 60;
	int iMin;
	int iSec;

	giMicTimerSeconds++;

	iMin = giMicTimerSeconds / iX;
	iSec = giMicTimerSeconds % iX;
	// Refreshing the Clock display
	sprintf_s(szClock, MAX_PATH, "%02d:%02d", iMin, iSec);
	SendMessageA(ghClock, WM_SETTEXT, 0, (LPARAM)szClock);

	// if mic time limit sending is enabled 
	if (gbSendLimit && giMicTimerSeconds == giLimit)
	{
		sprintf_s(szMsg, MAX_PATH, "%s on Mic for: %d:%02d min. TIME LIMIT!", gszCurrentNick, iMin, iSec);
		CopyPaste2Paltalk(szMsg);
	}
	// Work out when to send text to Paltalk
	else if (giMicTimerSeconds % giInterval == 0)
	{
		sprintf_s(szMsg, MAX_PATH, "%s on Mic for: %d:%02d min.", gszCurrentNick, iMin, iSec);
		CopyPaste2Paltalk(szMsg);
	}
}


void MonitorTimerTick(void)
{
	GetMicUser();
	// Failed to get current nick
	if (strlen(gszCurrentNick) < 2 && strlen(gszSavedNick) < 2) return;
	// no change keep going
	if (strcmp(gszCurrentNick, gszSavedNick) == 0)
	{
		iDrp = 0; // Reset dropout counter
	}
	// No nick but there was one before
	else if (strlen(gszCurrentNick) < 2 && strlen(gszSavedNick) > 2)
	{
		iDrp++; //to tolerate mic dropout

		char szTemp[MAX_PATH] = { '\0' };
		sprintf_s(szTemp, "Mic dropout count %d \n", iDrp);
		OutputDebugStringA(szTemp);

		if (iDrp > 4) // 5 second dropout
		{
			MicTimerReset(); //Stop the mic timing
			sprintf_s(gszSavedNick, "a"); // Reset the saved nick
			sprintf_s(szTemp, "5 dropouts: %d Reset Mic timer \n", iDrp);
			OutputDebugStringA(szTemp);
			iDrp = 0;
		}
	}
	// New nick on mic -> Start Mic Timer
	else if (strcmp(gszCurrentNick, gszSavedNick) != 0)
	{
		SYSTEMTIME sytUtc;
		char szMsg[MAX_PATH] = { '\O' };

		MicTimerStart();

		GetSystemTime(&sytUtc);

		sprintf_s(szMsg, "Start: %s at: %02d:%02d:%02d UTC", gszCurrentNick, sytUtc.wHour, sytUtc.wMinute, sytUtc.wSecond);
		CopyPaste2Paltalk(szMsg);

		sprintf_s(szMsg, "Start: %s at: %02d:%02d:%02d UTC\n", gszCurrentNick, sytUtc.wHour, sytUtc.wMinute, sytUtc.wSecond);
		OutputDebugStringA(szMsg);

		strcpy_s(gszSavedNick, gszCurrentNick);
	}
}

/// Get the Mic user
BOOL GetMicUser(void)
{
	if (!ghPtLv) return FALSE;
		
	char szOut[MAX_PATH] = { '\0' };
	char szMsg[MAX_PATH] = { '\0' };
	wchar_t wsMsg[MAX_PATH] = { '\0' };
	BOOL bRet = FALSE;

	const int iSizeOfItemNameBuff = MAX_PATH * sizeof(char); //wchar_t
	LPSTR pXItemNameBuff = NULL;

	LVITEMA lviNick = { 0 };
	DWORD dwProcId;
	VOID* vpMemLvi;
	HANDLE hProc;
	int iNicks;
	int iImg = 0;

	LVITEMA lviRead = { 0 };

	GetWindowThreadProcessId(ghPtLv, &dwProcId);
	hProc = OpenProcess(
		PROCESS_VM_OPERATION |
		PROCESS_VM_READ |
		PROCESS_VM_WRITE, FALSE, dwProcId);

	if (hProc == NULL)return FALSE;

	vpMemLvi = VirtualAllocEx(hProc, NULL, sizeof(LVITEMA),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (vpMemLvi == NULL) return FALSE;

	pXItemNameBuff = (LPSTR)VirtualAllocEx(hProc, NULL, iSizeOfItemNameBuff,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (pXItemNameBuff == NULL) return FALSE;

	iNicks = ListView_GetItemCount(ghPtLv);
	if (iMaxNicks < iNicks)
	{
		iMaxNicks = iNicks;
		char szTemp[100] = { 0 };
		sprintf_s(szTemp, "%d", iMaxNicks);
		SendDlgItemMessageA(ghMain, IDC_EDIT2, WM_SETTEXT, (WPARAM)0, (LPARAM)szTemp);
	}
#ifdef DEBUG
	sprintf_s(szMsg, "Number of nicks: %d \n", iNicks);
	OutputDebugStringA(szMsg);
#endif // DEBUG
	for (int i = 0; i < iNicks; i++)
	{
		lviNick.mask = LVIF_IMAGE | LVIF_TEXT;
		lviNick.pszText = pXItemNameBuff;
		lviNick.cchTextMax = MAX_PATH - 1;
		lviNick.iItem = i;
		lviNick.iSubItem = 0;

		WriteProcessMemory(hProc, vpMemLvi, &lviNick, sizeof(lviNick), NULL);
		
		SendMessage(ghPtLv, 4171, (WPARAM)i, (LPARAM)vpMemLvi); // 4171
		
		ReadProcessMemory(hProc, vpMemLvi, &lviRead, sizeof(lviRead), NULL);

		iImg = lviRead.iImage;
		//sprintf_s(szMsg, "Item index: %d iImg: %d \n", i, iImg);
		//OutputDebugStringA(szMsg);

		if(iImg == 10)
		{ 
			SendMessage(ghPtLv, 4141, (WPARAM)i, (LPARAM)vpMemLvi);    // 4141
			
			ReadProcessMemory(hProc, lviRead.pszText, &gszCurrentNick, sizeof(gszCurrentNick), NULL);
			bRet = TRUE;
			break;
		}
		else
		{
			sprintf_s(gszCurrentNick, "a");
			bRet = FALSE;
		}
		
	}
#ifdef DEBUG
	sprintf_s(szMsg, "Image: %d Nickname: %s \n",  iImg, gszCurrentNick);
	OutputDebugStringA(szMsg);
#endif // DEBUG
	// Cleanup 
	if (vpMemLvi != NULL) VirtualFreeEx(hProc, vpMemLvi, 0, MEM_RELEASE);
	if (pXItemNameBuff != NULL) VirtualFreeEx(hProc, pXItemNameBuff, 0, MEM_RELEASE);
CloseHandle(hProc);

	return bRet;
}


/// Start Monitoring the mic que
void StartStopMonitoring(void)
{
	if (!ghPtRoom)
	{
		msga("Error: No Paltalk Room!\n [Get Pt] first!");
		return;
	}
	if (!gbMonitor)
	{
		SetTimer(ghMain, IDT_MONITORTIMER, 1000, NULL);
		gbMonitor = TRUE;
		SendDlgItemMessageW(ghMain, IDC_BUTTON_START, WM_SETTEXT, 0, (LPARAM)L"Stop");
	}
	else
	{
		KillTimer(ghMain, IDT_MONITORTIMER);
		sprintf_s(gszSavedNick, "a");
		sprintf_s(gszCurrentNick, "a");
		MicTimerReset();
		gbMonitor = FALSE;
		SendDlgItemMessageW(ghMain, IDC_BUTTON_START, WM_SETTEXT, 0, (LPARAM)L"Start");
	}
}

/// Sending Text to Paltalk 
void CopyPaste2Paltalk(char* szMsg)
{
	char szOut[MAX_PATH] = { '\0' };
	
	if (strlen(gszCurrentNick) < 2) return;
	else if (SendMessageA(ghCbSend, BM_GETCHECK, (WPARAM)0, (LPARAM)0) != BST_CHECKED) return;

	BOOL bRes = SetForegroundWindow(ghPtMain);
	Sleep(500);

	sprintf_s(szOut, MAX_PATH, "*** %s ***", szMsg);

	for (int i = 0; i < strlen(szOut); i++)
	{
		SendMessageA(ghPtMain, WM_CHAR, (WPARAM)szOut[i], 0);
	}
	SendMessageA(ghPtMain, WM_KEYDOWN, (WPARAM)VK_RETURN, 0);
	SendMessageA(ghPtMain, WM_KEYUP, (WPARAM)VK_RETURN, 0);

}

