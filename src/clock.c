//NVD Clock
//Copyright (C) 2024-2025, Nikolay Dudkin

//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#ifndef UNICODE
	#define UNICODE
#endif

#define __STDC_WANT_LIB_EXT1__ 1
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <wchar.h>

#define BUFSIZE 256
#define TIMER_DEFAULT 250
#define TIMER_FAST 100
#define WM_CLOCKNOTIFYICONMESSAGE (WM_USER + 1)

struct
{
	HWND hWnd;
	RECT windowRect;
	HFONT hFontBig;
	HFONT hFontSmall;
	HBRUSH hBrush;
	int topmost;
	int hidden;
} windowData;

struct
{
	ULONGLONG tStart;
	int timerState;
	int passedH;
	int passedM;
	int passedS;
	int passedMS;
} timerData;

static void UpdateTimerData(int newTimerState)
{
	ULONGLONG ticks = GetTickCount64();
	timerData.timerState = newTimerState;

	switch (timerData.timerState)
	{
		case 0: //Ready
			timerData.tStart = 0;
			timerData.passedH = timerData.passedM = timerData.passedS = timerData.passedMS = 0;
		break;

		case 1: //Running
			if(!timerData.tStart)
				timerData.tStart = ticks;
		default: //Stopped
			timerData.passedMS = (int) (ticks - timerData.tStart);
			timerData.passedS = timerData.passedMS / 1000;
			timerData.passedMS -= timerData.passedS * 1000;
			timerData.passedH = timerData.passedS / 3600;
			timerData.passedM = (timerData.passedS - timerData.passedH * 3600) / 60;
			timerData.passedS = timerData.passedS - timerData.passedH * 3600 - timerData.passedM * 60;
		break;
	}
}

static void SwitchTimerState()
{
	switch (timerData.timerState)
	{
	case 0: // Ready
		UpdateTimerData(1);
		KillTimer(windowData.hWnd, 1);
		SetTimer(windowData.hWnd, 1, TIMER_FAST, (TIMERPROC)NULL);
		break;

	case 1: //Running
		UpdateTimerData(2);
		KillTimer(windowData.hWnd, 1);
		SetTimer(windowData.hWnd, 1, TIMER_DEFAULT, (TIMERPROC)NULL);
		break;

	default: //Stopped
		UpdateTimerData(0);
		break;
	}
}

