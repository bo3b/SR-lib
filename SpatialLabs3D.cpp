//***************************************************************************
// SpatialLabs3D integration for 3DMigoto
// Helifax (Octavian Vasilovici) Dec. 2021
//***************************************************************************

#include "SpatialLabs3D.h"

#include <d3d11_1.h>

#pragma optimize("", off)

// Not used
//#include "sr/types.h"
//#include "sr/sense/core/inputstream.h"
//#include "sr/sense/handtracker/handtracker.h"
//#include "sr/sense/eyetracker/eyetracker.h"
//#include "sr/world/display/screen.h"

// Other libs not used!
//#pragma comment(lib, "lib/SimulatedRealityCameras.lib")
//#pragma comment(lib, "lib/SimulatedRealityHandTrackers.lib")
//#pragma comment(lib, "lib/SimulatedRealityFaceTrackers.lib")
//#pragma comment(lib, "lib/SimulatedRealityUserModelers.lib")
//#pragma comment(lib, "lib/DimencoWeaving.lib")
//#pragma comment(lib, "third_party/GLog/lib/glog.lib")
//#pragma comment(lib, "third_party/Leap/lib/LeapC.lib")

// Only link against libs that are used.
// moved to project settings. SDK is x64 only 
//#pragma comment(lib, "lib/SimulatedRealityDirectX.lib").
//#pragma comment(lib, "lib/SimulatedReality.lib")
//#pragma comment(lib, "lib/SimulatedRealityCore.lib")
//#pragma comment(lib, "lib/SimulatedRealityDisplays.lib")
//#pragma comment(lib, "third_party/OpenCV/lib/opencv_world343.lib")

namespace SpatialLabs3D
{
	void SRWeaver::Start(WeavingInfo& info)
	{
		if (info._version != LATEST_VERSION)
		{
			::MessageBoxW(nullptr, L"It looks like you are trying to use a version of Vk3Dvision\nthat is not compatible with this SpatialLabs3D module!", L"Incompatibility Detected!", MB_OK);
			::ExitProcess(-1);
		}

		if (!_context)
		{
			_context = new SR::SRContext();
			_lensHint = SR::SwitchableLensHint::create(*_context);
			_context->initialize();
		}

		if (_weaver)
			delete _weaver;

		// Make a RTV that supports FULL SBS images!
		_render_width = info._render_width;
		_render_height = info._render_height;
		_weaver = new SR::PredictingDX11Weaver(*_context, info._device, info._deviceContext, info._render_width * 2, info._render_height, info._window);

		//*A low latency app would have 1 framebuffer latency, so 16666 microseconds(the generated frame will be presented at next v - sync)
		_weaver->setLatency(16666);
		_context->initialize();
	}

	//-------------------------------------------------------------------------

	void SRWeaver::Stop()
	{
		if (_weaver)
			delete _weaver;

		if (_context)
			delete _context;
	}
	//-------------------------------------------------------------------------

	void SRWeaver::Render()
	{
		// Weave the final Image
		_weaver->weave(_render_width, _render_height);
	}
	//-------------------------------------------------------------------------

	void SRWeaver::GetRenderTarget(ID3D11RenderTargetView** render_target)
	{
		*render_target = _weaver->getFrameBuffer();
	}
	//-------------------------------------------------------------------------

	void SRWeaver::EnableLenses()
	{
		//lensHint->isEnabled() -> Excessive calls to this function will completely kill perf! Instead use our own variable!
		if (!_lens_enabled)
		{
			_lensHint->enable();
			_lens_enabled = true;
		}
	}
	//-------------------------------------------------------------------------

	void SRWeaver::DisableLenses()
	{
		if (_lens_enabled)
		{
			_lensHint->disable();  //Allow the lens to be disabled if the system decides this is desirable
			_lens_enabled = false;
		}
	}
} // namespace SpatialLabs3D

//-------------------------------------------------------------------------

void StartSRWeaver(SpatialLabs3D::WeavingInfo info)
{
    SpatialLabs3D::SRWeaver::Instance().Start(info);
}

void StopSRWeaver()
{
    SpatialLabs3D::SRWeaver::Instance().Stop();
}

void GetRTV(ID3D11RenderTargetView** render_target)
{
    SpatialLabs3D::SRWeaver::Instance().GetRenderTarget(render_target);
}

void Render()
{
    SpatialLabs3D::SRWeaver::Instance().Render();
}

// Not sure where to use this at the moment
void EnableLenses()
{
    SpatialLabs3D::SRWeaver::Instance().EnableLenses();
}

// Not sure where to use this at the moment
void DisableLenses()
{
    SpatialLabs3D::SRWeaver::Instance().DisableLenses();
}
