#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgidebug.h>
#include <wrl/client.h>

class FD3D11Graphics
{
public:
	FD3D11Graphics() = default;
	FD3D11Graphics(const FD3D11Graphics& Other) = delete;
	FD3D11Graphics& operator=(const FD3D11Graphics& Other) = delete;
	~FD3D11Graphics();

	void Initialize(HWND WindowHandle, int Width, int Height, bool bEnableVSync);

	void Resize(int Width, int Height);

	void BeginScene();
	void EndScene();

private:
	// 창 크기를 따라가는 자원은 여기서 만든다. Initialize 와 Resize 가 같이 부른다.
	void CreateSizeDependentResources();
	void FlushDebugMessages();

	void OnFatal();

	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView;
	Microsoft::WRL::ComPtr<IDXGIInfoQueue> InfoQueue;
	DirectX::XMFLOAT4 ClearColor = {0.5f, 0.5f, 0.5f, 1.0f};
	bool bVSync = true;
};
