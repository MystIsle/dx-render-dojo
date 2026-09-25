#pragma once

#include <array>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgidebug.h>
#include <wrl/client.h>

class FWindow;
struct FDisplaySettings;

class FD3D11Graphics
{
public:
	FD3D11Graphics() = default;
	FD3D11Graphics(const FD3D11Graphics& Other) = delete;
	FD3D11Graphics& operator=(const FD3D11Graphics& Other) = delete;
	~FD3D11Graphics();

	void Initialize(const FWindow& Window, const FDisplaySettings& Settings);

	void Resize(int Width, int Height);

	void BeginScene(const std::array<float, 4>& Color);
	void EndScene();

private:
	// 창 크기를 따라가는 자원은 여기서 만든다. Initialize 와 Resize 가 같이 부른다.
	void CreateSizeDependentResources();
	void FlushDebugMessages();

	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView;
	Microsoft::WRL::ComPtr<IDXGIInfoQueue> InfoQueue;
	bool bVSync = true;
};
