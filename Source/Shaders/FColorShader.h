#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class FColorShader
{
public:
	void Initialize(ID3D11Device* Device);

	void Bind(ID3D11DeviceContext* Context) const;

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
};
