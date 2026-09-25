#include "Graphics/FCamera.h"

using namespace DirectX;

XMMATRIX FCamera::GetViewMatrix() const
{
	const XMVECTOR Eye = XMLoadFloat3(&Position);
	const XMVECTOR Forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	const XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	return XMMatrixLookToLH(Eye, Forward, Up);
}

XMMATRIX FCamera::GetProjectionMatrix(float AspectRatio) const
{
	return XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ);
}

void FCamera::SetPosition(float X, float Y, float Z)
{
	Position = {X, Y, Z};
}
