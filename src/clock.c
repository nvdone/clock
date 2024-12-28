//NVD Clock
//Copyright (C) 2024, Nikolay Dudkin

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

RECT windowRect;
HFONT hFontBig;
HFONT hFontSmall;
HBRUSH hBrush;

struct
{
	ULONGLONG tStart;
	int timerState;
	int passedH;
	int passedM;
	int passedS;
	int passedMS;
} timerData;

static void UpdateTimerData(int timerState)
{
	ULONGLONG ticks = GetTickCount64();
	timerData.timerState = timerState;

	switch (timerState)
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

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int topmost = 0;
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
			UpdateTimerData(0);
			SetLayeredWindowAttributes(hWnd, 0, 216, LWA_ALPHA);
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
				FillRect(hDC, &ps.rcPaint, hBrush);
				SetBkMode(hDC, TRANSPARENT);
				SetTextColor(hDC, RGB(255, 255, 255));
				SetTextAlign(hDC, TA_CENTER);

				GetLocalTime(&cTime);

				hOldFont = SelectObject(hDC, hFontBig);
				memset(buf, 0, sizeof(wchar_t) * BUFSIZE);
				swprintf(buf, BUFSIZE - 1, L"%s%02d:%02d:%02d%s", topmost ? L".": L"", cTime.wHour, cTime.wMinute, cTime.wSecond, topmost ? L".": L"");
				TextOut(hDC, windowRect.right / 2, 0, buf, (int) wcslen(buf));

				if(timerData.timerState == 1)
					UpdateTimerData(1);

				SelectObject(hDC, hFontSmall);
				memset(buf, 0, sizeof(wchar_t) * BUFSIZE);
				swprintf(buf, BUFSIZE - 1, L"%s%02d:%02d:%02d.%03d%s", timerData.timerState == 1 ? L".": L"", timerData.passedH, timerData.passedM, timerData.passedS, timerData.passedMS, timerData.timerState == 1 ? L".": L"");
				TextOut(hDC, windowRect.right / 2, 45, buf, (int) wcslen(buf));

				SelectObject(hDC, hOldFont);
			EndPaint(hWnd, &ps);
		break;

		case WM_LBUTTONDOWN:
		case WM_NCLBUTTONDOWN:
			if(GetKeyState(VK_CONTROL) & 0x8000)
			{
				SetWindowPos(hWnd, (topmost =~ topmost) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
				break;
			}
			return DefWindowProc(hWnd, message, wParam, lParam);
		break;

		case WM_RBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_NCRBUTTONDBLCLK:
			switch(timerData.timerState)
			{
				case 0: // Ready
					UpdateTimerData(1);
					KillTimer(hWnd, 1);
					SetTimer(hWnd, 1, TIMER_FAST, (TIMERPROC) NULL);
				break;

				case 1: //Running
					UpdateTimerData(2);
					KillTimer(hWnd, 1);
					SetTimer(hWnd, 1, TIMER_DEFAULT, (TIMERPROC) NULL);
				break;

				default: //Stopped
					UpdateTimerData(0);
				break;
			}
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
		break;

		case WM_LBUTTONDBLCLK:
		case WM_NCLBUTTONDBLCLK:
			DestroyWindow(hWnd);
		break;

		case WM_KEYUP:
			switch(wParam)
			{
				case VK_ESCAPE:
					DestroyWindow(hWnd);
				break;

				case VK_F1:
					MessageBox(NULL, L"NVD Clock\r\n(C) 2024, Nikolay Dudkin\r\n\r\nESC - close\r\nF1 - help\r\nF2 - toggle topmost\r\nF3 - stopwatch\r\nLeft doubleclick - close\r\nControl + left click - toggle topmost\r\nRight click - stopwatch", L"NVD Clock", MB_OK | MB_ICONINFORMATION);
				break;

				case VK_F2:
					SetWindowPos(hWnd, (topmost =~ topmost) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
				break;

				case VK_F3:
					switch(timerData.timerState)
					{
						case 0: // Ready
							UpdateTimerData(1);
							KillTimer(hWnd, 1);
							SetTimer(hWnd, 1, TIMER_FAST, (TIMERPROC) NULL);
						break;

						case 1: //Running
							UpdateTimerData(2);
							KillTimer(hWnd, 1);
							SetTimer(hWnd, 1, TIMER_DEFAULT, (TIMERPROC) NULL);
						break;

						default: //Stopped
							UpdateTimerData(0);
						break;
					}
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
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

		case WM_USER + 1:
			switch (lParam)
			{
			case WM_RBUTTONDOWN:
				switch(timerData.timerState)
				{
					case 0: // Ready
						UpdateTimerData(1);
						KillTimer(hWnd, 1);
						SetTimer(hWnd, 1, TIMER_FAST, (TIMERPROC) NULL);
					break;

					case 1: //Running
						UpdateTimerData(2);
						KillTimer(hWnd, 1);
						SetTimer(hWnd, 1, TIMER_DEFAULT, (TIMERPROC) NULL);
					break;

					default: //Stopped
						UpdateTimerData(0);
					break;
				}
				SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				if(!topmost)
					SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
				break;

			case WM_LBUTTONDOWN:
				SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				if(GetKeyState(VK_CONTROL) & 0x8000)
					topmost =~ topmost;
				if(!topmost)
					SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
				break;

			case WM_LBUTTONDBLCLK:
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

static int WindowShow(HINSTANCE hInstance)
{
	wchar_t *windowName = L"NVD clock";
	wchar_t *className = L"NVD clock class";
	RECT rect;
	WNDCLASSEX wcex;
	HWND hWindow;
	MSG msg;
	LOGFONT lf;
	HICON hMainIcon;
	NOTIFYICONDATA notifyIconData;

	memset(&rect, 0, sizeof(RECT));
	memset(&windowRect, 0, sizeof(RECT));
	windowRect.right = 170;
	windowRect.bottom = 65;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	rect.left = rect.right - windowRect.right - 1;
	rect.top = rect.bottom - windowRect.bottom - 1;
	rect.right = windowRect.right;
	rect.bottom = windowRect.bottom;

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
	hFontBig = CreateFontIndirect(&lf);
	if(!hFontBig)
	{
		UnregisterClass(className, hInstance);
		return 1;
	}

	lf.lfHeight = 18;
	hFontSmall = CreateFontIndirect(&lf);
	if(!hFontSmall)
	{
		UnregisterClass(className, hInstance);
		return 1;
	}

	hBrush = CreateSolidBrush(RGB(38, 37, 36));
	if(!hBrush)
	{
		DeleteObject(hFontSmall);
		DeleteObject(hFontBig);
		UnregisterClass(className, hInstance);
		return 1;
	}

	hMainIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
	if (!hMainIcon)
	{
		DeleteObject(hBrush);
		DeleteObject(hFontSmall);
		DeleteObject(hFontBig);
		UnregisterClass(className, hInstance);
		return 1;
	}

	hWindow = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, className, windowName, WS_VISIBLE | WS_POPUP, rect.left, rect.top, rect.right, rect.bottom, NULL, NULL, hInstance, NULL);

	if(!hWindow)
	{
		DeleteObject(hBrush);
		DeleteObject(hFontSmall);
		DeleteObject(hFontBig);
		UnregisterClass(className, hInstance);
		return 1;
	}

	notifyIconData.cbSize			= sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd				= hWindow;
	notifyIconData.uID				= 1;
	notifyIconData.uFlags			= NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage	= WM_USER + 1;
	notifyIconData.hIcon			= hMainIcon;

	#ifndef __WATCOMC__
	wcsncpy_s(notifyIconData.szTip, 64, windowName, 64);
	#else
	wcsncpy(notifyIconData.szTip, windowName, 64);
	#endif

	if (!Shell_NotifyIcon(NIM_ADD, &notifyIconData))
	{
		DestroyIcon(hMainIcon);
		DeleteObject(hFontSmall);
		DeleteObject(hFontBig);
		DeleteObject(hBrush);
		UnregisterClass(className, hInstance);
		return 1;
	}

	ShowWindow(hWindow, SW_SHOW);

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);

	DestroyIcon(hMainIcon);
	DeleteObject(hFontSmall);
	DeleteObject(hFontBig);
	DeleteObject(hBrush);
	UnregisterClass(className, hInstance);

	return 0;
}

#ifdef __WATCOMC__
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
#else
	int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
#endif
{
	return WindowShow(hInstance);
}
