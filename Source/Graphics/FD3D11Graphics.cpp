#include "Graphics/FD3D11Graphics.h"

#include <dxgi1_6.h>
#include <format>

#include "Core/FWindow.h"
#include "Utility/Check.h"
#include "Utility/FLog.h"

using Microsoft::WRL::ComPtr;

void FD3D11Graphics::Initialize(const FWindow& Window)
{
	ComPtr<IDXGIFactory2> Factory;
	CHECK_FATAL(CreateDXGIFactory2(0, IID_PPV_ARGS(Factory.GetAddressOf())));

	ComPtr<IDXGIAdapter1> Adapter;
	ComPtr<IDXGIFactory6> Factory6;
	if (SUCCEEDED(Factory->QueryInterface(IID_PPV_ARGS(Factory6.GetAddressOf()))))
	{
		CHECK_FATAL(Factory6->EnumAdapterByGpuPreference(
		    0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(Adapter.GetAddressOf())));
	}
	else
	{
		CHECK_FATAL(Factory->EnumAdapters1(0, Adapter.GetAddressOf()));
	}

	DXGI_ADAPTER_DESC1 AdapterDesc = {};
	CHECK_FATAL(Adapter->GetDesc1(&AdapterDesc));
	FLog::Info(std::format(
	    L"그래픽 카드 : {} ({} MB)", AdapterDesc.Description, AdapterDesc.DedicatedVideoMemory / 1024 / 1024));

	const D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;
	CHECK_FATAL(D3D11CreateDevice(Adapter.Get(),
	                              D3D_DRIVER_TYPE_UNKNOWN,
	                              nullptr,
	                              0,
	                              &FeatureLevel,
	                              1,
	                              D3D11_SDK_VERSION,
	                              Device.GetAddressOf(),
	                              nullptr,
	                              Context.GetAddressOf()));

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width = static_cast<UINT>(Window.GetWidth());
	SwapChainDesc.Height = static_cast<UINT>(Window.GetHeight());
	SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	CHECK_FATAL(Factory->CreateSwapChainForHwnd(
	    Device.Get(), Window.GetHandle(), &SwapChainDesc, nullptr, nullptr, SwapChain.GetAddressOf()));

	// DXGI 는 기본으로 Alt+Enter 에 독점 전체 화면 전환을 건다. 이 앱은 테두리 없는 창만 쓴다.
	CHECK_FATAL(Factory->MakeWindowAssociation(Window.GetHandle(), DXGI_MWA_NO_ALT_ENTER));

	CreateSizeDependentResources();
}

void FD3D11Graphics::CreateSizeDependentResources()
{
	ComPtr<ID3D11Texture2D> BackBuffer;
	CHECK_FATAL(SwapChain->GetBuffer(0, IID_PPV_ARGS(BackBuffer.GetAddressOf())));
	CHECK_FATAL(Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, RenderTargetView.GetAddressOf()));
}
