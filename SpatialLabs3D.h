#pragma once

//***************************************************************************
// SpatialLabs3D integration for 3DMigoto
// Helifax (Octavian Vasilovici) Dec. 2021
//***************************************************************************

#include "DataTypes.h"

#include "sr/sense/display/switchablehint.h"
#include "sr/weaver/dxweaver.h"

#include <cstdint>
#include <d3d11_1.h>

namespace SpatialLabs3D
{
	class SRWeaver
	{
	public:
		static SRWeaver& Instance()
		{
			static SRWeaver theInstance;
			return theInstance;
		}
		~SRWeaver() {};

		void Start(WeavingInfo& info);
		void Stop();
		void GetRenderTarget(ID3D11RenderTargetView** render_target);
		void Render();
		void EnableLenses();
		void DisableLenses();

	private:
		SRWeaver() {};
		static SRWeaver* instance;
		SRWeaver(const SRWeaver&);
		SRWeaver& operator=(const SRWeaver&);

		SR::SRContext* _context = nullptr;
		SR::SwitchableLensHint* _lensHint = nullptr;
		SR::PredictingDX11Weaver* _weaver = nullptr;

		uint32_t _render_width{};
		uint32_t _render_height{};

		bool _lens_enabled = true;
	};
}  // namespace SpatialLabs3D

//-------------------------------------------------------------------------

// Interface routines for the SpatialLabs3D.dll that are exported directly, and
// called via LoadLibrary/GetProcAddress from d3d11.dll. We also make them
// all __cdecl so that we don't get munged names.

extern "C" __declspec(dllexport) void __cdecl StartSRWeaver(SpatialLabs3D::WeavingInfo info);
extern "C" __declspec(dllexport) void __cdecl StopSRWeaver();
extern "C" __declspec(dllexport) void __cdecl GetRTV(ID3D11RenderTargetView** render_target);
extern "C" __declspec(dllexport) void __cdecl Render();
extern "C" __declspec(dllexport) void __cdecl EnableLenses();
extern "C" __declspec(dllexport) void __cdecl DisableLenses();
