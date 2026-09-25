#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

class FModel
{
public:
	FModel() = default;
	FModel(const FModel& Other) = delete;
	FModel& operator=(const FModel& Other) = delete;

	void Initialize(ID3D11Device* Device);

	void Render(ID3D11DeviceContext* Context) const;

private:
	struct FVertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Color;
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
};
