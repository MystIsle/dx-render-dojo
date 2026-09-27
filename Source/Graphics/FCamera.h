#pragma once

#include <DirectXMath.h>

class FCamera
{
public:
	DirectX::XMMATRIX XM_CALLCONV GetViewMatrix() const;
	DirectX::XMMATRIX XM_CALLCONV GetProjectionMatrix(float AspectRatio) const;

	void SetPosition(float X, float Y, float Z);

private:
	DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
	float FovAngleY = DirectX::XM_PIDIV4;
	float NearZ = 0.3f;
	float FarZ = 1000.0f;
};
