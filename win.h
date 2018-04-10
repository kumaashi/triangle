#pragma once

#include <windows.h>

class Window {
	static LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
		switch( msg ) {
			case WM_SYSCOMMAND:
				switch( ( wParam & 0xFFF0 ) ) {
					case SC_MONITORPOWER:
					case SC_SCREENSAVE:
						return 0;
						break;
				}
				break;
			case WM_CLOSE:
			case WM_DESTROY:
				PostQuitMessage( 0 );
				return 0;
				break;
			case WM_IME_SETCONTEXT:
				lParam &= ~ISC_SHOWUIALL;
				break;
			case WM_KEYDOWN:
				if(wParam == VK_ESCAPE) PostQuitMessage(0);
				break;
		}
		return DefWindowProc( hWnd, msg, wParam, lParam );
	}
	void *pBits = nullptr;
	HWND hWnd = nullptr;
	HINSTANCE hInst = GetModuleHandle(nullptr);
	DWORD style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
	DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD Width = -1;
	DWORD Height = -1;
	DWORD windowWidth = -1;
	DWORD windowHeight = -1;
	float scale = 2.5f;
	HBITMAP hDIB = NULL;
	HDC hDIBDC = NULL;
	HDC hdc = NULL;
public:
	bool ProcMsg() {
		bool isActive = true;
		MSG msg;
		while(PeekMessage(&msg , 0 , 0 , 0 , PM_REMOVE)) {
			if(msg.message == WM_QUIT) {
				isActive = false;
				break;
			} else {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		return isActive;
	}
	~Window() {
		if(hDIB) DeleteObject(hDIB);
	}
	void Present() {
		HDC hdc = GetDC(hWnd);
		StretchBlt(hdc, 0, 0, windowWidth, windowHeight, hDIBDC, 0, 0, Width, Height, SRCCOPY);
		ReleaseDC(hWnd, hdc);
	}
	void *GetBits() {
		return pBits;
	}
	DWORD GetWidth() {
		return Width;
	}
	DWORD GetHeight() {
		return Height;
	}
	HWND GetWindowHandle() {
		return hWnd;
	}
	Window(const char *szAppName, int BmpX, int BmpY) {
		hInst = GetModuleHandle(NULL);
		windowWidth = BmpX * scale;
		windowHeight = BmpY * scale;
		Width = BmpX;
		Height = BmpY;
		RECT rc = {0, 0, windowWidth, windowHeight};
		WNDCLASSEX twc = {
			sizeof( WNDCLASSEX ), CS_CLASSDC | CS_VREDRAW | CS_HREDRAW, MsgProc, 0L, 0L,
			hInst,  LoadIcon(NULL, IDI_APPLICATION), LoadCursor( NULL , IDC_ARROW ),
			( HBRUSH )GetStockObject( BLACK_BRUSH ), NULL, szAppName, NULL
		};
		RegisterClassEx( &twc );
		AdjustWindowRectEx( &rc, style, FALSE, dwExStyle);
		rc.right -= rc.left;
		rc.bottom -= rc.top;
		hWnd = CreateWindowEx(dwExStyle, szAppName, szAppName, style,
				GetSystemMetrics(SM_CXSCREEN) / 2 - (rc.right / 2),
				GetSystemMetrics(SM_CYSCREEN) / 2 - (rc.bottom / 2), 
				rc.right, rc.bottom, NULL, NULL, hInst, NULL );
		ShowWindow(GetWindowHandle(), SW_SHOW);
		UpdateWindow(GetWindowHandle());

		BITMAPINFO bi = {};
		bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
		bi.bmiHeader.biWidth       = BmpX;
		bi.bmiHeader.biHeight      = -BmpY;
		bi.bmiHeader.biPlanes      = 1;
		bi.bmiHeader.biBitCount    = 32;
		bi.bmiHeader.biCompression = BI_RGB;
		hDIB = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&pBits, 0, 0);
		hDIBDC = CreateCompatibleDC(NULL);
		SaveDC(hDIBDC);
		SelectObject(hDIBDC, hDIB);
	}
};
