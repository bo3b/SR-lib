#pragma once

#include <cstdint>
#include <d3d11.h>

namespace SimulatedReality
{
    constexpr uint16_t LATEST_VERSION = 3;  
    struct WeavingInfo
    {
        // Identify the structure version.
        const uint16_t _version = LATEST_VERSION;

        // Resolution at which we do the weaving. Always 4K
        uint16_t _render_width{};
        uint16_t _render_height{};

        // Needs HWND for non-deprecated constructor in 1.33.1
        HWND                 _window {};
        ID3D11Device*        _device {};
        ID3D11DeviceContext* _deviceContext {};
    };

}  // namespace SimulatedReality

// Interface routines for the SR.lib that are exported directly, and
// called via LoadLibrary/GetProcAddress from d3d11.dll. We also make them
// all extern "C" so that we get simple names.

extern "C" void StartSRWeaver(SimulatedReality::WeavingInfo info);
extern "C" void StopSRWeaver();
extern "C" void GetRTV(ID3D11RenderTargetView* *render_target);
extern "C" void Render();
