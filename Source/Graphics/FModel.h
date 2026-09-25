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

	UINT GetIndexCount() const { return IndexCount; }

	void Render(ID3D11DeviceContext* Context) const;

private:
	// FColorShader 의 입력 레이아웃, Color.vs 의 FVertexInput 과 멤버 순서·형식이 같아야 한다.
	struct FVertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Color;
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
	UINT IndexCount = 0;
};
