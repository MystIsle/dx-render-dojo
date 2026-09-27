#include "App/FApplication.h"

#include "Graphics/FD3D11Graphics.h"

void FApplication::Initialize(FD3D11Graphics& Graphics)
{
	Model.Initialize(Graphics.GetDevice());
}

void FApplication::Update()
{
}

void FApplication::Render()
{
}