static void WindowShow()
{
	KillTimer(windowData.hWnd, 1);

	if (windowData.hidden)
	{
		ShowWindow(windowData.hWnd, SW_HIDE);
	}
	else
	{
		SetTimer(windowData.hWnd, 1, timerData.timerState == 1 ? TIMER_FAST : TIMER_DEFAULT, (TIMERPROC)NULL);
		ShowWindow(windowData.hWnd, SW_SHOWNORMAL);
		SetWindowPos(windowData.hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		if (!windowData.topmost)
			SetWindowPos(windowData.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		RedrawWindow(windowData.hWnd, NULL, NULL, RDW_INVALIDATE);
	}
}

static int RegisterStartup(HKEY baseKey, wchar_t* name, wchar_t* args)
{
	HKEY key;
	wchar_t* path = (wchar_t*)malloc((MAX_PATH + wcslen(args) + 1) * sizeof(wchar_t));

	if (path == NULL)
		return 1;

	memset(path, 0, (MAX_PATH + wcslen(args) + 1) * sizeof(wchar_t));

	if (!GetModuleFileName(NULL, path, MAX_PATH))
	{
		free(path);
		return 2;
	}

	wcsncat_s(path, MAX_PATH + wcslen(args), args, wcslen(args));

	if (RegOpenKeyEx(baseKey, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ | KEY_WRITE, &key) != ERROR_SUCCESS)
	{
		free(path);
		return 3;
	}

	if (RegSetValueEx(key, name, 0, REG_SZ, (BYTE*) path, (DWORD) (wcslen(path) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
	{
		RegCloseKey(key);
		free(path);
		return 4;
	}

	RegCloseKey(key);
	free(path);
	return 0;
}

static int UnregisterStartup(HKEY baseKey, wchar_t* name)
{
	HKEY key;

	if (RegOpenKeyEx(baseKey, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ | KEY_WRITE, &key) != ERROR_SUCCESS)
		return 1;

	if (RegDeleteValue(key, name) != ERROR_SUCCESS)
	{
		RegCloseKey(key);
		return 2;
	}

	RegCloseKey(key);
	return 0;
}

static void ShowHelp()
{
	MessageBox(NULL, L"NVD Clock\r\n(C) 2024-2025, Nikolay Dudkin\r\n\r\nControls:\r\nWIN + ESC or control + left doubleclick - close\r\nESC or left doubleclick - hide\r\nF1 - help\r\nF2 or right click - stopwatch\r\nF3 or control + right click - toggle topmost\r\n\r\nCommand line arguments:\r\nautostart=normal - auto start at boot\r\nautostart=hidden - auto start at boot, hidden\r\nautostart=disable - disable auto start at boot\r\nhidden - start hidden", L"NVD Clock", MB_OK | MB_ICONINFORMATION);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static SYSTEMTIME pTime;

	SYSTEMTIME cTime;
	HDC hDC;
	PAINTSTRUCT ps;
	HGDIOBJ hOldFont;
	LRESULT hit;
	wchar_t buf[BUFSIZE];

	switch(message)
	{
		case WM_CREATE:
			SetLayeredWindowAttributes(hWnd, 0, 216, LWA_ALPHA);
			UpdateTimerData(0);
			SetTimer(hWnd, 1, TIMER_DEFAULT, (TIMERPROC) NULL);
		break;
		
		case WM_DESTROY:
			PostQuitMessage(0);
		break;

		case WM_TIMER:
			if(timerData.timerState == 1)
			{
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
			}
			else
			{
				GetLocalTime(&cTime);
				if(cTime.wHour != pTime.wHour || cTime.wMinute != pTime.wMinute || cTime.wSecond != pTime.wSecond)
				{
					memcpy(&pTime, &cTime, sizeof(SYSTEMTIME));
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
				}
			}
		break;

		case WM_PAINT:
			hDC = BeginPaint(hWnd, &ps);
				FillRect(hDC, &ps.rcPaint, windowData.hBrush);
				SetBkMode(hDC, TRANSPARENT);
				SetTextColor(hDC, RGB(255, 255, 255));
				SetTextAlign(hDC, TA_CENTER);

				GetLocalTime(&cTime);

				hOldFont = SelectObject(hDC, windowData.hFontBig);
				memset(buf, 0, sizeof(wchar_t) * BUFSIZE);
				swprintf(buf, BUFSIZE - 1, L"%s%02d:%02d:%02d%s", windowData.topmost ? L".": L"", cTime.wHour, cTime.wMinute, cTime.wSecond, windowData.topmost ? L".": L"");
				TextOut(hDC, windowData.windowRect.right / 2, 0, buf, (int) wcslen(buf));

				if(timerData.timerState == 1)
					UpdateTimerData(1);

				SelectObject(hDC, windowData.hFontSmall);
				memset(buf, 0, sizeof(wchar_t) * BUFSIZE);
				swprintf(buf, BUFSIZE - 1, L"%s%02d:%02d:%02d.%03d%s", timerData.timerState == 1 ? L".": L"", timerData.passedH, timerData.passedM, timerData.passedS, timerData.passedMS, timerData.timerState == 1 ? L".": L"");
				TextOut(hDC, windowData.windowRect.right / 2, 45, buf, (int) wcslen(buf));

				SelectObject(hDC, hOldFont);
			EndPaint(hWnd, &ps);
		break;

		case WM_KEYUP:
			switch(wParam)
			{
				case VK_ESCAPE:
					if(GetKeyState(VK_LWIN) & 0x8000)
						DestroyWindow(hWnd);
					else
					{
						windowData.hidden = ~0;
						WindowShow();
					}
				break;

				case VK_F1:
					ShowHelp();
				break;

				case VK_F2:
					SwitchTimerState();
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
				break;

				case VK_F3:
					windowData.topmost = ~windowData.topmost;
					WindowShow();
				break;

				default:
				break;
			}
		break;

		case WM_NCHITTEST:
			hit = DefWindowProc(hWnd, message, wParam, lParam);
			if (hit == HTCLIENT)
				hit = HTCAPTION;
		return hit;

		case WM_LBUTTONDBLCLK:
		case WM_NCLBUTTONDBLCLK:
			if(GetKeyState(VK_CONTROL) & 0x8000)
				DestroyWindow(hWnd);
			else
			{
				windowData.hidden = ~windowData.hidden;
				WindowShow();
			}
		break;

		case WM_RBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_NCRBUTTONDBLCLK:
			if(GetKeyState(VK_CONTROL) & 0x8000)
				windowData.topmost = ~windowData.topmost;
			else
				SwitchTimerState();
			WindowShow();
		break;

		case WM_CLOCKNOTIFYICONMESSAGE:
			switch (lParam)
			{
				case WM_LBUTTONDOWN:
					windowData.hidden = ~windowData.hidden;
					WindowShow();
				break;

				case WM_RBUTTONDOWN:
					windowData.hidden = 0;
					if(GetKeyState(VK_CONTROL) & 0x8000)
						windowData.topmost = ~windowData.topmost;
					else
						SwitchTimerState();
					WindowShow();
					break;

				case WM_LBUTTONDBLCLK:
					if(GetKeyState(VK_CONTROL) & 0x8000)
						DestroyWindow(hWnd);
					break;

				default:
					break;
			}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

static int WindowOpen(HINSTANCE hInstance)
{
	wchar_t *windowName = L"NVD clock";
	wchar_t *className = L"NVD clock class";
	RECT rect;
	WNDCLASSEX wcex;
	MSG msg;
	LOGFONT lf;
	HICON hMainIcon;
	NOTIFYICONDATA notifyIconData;

	memset(&rect, 0, sizeof(RECT));
	memset(&windowData.windowRect, 0, sizeof(RECT));
	windowData.windowRect.right = 170;
	windowData.windowRect.bottom = 65;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	rect.left = rect.right - windowData.windowRect.right - 1;
	rect.top = rect.bottom - windowData.windowRect.bottom - 1;
	rect.right = windowData.windowRect.right;
	rect.bottom = windowData.windowRect.bottom;

	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC) WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH) COLOR_WINDOW;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= className;
	wcex.hIconSm		= NULL;

	if(!RegisterClassEx(&wcex))
		return 1;

	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = 48;
	lf.lfWeight = FW_NORMAL;
	lf.lfOutPrecision = OUT_TT_PRECIS;
	lf.lfQuality = ANTIALIASED_QUALITY;
	wcscpy_s(lf.lfFaceName, LF_FACESIZE - 1, L"Segoe UI Light");
	windowData.hFontBig = CreateFontIndirect(&lf);
	if(!windowData.hFontBig)
	{
		UnregisterClass(className, hInstance);
		return 1;
	}

	lf.lfHeight = 18;
	windowData.hFontSmall = CreateFontIndirect(&lf);
	if(!windowData.hFontSmall)
	{
		UnregisterClass(className, hInstance);
		return 1;
	}

	windowData.hBrush = CreateSolidBrush(RGB(38, 37, 36));
	if(!windowData.hBrush)
	{
		DeleteObject(windowData.hFontSmall);
		DeleteObject(windowData.hFontBig);
		UnregisterClass(className, hInstance);
		return 1;
	}

	hMainIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
	if (!hMainIcon)
	{
		DeleteObject(windowData.hBrush);
		DeleteObject(windowData.hFontSmall);
		DeleteObject(windowData.hFontBig);
		UnregisterClass(className, hInstance);
		return 1;
	}

	windowData.hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, className, windowName, WS_VISIBLE | WS_POPUP, rect.left, rect.top, rect.right, rect.bottom, NULL, NULL, hInstance, NULL);

	if(!windowData.hWnd)
	{
		DeleteObject(windowData.hBrush);
		DeleteObject(windowData.hFontSmall);
		DeleteObject(windowData.hFontBig);
		UnregisterClass(className, hInstance);
		return 1;
	}

	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));
	notifyIconData.cbSize			= sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd				= windowData.hWnd;
	notifyIconData.uID				= 1;
	notifyIconData.uFlags			= NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage	= WM_CLOCKNOTIFYICONMESSAGE;
	notifyIconData.hIcon			= hMainIcon;

	wcsncpy_s(notifyIconData.szTip, 64, windowName, 64);

	if (!Shell_NotifyIcon(NIM_ADD, &notifyIconData))
	{
		DestroyIcon(hMainIcon);
		DeleteObject(windowData.hFontSmall);
		DeleteObject(windowData.hFontBig);
		DeleteObject(windowData.hBrush);
		UnregisterClass(className, hInstance);
		return 1;
	}

	ShowWindow(windowData.hWnd, windowData.hidden ? SW_HIDE : SW_SHOW);

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);

	DestroyIcon(hMainIcon);
	DeleteObject(windowData.hFontSmall);
	DeleteObject(windowData.hFontBig);
	DeleteObject(windowData.hBrush);
	UnregisterClass(className, hInstance);

	return 0;
}

#ifdef __WATCOMC__
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
#else
	int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
#endif
{
	if(!wcscmp(lpCmdLine, L"?") || !wcscmp(lpCmdLine, L"-?") || !wcscmp(lpCmdLine, L"/?") || !wcscmp(lpCmdLine, L"help") || !wcscmp(lpCmdLine, L"-help") || !wcscmp(lpCmdLine, L"/help") || !wcscmp(lpCmdLine, L"--help"))
	{
		ShowHelp();
		return 0;
	}

	if(!wcscmp(lpCmdLine, L"autostart=normal"))
	{
		if(!RegisterStartup(HKEY_CURRENT_USER, L"NVDClock", L""))
			MessageBox(NULL, L"Normal auto start enabled", L"NVD Clock", MB_OK | MB_ICONINFORMATION);
		return 0;
	}

	if(!wcscmp(lpCmdLine, L"autostart=hidden"))
	{
		if(!RegisterStartup(HKEY_CURRENT_USER, L"NVDClock", L" hidden"))
			MessageBox(NULL, L"Hidden auto start enabled", L"NVD Clock", MB_OK | MB_ICONINFORMATION);
		return 0;
	}

	if(!wcscmp(lpCmdLine, L"autostart=disabled"))
	{
		if(!UnregisterStartup(HKEY_CURRENT_USER, L"NVDClock"))
			MessageBox(NULL, L"Auto start disabled", L"NVD Clock", MB_OK | MB_ICONINFORMATION);
		return 0;
	}

	memset(&windowData, 0, sizeof(windowData));

	if(!wcscmp(lpCmdLine, L"hidden"))
		windowData.hidden = ~0;

	return WindowOpen(hInstance);
}
