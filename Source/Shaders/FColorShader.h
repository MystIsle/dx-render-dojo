#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class FColorShader
{
public:
	FColorShader() = default;
	FColorShader(const FColorShader& Other) = delete;
	FColorShader& operator=(const FColorShader& Other) = delete;

	void Initialize(ID3D11Device* Device);

	void Render(ID3D11DeviceContext* Context, UINT IndexCount) const;

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
};
