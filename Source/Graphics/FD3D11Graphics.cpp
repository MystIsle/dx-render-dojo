#include "Graphics/FD3D11Graphics.h"

#include <dxgi1_6.h>
#include <format>

#include "Utility/Check.h"
#include "Utility/FLog.h"

using Microsoft::WRL::ComPtr;

void FD3D11Graphics::Initialize(HWND WindowHandle, int Width, int Height, bool bEnableVSync)
{
	UINT FactoryFlags = 0;

	ComPtr<IDXGIFactory2> Factory;
	CHECK_FATAL(CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(Factory.ReleaseAndGetAddressOf())));

	ComPtr<IDXGIAdapter1> Adapter;
	ComPtr<IDXGIFactory6> Factory6;
	if (SUCCEEDED(Factory.As(&Factory6)))
	{
		CHECK_FATAL(Factory6->EnumAdapterByGpuPreference(
		    0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(Adapter.ReleaseAndGetAddressOf())));
	}
	else
	{
		CHECK_FATAL(Factory->EnumAdapters1(0, Adapter.ReleaseAndGetAddressOf()));
	}

	DXGI_ADAPTER_DESC1 AdapterDesc = {};
	CHECK_FATAL(Adapter->GetDesc1(&AdapterDesc));
	FLog::Info(std::format(
	    L"그래픽 카드 : {} ({} MB)", AdapterDesc.Description, AdapterDesc.DedicatedVideoMemory / 1024 / 1024));

	UINT Flags = 0;

	const D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;
	CHECK_FATAL(D3D11CreateDevice(Adapter.Get(),
	                              D3D_DRIVER_TYPE_UNKNOWN,
	                              nullptr,
	                              Flags,
	                              &FeatureLevel,
	                              1,
	                              D3D11_SDK_VERSION,
	                              Device.ReleaseAndGetAddressOf(),
	                              nullptr,
	                              Context.ReleaseAndGetAddressOf()));
}
