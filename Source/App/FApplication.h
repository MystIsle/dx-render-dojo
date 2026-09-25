#pragma once

#include <array>

#include "Graphics/FModel.h"
#include "Shaders/FColorShader.h"

class FD3D11Graphics;

class FApplication
{
public:
	FApplication() = default;
	FApplication(const FApplication& Other) = delete;
	FApplication& operator=(const FApplication& Other) = delete;

	void Initialize(FD3D11Graphics& Graphics);

	const std::array<float, 4>& GetClearColor() const { return ClearColor; }

	void Update();
	void Render(FD3D11Graphics& Graphics);

private:
	std::array<float, 4> ClearColor = {0.5f, 0.5f, 0.5f, 1.0f};
	FModel Model;
	FColorShader ColorShader;
};
