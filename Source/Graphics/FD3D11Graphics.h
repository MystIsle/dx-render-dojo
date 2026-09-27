#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

class FD3D11Graphics
{
public:
	FD3D11Graphics() = default;
	FD3D11Graphics(const FD3D11Graphics& Other) = delete;
	FD3D11Graphics& operator=(const FD3D11Graphics& Other) = delete;

	void Initialize(HWND WindowHandle, int Width, int Height, bool bEnableVSync);

	void BeginScene();
	void EndScene();

private:
	void CreateSizeDependentResources();

	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView;
	DXGI_RGBA ClearColor = {0.5f, 0.5f, 0.5f, 1.0f};
	bool bVSync = true;
};
