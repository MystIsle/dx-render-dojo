#pragma once

#include "Graphics/FCamera.h"
#include "Graphics/FModel.h"
#include "Shaders/FColorShader.h"

class FD3D11Graphics;

class FApplication
{
public:
	void Initialize(FD3D11Graphics& Graphics);

	void Update();
	void Render(FD3D11Graphics& Graphics);

private:
	FCamera Camera;
	FModel Model;
	FColorShader ColorShader;
};
