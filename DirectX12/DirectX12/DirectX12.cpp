// DirectX12.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include<tchar.h> 
#include <windows.h>
#include <windowsx.h>  // GET_X_LPARAM
#include <stdio.h>
#include <math.h>
#include <iostream>

#include <d3d12.h>
#include <dxgi1_4.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace std;

//#ifdef  _UNICODE
//#define __T(x)      L ## x
//#else   /* ndef _UNICODE */
//#define __T(x)      x
//#endif  /* _UNICODE */
//#define _T(x)       __T(x)
//#define _TEXT(x)    __T(x)


// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
int main() 
{
	cout << "Value of str is : " << endl;
}
bool running = true;

DWORD WINAPI RunMainLoop(LPVOID lpParam)
{
	while (running)
	{
		Sleep(16);
	}
	return 0;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CLOSE) running = false;
	if (uMsg == WM_NCHITTEST) {
		RECT win; if (!GetWindowRect(hwnd, &win)) return HTNOWHERE;
		POINT pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		POINT pad = { GetSystemMetrics(SM_CXFRAME), GetSystemMetrics(SM_CYFRAME) };
		bool left = pos.x < win.left + pad.x;
		bool right = pos.x > win.right - pad.x - 1;  // win.right 1 pixel beyond window, right?
		bool top = pos.y < win.top + pad.y;
		bool bottom = pos.y > win.bottom - pad.y - 1;
		if (top && left)     return HTTOPLEFT;
		if (top && right)    return HTTOPRIGHT;
		if (bottom && left)  return HTBOTTOMLEFT;
		if (bottom && right) return HTBOTTOMRIGHT;
		if (left)            return HTLEFT;
		if (right)           return HTRIGHT;
		if (top)             return HTTOP;
		if (bottom)          return HTBOTTOM;
		return HTCAPTION;
	}
	if (uMsg == WM_SIZE) {
		
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"app";
	if (!RegisterClass(&wc)) { MessageBox(0, __T("RegisterClass failed"), 0, 0); return 1; }

	HWND hwnd = CreateWindowEx(
		0, L"app", L"title",
		WS_POPUP | WS_VISIBLE,
		// WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, 0, 0, hInstance, 0);
	if (!hwnd) { MessageBox(0, __T("CreateWindowEx failed"), 0, 0); return 1; }


	CreateThread(0, 0, RunMainLoop, 0, 0, 0);


	while (running)
	{
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Sleep(16);
	}

	return 0;
}

