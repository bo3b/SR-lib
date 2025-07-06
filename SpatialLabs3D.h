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
		~SRWeaver() = default;

		void Start(const WeavingInfo& info);
		void Stop();
		void GetRenderTarget(ID3D11RenderTargetView** render_target);
		void Render();

	private:
		SRWeaver() {};
		static SRWeaver* instance;
		SRWeaver(const SRWeaver&);
		SRWeaver& operator=(const SRWeaver&);

		SR::SRContext* _context = nullptr;
		SR::PredictingDX11Weaver* _weaver = nullptr;

		uint32_t _render_width{};
		uint32_t _render_height{};
	};
}  // namespace SpatialLabs3D

//-------------------------------------------------------------------------
