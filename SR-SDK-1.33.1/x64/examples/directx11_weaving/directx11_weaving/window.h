/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#ifndef WINDOW_H
#define WINDOW_H
#include <DirectXMath.h>
#include <windows.h>

class Window
{
public:
    Window(const char* ClassName, const char* WindowTitle, bool windowed = false, DirectX::XMINT2 WindowSize = {800, 600});

    void Update();

    HWND GetHandle();

    bool ShouldClose();

    bool ShouldToggleLateLatching();

    DirectX::XMINT2 GetSize();

    void NotifySizeChanged(DirectX::XMINT2 Size);

    void NotifyMoved();

    static Window& GetWindow();

    bool IsWindowed();

    /*!
     * \brief Function to reposition, resize and display this window on top
     *
     * \param x position to use in SetWindowPos
     * \param y position to use in SetWindowPos
     * \param width to target in SetWindowPos
     * \param height to target in SetWindowPos
     *
     * Calls SetWindowPos and resets the window style flags.
     * Can be used to set the window fullscreen (windowed)
     */
    void SetPositionOnTop(size_t x, size_t y, size_t width, size_t height);

private:
    HWND WindowHandle;
    DirectX::XMINT2 Size;
    static Window* Instance;
    bool WindowedPreference;
    bool windowFitsToMonitor = false;
    bool ensureThisWindowFitsToMonitor();
};

#endif
