#pragma once

#include <DirectXMath.h>

class FCamera
{
public:
	FCamera() = default;
	FCamera(const FCamera& Other) = delete;
	FCamera& operator=(const FCamera& Other) = delete;

	DirectX::XMMATRIX GetViewMatrix() const;
	DirectX::XMMATRIX GetProjectionMatrix(float AspectRatio) const;

	void SetPosition(float X, float Y, float Z);

private:
	DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
	float FovAngleY = DirectX::XM_PIDIV4;
	float NearZ = 0.3f;
	float FarZ = 1000.0f;
};
