#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class FModel
{
public:
	void Initialize(ID3D11Device* Device);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer;
	UINT IndexCount = 0;
};
