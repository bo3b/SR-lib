/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <window.h>
#include <shellscalingapi.h>

bool window_close_flag = false;
Window* Window::Instance = nullptr;

#ifdef WINDOWED_APPLICATION
const DWORD WindowStyle = WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
#else
const DWORD WindowStyle = WS_OVERLAPPED | WS_VISIBLE;
#endif

const DWORD WindowStyleEx = 0;

LRESULT CALLBACK WindowMessageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        window_close_flag = true;
        break;
    }

    case WM_SIZE:
    {
        Window::GetWindow().NotifySizeChanged({ LOWORD(lParam), HIWORD(lParam) });
    }

    default:
    {
        return DefWindowProcA(hwnd, message, wParam, lParam);
    }
    }
    return 0;
}

Window::Window(const char* ClassName, const char* WindowTitle)
{
    Instance = this;

    HINSTANCE ProcessHandle = GetModuleHandleA(nullptr);

    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.hInstance = ProcessHandle;
    wc.lpszClassName = ClassName;
    wc.lpfnWndProc = WindowMessageProc;
    wc.style = CS_DBLCLKS;
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpszMenuName = nullptr;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

    RegisterClassExA(&wc);
    WindowHandle = CreateWindowExA(WindowStyleEx, ClassName, WindowTitle, WindowStyle, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, ProcessHandle, nullptr);
}

void Window::Update()
{
    MSG messages;

    while (PeekMessageA(&messages, WindowHandle, 0, 0, 1))
    {
        TranslateMessage(&messages);
        DispatchMessageA(&messages);
    }
}

HWND Window::GetHandle()
{
    return WindowHandle;
}

bool Window::ShouldClose()
{
    return window_close_flag;
}

DirectX::XMINT2 Window::GetSize()
{
    return Size;
}

void Window::NotifySizeChanged(DirectX::XMINT2 NewSize)
{
    Size = NewSize;
}

Window& Window::GetWindow()
{
    return *Instance;
}

void Window::SetPositionOnTop(size_t x, size_t y, size_t width, size_t height)
{
    SetWindowLongA(WindowHandle, GWL_STYLE, WindowStyle);
    SetWindowPos(WindowHandle, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
}
