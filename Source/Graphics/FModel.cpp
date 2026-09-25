#include "Graphics/FModel.h"

#include <array>
#include <cstdint>

#include "Utility/Check.h"

using namespace DirectX;

void FModel::Initialize(ID3D11Device* Device)
{
	// 시계 방향으로 적어야 앞면이다. 반대로 적으면 뒷면으로 보고 그리지 않는다.
	constexpr std::array<FVertex, 3> Vertices = {{
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
	CHECK_FATAL(Device->CreateBuffer(&VertexBufferDesc, &VertexData, VertexBuffer.GetAddressOf()));

	D3D11_BUFFER_DESC IndexBufferDesc = {};
	IndexBufferDesc.ByteWidth = sizeof(Indices);
	IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA IndexData = {};
	IndexData.pSysMem = Indices.data();
	CHECK_FATAL(Device->CreateBuffer(&IndexBufferDesc, &IndexData, IndexBuffer.GetAddressOf()));

	IndexCount = static_cast<UINT>(Indices.size());
}

void FModel::Render(ID3D11DeviceContext* Context) const
{
	constexpr UINT Stride = sizeof(FVertex);
	constexpr UINT Offset = 0;
	Context->IASetVertexBuffers(0, 1, VertexBuffer.GetAddressOf(), &Stride, &Offset);
	Context->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
