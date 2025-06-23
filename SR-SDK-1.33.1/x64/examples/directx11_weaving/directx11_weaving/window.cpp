/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <window.h>
#include <shellscalingapi.h>

bool window_close_flag = false;
Window* Window::Instance = nullptr;

bool toggle_late_latching = false;

const DWORD WindowStyle = WS_OVERLAPPED | WS_VISIBLE;
const DWORD WindowStyleEx = 0;

// Returns true if function completed succesfully, returns false otherwise
bool Window::ensureThisWindowFitsToMonitor() {
    // Get monitor that the window is currently located on.
    HMONITOR windowMonitor = MonitorFromWindow(GetHandle(), MONITOR_DEFAULTTONEAREST);

    // If no monitor handle was returned (no monitor may be attached), don't change window and return false
    if (!windowMonitor) {
        return false;
    }

    // Get monitor rectangle in the virtual screen
    MONITORINFO monitorInfo;
    ZeroMemory(&monitorInfo, sizeof(monitorInfo));
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoA(windowMonitor, &monitorInfo);

    // Get current window rectangle
    RECT windowRect;
    GetWindowRect(GetHandle(), &windowRect);

    // If the monitor rectangle is different from the current window rectangle...
    if (windowRect.left != monitorInfo.rcMonitor.left ||
        windowRect.right != monitorInfo.rcMonitor.right ||
        windowRect.top != monitorInfo.rcMonitor.top ||
        windowRect.bottom != monitorInfo.rcMonitor.bottom) {
        //...set the window rectangle to fit the monitor rectangle
        SetWindowPos(
            GetHandle(),
            HWND_TOP,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER
        );
    }

    // Return true, as function has completed succesfully
    return true;
}

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
            return DefWindowProcA(hwnd, message, wParam, lParam);
        }

        case WM_MOVE:
        {
            Window::GetWindow().NotifyMoved();
            return DefWindowProcA(hwnd, message, wParam, lParam);
        }

        case WM_CHAR:
        {
            if (wParam == 'l' || wParam == 'L')
                toggle_late_latching = true;
            break;
        }

        default:
        {
            return DefWindowProcA(hwnd, message, wParam, lParam);
        }
    }
    return 0;
}

Window::Window(const char* ClassName, const char* WindowTitle, bool windowed, DirectX::XMINT2 WindowSize) : WindowedPreference(windowed)
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
    WindowHandle = CreateWindowExA(WindowStyleEx, ClassName, WindowTitle, WindowStyle, CW_USEDEFAULT, CW_USEDEFAULT, WindowSize.x, WindowSize.y, nullptr, nullptr, ProcessHandle, nullptr);
    windowFitsToMonitor = true;
}

void Window::Update()
{
    MSG messages;

    while (PeekMessageA(&messages, WindowHandle, 0, 0, 1))
    {
        TranslateMessage(&messages);
        DispatchMessageA(&messages);
    }

    if (!windowFitsToMonitor) {
        windowFitsToMonitor = ensureThisWindowFitsToMonitor();
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

bool Window::ShouldToggleLateLatching()
{
    bool prevToggleLateLatching = toggle_late_latching;
    toggle_late_latching = false;
    return prevToggleLateLatching;
}

DirectX::XMINT2 Window::GetSize()
{
    return Size;
}

void Window::NotifySizeChanged(DirectX::XMINT2 NewSize)
{
    Size = NewSize;
    windowFitsToMonitor = false;
}

void Window::NotifyMoved()
{
    windowFitsToMonitor = false;
}

Window& Window::GetWindow()
{
    return *Instance;
}

bool Window::IsWindowed()
{
    return WindowedPreference;
}

void Window::SetPositionOnTop(size_t x, size_t y, size_t width, size_t height)
{
    SetWindowLongA(WindowHandle, GWL_STYLE, WindowStyle);
    SetWindowPos(WindowHandle, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
}
