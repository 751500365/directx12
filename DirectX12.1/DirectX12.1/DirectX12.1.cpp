// DirectX12.1.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "DirectX12.1.h"

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

#define MAX_LOADSTRING 100


bool running = true;

GraphicsClass* m_Graphics;

DWORD WINAPI RunMainLoop(LPVOID lpParam)
{
    while (running)
    {
        Sleep(16);
    }
    return 0;
}



//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//

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


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
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

    SetWindowPos(hwnd, NULL, 40, 40, 640, 480, SWP_SHOWWINDOW);

    m_Graphics = new GraphicsClass;

   /* m_Graphics->Initialize(1050, 1680, hwnd);*/
    m_Graphics->Initialize(480, 640, hwnd);

    CreateThread(0, 0, RunMainLoop, 0, 0, 0);


    while (running)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        m_Graphics->Frame();
        Sleep(16);
    }

    return 0;
}

