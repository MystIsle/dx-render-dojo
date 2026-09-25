#pragma once

#include <array>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

class FWindow;
struct FDisplaySettings;

class FD3D11Graphics
{
public:
	FD3D11Graphics() = default;
	FD3D11Graphics(const FD3D11Graphics& Other) = delete;
	FD3D11Graphics& operator=(const FD3D11Graphics& Other) = delete;

	void Initialize(const FWindow& Window, const FDisplaySettings& Settings);

	void BeginScene(const std::array<float, 4>& Color);
	void EndScene();

private:
	void CreateSizeDependentResources();

	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView;
	bool bVSync = true;
};
