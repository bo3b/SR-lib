//***************************************************************************
// SimulatedReality integration for 3DMigoto
// Helifax (Octavian Vasilovici) Dec. 2021
// Bo3b: Updated July 2025
//***************************************************************************

#include "SimulatedReality.h"

#include <d3d11_1.h>

//#pragma optimize("", off)

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

// Only link against libs that are used.  The SDK is x32 and x64 now.
// These are for reference only here, and were moved to librarian input.
// They are now marked as DelayLoad, so that we don't have a hard DLL
// requirement for these, and only loaded when:
//  direct_mode = simulated_reality
// In order to avoid warnings, we only need to link against:
//#pragma comment(lib, "lib/SimulatedRealityCore.lib")

// 7-2-25
// Now moving the delay load of the DLLs here, and removed from the main
// geo-11 code in HackerDXGI during setup.  We want to be able to ship geo-11
// without any dangling dlls like the prior SpatialLabs3D.dll, but still
// allow people to use direct_mode = simulated_reality if they have the
// hardware and installed SR dlls.
// That means making this a static lib that is included with geo-11, but
// in here we do the LoadLibrary of the SR dlls needed.


// Any failures are considered fatal, so instead of doing any handling,
// we'll throw up MessageBox to let the user know. The user specified to
// use simulated_reality hardware, but it does not exist. There is no
// logical fallback, so let's just let them know.
void FatalBox(std::wstring error, std::wstring title)
{
	::MessageBoxW(nullptr, error.c_str(), title.c_str(), MB_OK);
	::ExitProcess(-1);
}

namespace SimulatedReality
{
	void SRWeaver::Start(const WeavingInfo& info)
	{
		if (info._version != LATEST_VERSION)
			FatalBox(L"It looks like you are trying to use a version of geo-11\nthat is not compatible with this SimulatedReality module!", L"Incompatibility Detected!");

		if (!_context)
		{
			// Let's double check we are able find the DLLs we need for simulated reality,
			// so we can give a good error if not.
			HMODULE srCore_dll = nullptr;
#ifdef _WIN64
			 srCore_dll = LoadLibrary(L"SimulatedRealityCore.dll");
#else
			srCore_dll = LoadLibrary(L"SimulatedRealityCore32.dll");
#endif
			if (srCore_dll == nullptr)
				FatalBox(L"Simulated Reality DLLs are not available on your system.\nUnable to use direct_mode=simulated_reality.", L"Missing SR runtime DLLs");

			_context = new SR::SRContext();
			if (_context == nullptr)
				FatalBox(L"Failed to create SRContext for simulated_reality.", L"Missing SR runtime DLLs");
		}
		
		if (_weaver != nullptr)
			delete _weaver;

		// Make a RTV that supports FULL SBS images!
		_render_width = info._render_width;
		_render_height = info._render_height;
		_weaver = new SR::PredictingDX11Weaver(*_context, info._device, info._deviceContext, info._render_width * 2, info._render_height, info._window);

		// We are required to pass HWND now, so let it figure out latency.
		_weaver->setLatencyInFrames(1);

		_context->initialize();
	}

	//-------------------------------------------------------------------------

	void SRWeaver::Stop()
	{
		if (_weaver)
			delete _weaver;

		if (_context)
			delete _context; // Will call destructor
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

} // namespace SimulatedReality

//-------------------------------------------------------------------------

void StartSRWeaver(SimulatedReality::WeavingInfo info)
{
    SimulatedReality::SRWeaver::Instance().Start(info);
}

void StopSRWeaver()
{
    SimulatedReality::SRWeaver::Instance().Stop();
}

void GetRTV(ID3D11RenderTargetView** render_target)
{
    SimulatedReality::SRWeaver::Instance().GetRenderTarget(render_target);
}

void Render()
{
	SimulatedReality::SRWeaver::Instance().Render();
}
