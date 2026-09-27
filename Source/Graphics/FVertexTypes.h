#pragma once

#include <DirectXMath.h>
#include <array>
#include <d3d11.h>

struct FVertexPositionColor
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT4 Color;

	// 정의는 FVertexTypes.cpp 에 있다. 오프셋을 offsetof 로 적으려면 구조체가 다 정의된 뒤여야 한다.
	static const std::array<D3D11_INPUT_ELEMENT_DESC, 2> InputElements;
};

// InputElements 의 형식(float 3개, float 4개)과 크기가 맞는지 본다.
static_assert(sizeof(FVertexPositionColor) == 28);
