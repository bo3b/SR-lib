#pragma once

#include <cstdint>
#include <d3d11.h>

namespace SpatialLabs3D
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

}  // namespace SpatialLabs3D

typedef void (*fnStartSRWeaver)(SpatialLabs3D::WeavingInfo info);
typedef void (*fnGetRTV)(ID3D11RenderTargetView** render_target);
typedef void (*fnRender)();
