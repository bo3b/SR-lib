#pragma once

//***************************************************************************
// RenderToTexture implementation
// Helifax (Octavian Vasilovici) Dec. 2021
//***************************************************************************

#include <d3d11.h>
#include <stdint.h>

namespace TextureRenderer
{
	struct Backup_DX11_State
	{
		UINT ScissorRectsCount = 0;
		UINT ViewportsCount = 0;
		D3D11_RECT ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = { 0 };
		D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = { 0 };
		ID3D11RasterizerState* RS = nullptr;
		ID3D11BlendState* BlendState = nullptr;
		FLOAT BlendFactor[4] = { 0 };
		UINT SampleMask = 0;
		UINT StencilRef = 0;
		ID3D11DepthStencilState* DepthStencilState = nullptr;
		ID3D11ShaderResourceView* PSShaderResource = nullptr;
		ID3D11SamplerState* PSSampler = nullptr;
		ID3D11PixelShader* PS = nullptr;
		ID3D11VertexShader* VS = nullptr;
		UINT PSInstancesCount = 0;
		UINT VSInstancesCount = 0;
		ID3D11ClassInstance* PSInstances[256] = { 0 };
		ID3D11ClassInstance* VSInstances[256] = { 0 };  // 256 is max according to PSSetShader documentation
		D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		ID3D11Buffer* IndexBuffer = nullptr;
		ID3D11Buffer* VertexBuffer = nullptr;
		ID3D11Buffer* VSConstantBuffer = nullptr;
		UINT IndexBufferOffset = 0;
		UINT VertexBufferStride = 0;
		UINT VertexBufferOffset = 0;
		DXGI_FORMAT IndexBufferFormat = DXGI_FORMAT_UNKNOWN;
		ID3D11InputLayout* InputLayout = nullptr;
		ID3D11RenderTargetView* RenderTargetView = nullptr;
		ID3D11DepthStencilView* DepthStencilView = nullptr;
	};
	//----------------------------------------------------------------------------

	struct RenderDetails
	{
		IDXGISwapChain* _game_swap_chain = nullptr;
		ID3D11Device* _device = nullptr;
		ID3D11DeviceContext* _context = nullptr;

		ID3D11Texture2D* _back_buffer_tex = nullptr;
		ID3D11ShaderResourceView* _back_buffer_SRV = nullptr;
		ID3D11RenderTargetView* _render_RTV = nullptr;

		ID3D11SamplerState* _sampler = nullptr;
		ID3D11VertexShader* _vertex_shader = nullptr;
		ID3D11PixelShader* _fragment_shader = nullptr;

		ID3D11DepthStencilState* _depth_SS = nullptr;
		ID3D11RasterizerState* _rasterizer_state = nullptr;

		float _game_width = 0;
		float _game_height = 0;
	};
	//----------------------------------------------------------------------------

	class RenderToTex
	{
	public:
		RenderToTex()
		{
		}
		//----------------------------------------------------------------------------

		~RenderToTex()
		{
			RemoveResources();

			if (_details._sampler)
				_details._sampler->Release();
			if (_details._vertex_shader)
				_details._vertex_shader->Release();
			if (_details._fragment_shader)
				_details._fragment_shader->Release();
			if (_details._depth_SS)
				_details._depth_SS->Release();
			if (_details._rasterizer_state)
				_details._rasterizer_state->Release();
		}
		//----------------------------------------------------------------------------

		void UpdateDetails(RenderDetails details);
		//----------------------------------------------------------------------------

		ID3D11RenderTargetView* GetRTV();
		//----------------------------------------------------------------------------

		void Render(ID3D11Texture2D* backbuffer, uint32_t layer);
		//----------------------------------------------------------------------------

		void SetViewPort(D3D11_VIEWPORT viewport)
		{
			_viewport = viewport;
		}
		//----------------------------------------------------------------------------

		ID3D11DeviceContext* GetDeviceContext()
		{
			return _details._context;
		}
		//----------------------------------------------------------------------------

		void RemoveResources();
		//----------------------------------------------------------------------------

		void SaveD3DState();
		//----------------------------------------------------------------------------

		void RestoreD3DState();
		//----------------------------------------------------------------------------

	private:
		D3D11_VIEWPORT _viewport{};
		Backup_DX11_State _d3dState{};

	public:
		RenderDetails _details{};
	};
	//----------------------------------------------------------------------------

}  // namespace Vk3DVision
