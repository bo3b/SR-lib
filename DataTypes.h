#pragma once

#include <cstdint>
#include <d3d11.h>

namespace SpatialLabs3D
{
    constexpr uint16_t LATEST_VERSION = 2;
    struct WeavingInfo
    {
        // Identify the structure version.
        const uint16_t _version = LATEST_VERSION;

        // Resolution at which we do the weaving. Always 4K
        uint16_t _render_width{};
        uint16_t _render_height{};

        ID3D11Device* _device{};
        ID3D11DeviceContext* _deviceContext{};
    };

}  // namespace SpatialLabs3D

typedef void (__cdecl *fnStartSRWeaver)(SpatialLabs3D::WeavingInfo info);
typedef void (__cdecl *fnGetRTV)(ID3D11RenderTargetView** render_target);
typedef void (__cdecl *fnRender)();
