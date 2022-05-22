#pragma once

//***************************************************************************
// SpatialLabs3D integration for 3DMigoto
// Helifax (Octavian Vasilovici) Dec. 2021
//***************************************************************************

// Spatial Labs
#include "sr/weaver/dxweaver.h"
#include "sr/sense/display/switchablehint.h"
#include "DataTypes.h"

// Some forward declares
namespace SR
{
	class SRContext;
	class PredictingDX11Weaver;
}  // namespace SR

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
	//-------------------------------------------------------------------------

	void StartSRWeaver(SpatialLabs3D::WeavingInfo info);
	//---------------------------------------------------------------------

	void StopSRWeaver();
	//---------------------------------------------------------------------

	void GetRTV(ID3D11RenderTargetView** render_target);
	//---------------------------------------------------------------------

	void Render();
	//---------------------------------------------------------------------

	void EnableLenses();
	//---------------------------------------------------------------------

	void DisableLenses();
	//---------------------------------------------------------------------

}  // namespace SpatialLabs3D
