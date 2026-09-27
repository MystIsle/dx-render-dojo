#include "Graphics/FModel.h"

#include <array>
#include <cstdint>

#include "Graphics/FVertexTypes.h"
#include "Utility/Check.h"

using namespace DirectX;

void FModel::Initialize(ID3D11Device* Device)
{
	// 시계 방향으로 적어야 앞면이다. 반대로 적으면 뒷면으로 보고 그리지 않는다.
	constexpr std::array<FVertexPositionColor, 3> Vertices = {{
	    {XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
	    {XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
	    {XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
	}};
	constexpr std::array<std::uint32_t, 3> Indices = {0, 1, 2};

	D3D11_BUFFER_DESC VertexBufferDesc = {};
	VertexBufferDesc.ByteWidth = sizeof(Vertices);
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VertexData = {};
	VertexData.pSysMem = Vertices.data();
	CHECK_FATAL(Device->CreateBuffer(&VertexBufferDesc, &VertexData, VertexBuffer.ReleaseAndGetAddressOf()));

	D3D11_BUFFER_DESC IndexBufferDesc = {};
	IndexBufferDesc.ByteWidth = sizeof(Indices);
	IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA IndexData = {};
	IndexData.pSysMem = Indices.data();
	CHECK_FATAL(Device->CreateBuffer(&IndexBufferDesc, &IndexData, IndexBuffer.ReleaseAndGetAddressOf()));

	IndexCount = static_cast<UINT>(Indices.size());
}
