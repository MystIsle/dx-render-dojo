#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class FD3D11Graphics
{
public:
	FD3D11Graphics() = default;
	FD3D11Graphics(const FD3D11Graphics& Other) = delete;
	FD3D11Graphics& operator=(const FD3D11Graphics& Other) = delete;

	void Initialize();

private:
	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
};
