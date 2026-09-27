#pragma once

#include "Graphics/FModel.h"
#include "Shaders/FColorShader.h"

class FD3D11Graphics;

class FApplication
{
public:
	void Initialize(FD3D11Graphics& Graphics);

	void Update();
	void Render();

private:
	FModel Model;
	FColorShader ColorShader;
};
