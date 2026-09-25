#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

class FColorShader
{
public:
	FColorShader() = default;
	FColorShader(const FColorShader& Other) = delete;
	FColorShader& operator=(const FColorShader& Other) = delete;

	void Initialize(ID3D11Device* Device);

	void Render(ID3D11DeviceContext* Context,
	            UINT IndexCount,
	            const DirectX::XMMATRIX& World,
	            const DirectX::XMMATRIX& View,
	            const DirectX::XMMATRIX& Projection) const;

private:
	// Color.vs 의 cbuffer MatrixBuffer 와 멤버 순서가 같아야 한다.
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
