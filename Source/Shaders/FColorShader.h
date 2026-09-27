#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

class FColorShader
{
public:
	void Initialize(ID3D11Device* Device);

	void XM_CALLCONV SetMatrices(ID3D11DeviceContext* Context,
	                             DirectX::FXMMATRIX World,
	                             DirectX::CXMMATRIX View,
	                             DirectX::CXMMATRIX Projection);

	void Bind(ID3D11DeviceContext* Context) const;

private:
	// Color.hlsl 의 cbuffer MatrixBuffer 와 멤버 순서가 같아야 한다.
	struct FMatrixBuffer
	{
		DirectX::XMFLOAT4X4 World;
		DirectX::XMFLOAT4X4 View;
		DirectX::XMFLOAT4X4 Projection;
	};

	Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> MatrixBuffer;
};
